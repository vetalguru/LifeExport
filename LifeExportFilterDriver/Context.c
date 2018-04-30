#include "Context.h"
#include "Utility.h"


// Local functions
NTSTATUS
AA_CreateFileContext(
	_Outptr_ PAA_FILE_CONTEXT *aFileContext
);


#ifdef ALLOC_PRAGMA
	#pragma alloc_text(PAGE, AA_CreateFileContext)
	#pragma alloc_text(PAGE, AA_FileContextCleanup)
#endif


NTSTATUS
AA_CreateFileContext(
	_Outptr_ PAA_FILE_CONTEXT *aFileContext
)
/*++

Routine Description:

	This routine creates a new file context

Arguments:

	aFileContext         - Returns the file context

Return Value:

	Status

--*/
{
	PAGED_CODE();

	*aFileContext = NULL;

	PAA_FILE_CONTEXT fileContext = NULL;
	NTSTATUS status = FltAllocateContext(GlobalData.FilterHandle,
		FLT_FILE_CONTEXT,
		AA_FILE_CONTEXT_SIZE,
		PagedPool,
		&fileContext);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

	RtlZeroMemory(fileContext, AA_FILE_CONTEXT_SIZE);
	*aFileContext = fileContext;

	return STATUS_SUCCESS;
}


VOID
AA_FileContextCleanup(
	_In_ PFLT_CONTEXT     aContext,
	_In_ FLT_CONTEXT_TYPE aContextType
)
/*++

Routine Description:

	This routine is called whenever the file context is about to be destroyed.
	Typically we need to clean the data structure inside it.

Arguments:

	aContext - Pointer to the PAA_FILE_CONTEXT data structure.
	aContextType - This value should be FLT_FILE_CONTEXT.

Return Value:

	None

--*/
{
	UNREFERENCED_PARAMETER(aContextType);
	UNREFERENCED_PARAMETER(aContext);

	PAGED_CODE()

	/*PAA_FILE_CONTEXT fileContext = (PAA_FILE_CONTEXT)aContext;*/
}
