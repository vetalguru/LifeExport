#include "LifeExportFilterDriver.h"
#include "Globals.h"
#include "CommunicationPort.h"
#include "Communication.h"


//*************************************************************************
//    Driver initialization and unload routines.
//*************************************************************************


//
// operation registration
//
CONST FLT_OPERATION_REGISTRATION Callbacks[] =
{
	{
		IRP_MJ_CREATE,
		0,
		AA_PreCreate,
		NULL
	},

{ IRP_MJ_OPERATION_END }
};


//
// This defines what we want to filter with FltMgr 
//
CONST FLT_REGISTRATION FilterRegistration = {
	sizeof(FLT_REGISTRATION),    // Size
	FLT_REGISTRATION_VERSION,    // Version
	0,                           // Flags

	NULL,                        // Context Registration
	Callbacks,                   // Operation Registration

	AA_Unload,                   // FilterUnload Callback
	NULL,                        // InstanceSetup Callback
	NULL,                        // InstanceQueryTeardown Callback
	NULL,                        // InstanceTeardownStart Callback
	NULL,                        // InstanceTeardownComplete Callback

	NULL,                        // GenerateFileName Callback
	NULL,                        // GenerateDestinationFileName Callback 
	NULL                         // NormalizeNameComponent Callback
};



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

	// Create default security descriptor for FltCreateCommunicationPort
	PSECURITY_DESCRIPTOR sd = NULL;
	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);
	if (!NT_SUCCESS(status))
	{
		return status;
	}

    // TODO: Create communication port server for alert

    // Create comminication port server for CREATE messages
	status = AA_CreateCommunicationPort(sd, LIFE_EXPORT_CREATE_CONNECTION_TYPE);
	if (!NT_SUCCESS(status))
	{
		if (sd != NULL)
		{
			FltFreeSecurityDescriptor(sd);
			sd = NULL;
		}

		if (GlobalData.FilterHandle != NULL)
		{
			FltUnregisterFilter(GlobalData.FilterHandle);
			GlobalData.FilterHandle = NULL;
		}

		return status;
	}


    // TODO: Create communication port server for READ messages


	// Start filtering
	status = FltStartFiltering(GlobalData.FilterHandle);
	if (!NT_SUCCESS(status))
	{
		if (GlobalData.ServerPortCreate != NULL)
		{
			AA_CloseCommunicationPort(LIFE_EXPORT_CREATE_CONNECTION_TYPE);
		}

		if (sd != NULL)
		{
			FltFreeSecurityDescriptor(sd);
			sd = NULL;
		}

		if (GlobalData.FilterHandle != NULL)
		{
			FltUnregisterFilter(GlobalData.FilterHandle);
			GlobalData.FilterHandle = NULL;
		}

		return status;
	}


	if (sd != NULL)
	{
		FltFreeSecurityDescriptor(sd);
		sd = NULL;
	}

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

    // Close communication port server for CREATE messages
	AA_CloseCommunicationPort(LIFE_EXPORT_CREATE_CONNECTION_TYPE);

    // TODO: Close communication port server for alert messages

    // Unregister filter
    FltUnregisterFilter(GlobalData.FilterHandle);
    GlobalData.FilterHandle = NULL;

    // TODO: Free global data

    return STATUS_SUCCESS;
}
