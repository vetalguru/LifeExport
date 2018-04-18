#include "Callbacks.h"



#ifdef ALLOC_PRAGMA
	#pragma alloc_text(PAGE, AA_PreCreate)
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

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}
