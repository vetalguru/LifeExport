#include "LifeExportFilterDriver.h"
#include "Globals.h"


//*************************************************************************
//    Driver initialization and unload routines.
//*************************************************************************

#ifndef ALLOC_PRAGMA
    #pragma alloc_text(INIT,  DriverEntry)
    #pragma alloc_text(PAGED, AA_Unload)
#endif


_Use_decl_annotations_
NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  aDriverObject,
    _In_ PUNICODE_STRING aRegistryPath
)
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    aDriverObject - Pointer to driver object created by the system to
                    represent this driver.

    aRegistryPath - Unicode string identifying where the parameters for this
                    driver are located in the registry.

Return Value:

    Returns the final status of this operation.

--*/
{
    UNREFERENCED_PARAMETER(aRegistryPath);

    NTSTATUS status = STATUS_SUCCESS;

    // TODO: Init grobal data
    RtlZeroMemory(&GlobalData, sizeof(GlobalData));

    // Register filter with FltMgr
    status = FltRegisterFilter(aDriverObject, &FilterRegistration, &GlobalData.FilterHandle);
    FLT_ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    // Start filtering
    status = FltStartFiltering(GlobalData.FilterHandle);
    if (!NT_SUCCESS(status))
    {
        FltUnregisterFilter(GlobalData.FilterHandle);
        GlobalData.FilterHandle = NULL;

        return status;
    }

    // TODO: Create communication port server for alert

    // TODO: Create comminication port server for CREATE messages

    // TODO: Create communication port server for READ messages

    return status;
}


NTSTATUS
AA_Unload(
    _Unreferenced_parameter_ FLT_FILTER_UNLOAD_FLAGS aFlags
)
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unloaded indicated by the Flags
    parameter.

Arguments:

    aFlags - Indicating if this is a mandatory unload.

Return Value:

    Returns the final status of this operation.

--*/
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(aFlags);

    // TODO: Close comminication port server for READ message

    // TODO: Close communication port server for CREATE messages

    // TODO: Close communication port server for alert messages

    // Unregister filter
    FltUnregisterFilter(GlobalData.FilterHandle);
    GlobalData.FilterHandle = NULL;

    // TODO: Free global data

    NTSTATUS status = STATUS_SUCCESS;

    return status;
}
