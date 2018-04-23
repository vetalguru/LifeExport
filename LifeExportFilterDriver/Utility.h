#ifndef _LIFE_EXPORT_FILTER_UTILITY_H_
#define _LIFE_EXPORT_FILTER_UTILITY_H_


#include <fltKernel.h>



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


#endif // _LIFE_EXPORT_FILTER_UTILITY_H_
