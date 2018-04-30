#include "Callbacks.h"
#include "Globals.h"
#include "Communication.h"
#include "Utility.h"
#include "Context.h"



#ifdef ALLOC_PRAGMA
	#pragma alloc_text(PAGE, AA_PreCreate)
	#pragma alloc_text(PAGE, AA_PostCreate)
	#pragma alloc_text(POAGE, AA_PreClose)
#endif


FLT_PREOP_CALLBACK_STATUS
AA_PreCreate(
	_Inout_                        PFLT_CALLBACK_DATA     aData,
	_In_                           PCFLT_RELATED_OBJECTS  aFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID                 *aCompletionContext
)
/*++

Routine Description:

	This routine is the pre-create completion routine.

Arguments:

	aData - Pointer to the filter callbackData that is passed to us.

	aFltObjects - Pointer to the FLT_RELATED_OBJECTS data structure cpontaining
		opaque handles to this filter, instance, its associated volume and
		file object.

	aCompletionContext - If this callback routine returns FLT_PREOP_SUCCESS_WITH_CALLBACKS or
		FLT_PREOP_SYNCHRONIZE, this parameter is an optional context pointer to be passed to
		the corresponding post-operation callback routine. Otherwise, it must be NULL.

Return Value:

	FLT_PREOP_SYNCHRONIZE - PostCreate needs to be called back synchronizedly.
	FLT_PREOP_SUCCESS_NO_CALLBACK - PostCreate does not need to be called.

--*/
{
	UNREFERENCED_PARAMETER(aFltObjects);
	UNREFERENCED_PARAMETER(aCompletionContext);

	PAGED_CODE();

	//
	// Directory opens don't need to be life exported 
	//
	if (FlagOn(aData->Iopb->Parameters.Create.Options, FILE_DIRECTORY_FILE))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	//
	// Directory pre-rename operations which always open a directory 
	//
	if (FlagOn(aData->Iopb->OperationFlags, SL_OPEN_TARGET_DIRECTORY))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	//
	// Skip paging files 
	//
	if (FlagOn(aData->Iopb->OperationFlags, SL_OPEN_PAGING_FILE))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	return FLT_PREOP_SYNCHRONIZE;
}


