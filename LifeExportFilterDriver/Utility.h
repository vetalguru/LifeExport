#ifndef _LIFE_EXPORT_FILTER_UTILITY_H_
#define _LIFE_EXPORT_FILTER_UTILITY_H_


#include <fltKernel.h>
#include "Communication.h"


FORCEINLINE
NTSTATUS
AA_CancelFileOpen(
	_Inout_ PFLT_CALLBACK_DATA    aData,
	_In_    PCFLT_RELATED_OBJECTS aFltObjects,
	_In_    NTSTATUS              aStatus
)
/*++

Routine Description:

	This function cancel the file open. This is supposed to be called at post create if
	the I/O is cancelled.

Arguments:

	aData - Pointer to the filter callbackData that is passed to us.

	aFltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance, its associated volume and
		file object.

	aStatus - The status code to be returned for this IRP.

Return Values:

	Returns the final status of this operation.

--*/
{
	if (aData == NULL)
	{
		return STATUS_INVALID_PARAMETER_1;
	}

	if (aFltObjects == NULL)
	{
		return STATUS_INVALID_PARAMETER_2;
	}

	FltCancelFileOpen(aFltObjects->Instance, aFltObjects->FileObject);
	aData->IoStatus.Status = aStatus;
	aData->IoStatus.Information = 0;

	return STATUS_SUCCESS;
}


FORCEINLINE
NTSTATUS
AA_GetFileId(
	_In_  PFLT_INSTANCE    aInstance,
	_In_  PFILE_OBJECT     aFileObject,
	_Out_ PAA_FILE_ID_INFO aFileId
)
/*++

Routine Description:

	This routine obtains the File ID.

Arguments:

	aInstance - Opaque filter pointer for the caller. This parameter is required and cannot be NULL.

	aFileObject - File object pointer for the file. This parameter is required and cannot be NULL.

	aFileId - Pointer to file id. This is the output

Return Value:

	Returns statuses forwarded from FltQueryInformationFile.
	STATUS_POSSIBLE_DEADLOCK - Possible deadlock condition.
	                           It is not an error, but use is impossible. 

--*/
{
	if (aInstance == NULL)
	{
		return STATUS_INVALID_PARAMETER_1;
	}

	if (aFileObject == NULL)
	{
		return STATUS_INVALID_PARAMETER_2;
	}

	if (aFileId == NULL)
	{
		return STATUS_INVALID_PARAMETER_3;
	}

	RtlZeroMemory(aFileId, sizeof(FILE_ID_128));

	// Need to call IoGetTopLevelIrp
	// https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/fltkernel/nf-fltkernel-fltqueryinformationfile

	PIRP irp = IoGetTopLevelIrp();
	if (irp != NULL)
	{
		return STATUS_POSSIBLE_DEADLOCK;
	}

	FLT_FILESYSTEM_TYPE fsType;
	NTSTATUS status = FltGetFileSystemType(aInstance, &fsType);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (fsType == FLT_FSTYPE_REFS)
	{
		FILE_ID_INFORMATION fileInfo;
		status = FltQueryInformationFile(aInstance,
			aFileObject,
			&fileInfo,
			sizeof(FILE_ID_INFORMATION),
			FileIdInformation,
			NULL);
		if (NT_SUCCESS(status))
		{
			RtlCopyMemory(&(aFileId->FileId_128), &(fileInfo.FileId), sizeof(aFileId->FileId_128));
		}
	}
	else
	{
		FILE_INTERNAL_INFORMATION fileInfo;
		status = FltQueryInformationFile(
			aInstance,
			aFileObject,
			&fileInfo,
			sizeof(FILE_INTERNAL_INFORMATION),
			FileInternalInformation,
			NULL);
		if (NT_SUCCESS(status))
		{
			aFileId->FileId_64.Value = fileInfo.IndexNumber.QuadPart;
		}
	}

	return status;
}


LONG
AA_ExceptionFilter(
	_In_ PEXCEPTION_POINTERS aExceptionPointer,
	_In_ BOOLEAN             aAccessingUserBuffer
)
{
	NTSTATUS status = aExceptionPointer->ExceptionRecord->ExceptionCode;

	//
	//  Certain exceptions shouldn't be dismissed within the filter
	//  unless we're touching user memory.
	//

	if (!FsRtlIsNtstatusExpected(status) && !aAccessingUserBuffer)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}

	return EXCEPTION_EXECUTE_HANDLER;
}


NTSTATUS
AA_GetFileEncrypted(
	_In_   PFLT_INSTANCE aInstance,
	_In_   PFILE_OBJECT  aFileObject,
	_Out_  PBOOLEAN      aEncrypted
)
/*++

Routine Description:

	This routine obtains the File ID and saves it in the stream context.

Arguments:

	aInstance - Opaque filter pointer for the caller. This parameter is required and cannot be NULL.

	aFileObject - File object pointer for the file. This parameter is required and cannot be NULL.

	aEncrypted - Pointer to a boolean indicating if this file is encrypted or not. This is the output.

Return Value:

	Returns statuses forwarded from FltQueryInformationFile.

--*/
{
	FILE_BASIC_INFORMATION basicInfo;
	NTSTATUS status = FltQueryInformationFile(aInstance,
		aFileObject,
		&basicInfo,
		sizeof(FILE_BASIC_INFORMATION),
		FileBasicInformation,
		NULL);
	if (NT_SUCCESS(status))
	{

		*aEncrypted = BooleanFlagOn(basicInfo.FileAttributes, FILE_ATTRIBUTE_ENCRYPTED);
	}

	return status;
}


BOOLEAN
AA_IsStreamAlternate(
	_Inout_ PFLT_CALLBACK_DATA aData
)
/*++

Routine Description:

	This return if data stream is alternate or not.
	It by default returns FALSE if it fails to retrieve the name information
	from the file system.

Arguments:

	aData - Pointer to the filter callbackData that is passed to us.

Return Value:

	TRUE - This data stream is alternate.
	FALSE - This data stream is NOT alternate.

--*/
{
	PAGED_CODE();

	if (aData == NULL)
	{
		return FALSE;
	}

	PFLT_FILE_NAME_INFORMATION nameInfo = NULL;
	NTSTATUS status = FltGetFileNameInformation(aData,
		FLT_FILE_NAME_OPENED | FLT_FILE_NAME_QUERY_ALWAYS_ALLOW_CACHE_LOOKUP,
		&nameInfo);
	if (!NT_SUCCESS(status) || (nameInfo == NULL))
	{
		if (nameInfo != NULL)
		{
			FltReleaseFileNameInformation(nameInfo);
			nameInfo = NULL;
		}

		return FALSE;
	}

	status = FltParseFileNameInformation(nameInfo);
	if (!NT_SUCCESS(status)) {

		if (nameInfo != NULL) {

			FltReleaseFileNameInformation(nameInfo);
			nameInfo = NULL;
		}

		return FALSE;
	}

	BOOLEAN result = (nameInfo->Stream.Length > 0);

	if (nameInfo != NULL) {

		FltReleaseFileNameInformation(nameInfo);
		nameInfo = NULL;
	}

	return result;
}

#endif // _LIFE_EXPORT_FILTER_UTILITY_H_
