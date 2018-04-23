#ifndef _LIFE_EXPORT_FILTER_UTILITY_H_
#define _LIFE_EXPORT_FILTER_UTILITY_H_


#include <fltKernel.h>


typedef union _FILE_ID_INFO
{
	struct
	{
		ULONGLONG Value;
		ULONGLONG UpperZeroes;
	}FileId_64;

	FILE_ID_128 FileId_128;
}FILE_ID_INFO, *PFILE_ID_INFO;




FORCEINLINE
NTSTATUS
AA_CancelFileOpen(
	_Inout_ PFLT_CALLBACK_DATA    aData,
	_In_    PCFLT_RELATED_OBJECTS aFltObjects,
	_In_    NTSTATUS              aStatus = STATUS_SUCCESS
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
	_In_  PFLT_INSTANCE aInstance,
	_In_  PFILE_OBJECT  aFileObject,
	_Out_ PFILE_ID_INFO aFileId
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

	FLT_FILESYSTEM_TYPE fsType;
	NTSTATUS status = FltGetFileSystemType(aInstance, &fsType);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	if (fsType = FLT_FSTYPE_REFS)
	{
		FILE_ID_INFORMATION fileInfo = { 0 };
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
		FILE_INTERNAL_INFORMATION fileInfo = { 0 };
		status = FltQueryInformationFile(aInstance,
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


#endif // _LIFE_EXPORT_FILTER_UTILITY_H_