FLT_POSTOP_CALLBACK_STATUS
AA_PostCreate(
	_Inout_  PFLT_CALLBACK_DATA       aData,
	_In_     PCFLT_RELATED_OBJECTS    aFltObjects,
	_In_opt_ PVOID                    aCompletionContext,
	_In_     FLT_POST_OPERATION_FLAGS aFlags
)
/*++

Routine Description:

	This routine is the post-create completion routine.
	In this routine, stream context and/or transaction context shall be
	created if not exits.

	Note that we only allocate and set the stream context to filter manager
	at post create.

Arguments:

	aData - Pointer to the filter callbackData that is passed to us.

	aFltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
				  opaque handles to this filter, instance, its associated volume and
				  file object.

	aCompletionContext - The completion context set in the pre-create routine.

	aFlags - Denotes whether the completion is successful or is being drained.

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(aCompletionContext);
	UNREFERENCED_PARAMETER(aFlags);

	PAGED_CODE();

	NTSTATUS status = aData->IoStatus.Status;
	if (!NT_SUCCESS(status) || status == STATUS_REPARSE)
	{

		// File creation may fail
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	//  After creation, skip it if it is directory
	BOOLEAN isDir = FALSE;
	status = FltIsDirectory(aFltObjects->FileObject,
		aFltObjects->Instance,
		&isDir);
	//  If FltIsDirectory failed, we do not know if it is a directoy,
	//  we let it go through because if it is a directory, it will fail
	//  at section creation anyway.
	if (!NT_SUCCESS(status) && isDir)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	//  We skip the encrypted file open without FILE_WRITE_DATA and FILE_READ_DATA
	//  This is because if application calls OpenEncryptedFileRaw(...) for backup, 
	//  it won't have to decrypt the file. In such case, if we scan it, we will hit 
	//  an assertion error in NTFS because it does not have the encryption context.
	//  Thus, we have to skip the encrypted file not open for read/write.
	ACCESS_MASK desiredAccess = aData->Iopb->Parameters.Create.SecurityContext->DesiredAccess;
	if (!(FlagOn(desiredAccess, FILE_WRITE_DATA)) &&
		!(FlagOn(desiredAccess, FILE_READ_DATA)))
	{
		BOOLEAN encrypted = FALSE;
		status = AA_GetFileEncrypted(aFltObjects->Instance,
			aFltObjects->FileObject,
			&encrypted);

		if (encrypted == TRUE)
		{
			return FLT_POSTOP_FINISHED_PROCESSING;
		}
	}

	//  Skip the alternate data stream
	if (AA_IsStreamAlternate(aData))
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	//
	// 	Get FileID and send it to usermode
	//

	if (GlobalData.ServerPortCreate == NULL)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	if (GlobalData.ClientPortCreate == NULL)
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	// Check if file context supported
	if (!FltSupportsFileContexts(aFltObjects->FileObject))
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	PAA_FILE_CONTEXT fileContext = NULL;
	status = FltGetFileContext(aData->Iopb->TargetInstance,
		aData->Iopb->TargetFileObject,
		&fileContext);
	if (status == STATUS_NOT_FOUND)
	{
		LIFE_EXPORT_CREATE_NOTIFICATION_REQUEST notification = { 0 };
		AA_FILE_ID_INFO fileId;
		status = AA_GetFileId(aFltObjects->Instance, aFltObjects->FileObject, &fileId);
		if (!NT_SUCCESS(status))
		{
			if (status == STATUS_POSSIBLE_DEADLOCK)
			{
				AA_SET_INVALID_FILE_REFERENCE(fileId);
			}
			else
			{
				return FLT_POSTOP_FINISHED_PROCESSING;
			}
		}

		RtlCopyMemory(&notification.FileId, &fileId, sizeof(fileId));

		LIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE response = { 0 };
		ULONG reponseLength = sizeof(LIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE);

		status = FltSendMessage(GlobalData.FilterHandle,
			&GlobalData.ClientPortCreate,
			&notification,
			sizeof(LIFE_EXPORT_CREATE_NOTIFICATION_REQUEST),
			&response,
			&reponseLength,
			NULL);
		if (!NT_SUCCESS(status) || (status == STATUS_TIMEOUT))
		{
			if (status == STATUS_PORT_DISCONNECTED)
			{
				FltCloseClientPort(GlobalData.FilterHandle, &GlobalData.ClientPortCreate);
				GlobalData.ClientPortCreate = NULL;
				return FLT_POSTOP_FINISHED_PROCESSING;
			}

			if (status != STATUS_TIMEOUT)
			{
				return FLT_POSTOP_FINISHED_PROCESSING;
			}

			return FLT_POSTOP_FINISHED_PROCESSING;
		}

		if (response.ConnectionResult == CREATE_RESULT_EXPORT_FILE)
		{
			// Need to create a context for the exported file if it is not created
			AA_CreateFileContext(&fileContext);
			if (!NT_SUCCESS(status))
			{
				return FLT_POSTOP_FINISHED_PROCESSING;
			}

			RtlZeroMemory(fileContext, sizeof(AA_FILE_CONTEXT));
			RtlCopyMemory(&fileContext->FileID, &fileId, sizeof(fileId));

			status = FltSetFileContext(aData->Iopb->TargetInstance,
				aData->Iopb->TargetFileObject,
				FLT_SET_CONTEXT_KEEP_IF_EXISTS,
				fileContext,
				NULL);
			if (!NT_SUCCESS(status))
			{
				FltReleaseContext(fileContext);
			}

			if (status != STATUS_FLT_CONTEXT_ALREADY_DEFINED)
			{
				//  FltSetFileContext failed for a reason other than the context already
				//  existing on the file. So the object now does not have any context set
				//  on it. So we return failure to the caller.
				return FLT_POSTOP_FINISHED_PROCESSING;
			}
		}
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
AA_PreClose(
	_Inout_                        PFLT_CALLBACK_DATA    aData,
	_In_                           PCFLT_RELATED_OBJECTS aFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID                 *aCompletionContext
)
/*++

Routine Description:

	Pre-close callback. Make the file context persistent in the volatile cache.
	If the file is transacted, it will be synced at KTM notification callback
	if committed.

Arguments:

	aData - Pointer to the filter callbackData that is passed to us.

	aFltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
	opaque handles to this filter, instance, its associated volume and
	file object.

	aCompletionContext - If this callback routine returns FLT_PREOP_SUCCESS_WITH_CALLBACK or
	FLT_PREOP_SYNCHRONIZE, this parameter is an optional context pointer to be passed to
	the corresponding post-operation callback routine. Otherwise, it must be NULL.

Return Value:

	The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(aData);
	UNREFERENCED_PARAMETER(aCompletionContext);

	PAGED_CODE();

	// Check file context and release it if it is present
	PAA_FILE_CONTEXT fileContext = NULL;
	NTSTATUS status = FltGetFileContext(aFltObjects->Instance,
		aFltObjects->FileObject,
		&fileContext);
	if (NT_SUCCESS(status))
	{
		FltReleaseContext(fileContext);
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
