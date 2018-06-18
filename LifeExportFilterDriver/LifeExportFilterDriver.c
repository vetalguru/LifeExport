#include "LifeExportFilterDriver.h"
#include "Globals.h"
#include "CommunicationPort.h"
#include "Communication.h"
#include "Context.h"
#include "Utility.h"

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
        AA_PostCreate
    },

    {
        IRP_MJ_CLOSE,
        0,
        AA_PreClose,
        NULL
    },

    {
        IRP_MJ_READ,
        FLTFL_OPERATION_REGISTRATION_SKIP_PAGING_IO /* | FLTFL_OPERATION_REGISTRATION_SKIP_CACHED_IO */,
        AA_PreRead,
        NULL
    },

    {
        IRP_MJ_OPERATION_END
    }

};


// Context registration structure
CONST FLT_CONTEXT_REGISTRATION ContextRegistration[] =
{
    {
        FLT_FILE_CONTEXT,
        0,
        AA_FileContextCleanup,
        sizeof(AA_FILE_CONTEXT),
        AA_FILE_CONTEXT_TAG
    },

    {
        FLT_CONTEXT_END
    }

};


//
// This defines what we want to filter with FltMgr
//
CONST FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),    // Size
    FLT_REGISTRATION_VERSION,    // Version
    0,                           // Flags

    ContextRegistration,         // Context Registration
    Callbacks,                   // Operation Registration

    AA_Unload,                   // FilterUnload Callback
    AA_InstanceSetup,            // InstanceSetup Callback
    NULL,                        // InstanceQueryTeardown Callback
    NULL,                        // InstanceTeardownStart Callback
    NULL,                        // InstanceTeardownComplete Callback

    NULL,                        // GenerateFileName Callback
    NULL,                        // GenerateDestinationFileName Callback
    NULL                         // NormalizeNameComponent Callback
};


//
// Local function declaration
//
NTSTATUS
AA_SendUnloadingMessageToUserMode(
    VOID
);


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, AA_InstanceSetup)
    #pragma alloc_text(PAGE, AA_Unload)
    #pragma alloc_text(PAGE, AA_PreCreate)
    #pragma alloc_text(PAGE, AA_PostCreate)
    #pragma alloc_text(PAGE, AA_PreClose)
    // Local function
    #pragma alloc_text(PAGE, AA_SendUnloadingMessageToUserMode)
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
        // Close filter handle
        if (GlobalData.FilterHandle != NULL)
        {
            FltUnregisterFilter(GlobalData.FilterHandle);
            GlobalData.FilterHandle = NULL;
        }

        return status;
    }

    // Create communication port server for CONTROL message
    status = AA_CreateCommunicationPort(sd, LIFE_EXPORT_CONTROL_CONNECTION_TYPE);
    if (!NT_SUCCESS(status))
    {
        // Close security descriptor
        if (sd != NULL)
        {
            FltFreeSecurityDescriptor(sd);
            sd = NULL;
        }

        // Close filter handle
        if (GlobalData.FilterHandle != NULL)
        {
            FltUnregisterFilter(GlobalData.FilterHandle);
            GlobalData.FilterHandle = NULL;
        }

        return status;
    }

    // Create comminication port server for CREATE messages
    status = AA_CreateCommunicationPort(sd, LIFE_EXPORT_CREATE_CONNECTION_TYPE);
    if (!NT_SUCCESS(status))
    {
        // Close CONTROL server port
        if (GlobalData.ServerPortControl != NULL)
        {
            AA_CloseCommunicationPort(LIFE_EXPORT_CONTROL_CONNECTION_TYPE);
            GlobalData.ServerPortControl = NULL;
        }

        // Close security descriptor
        if (sd != NULL)
        {
            FltFreeSecurityDescriptor(sd);
            sd = NULL;
        }

        // Close filter handle
        if (GlobalData.FilterHandle != NULL)
        {
            FltUnregisterFilter(GlobalData.FilterHandle);
            GlobalData.FilterHandle = NULL;
        }

        return status;
    }

    // Create communication port server for READ messages
    status = AA_CreateCommunicationPort(sd, LIFE_EXPORT_READ_CONNECTION_TYPE);
    if (!NT_SUCCESS(status))
    {
        // Close CREATE server port
        if (GlobalData.ServerPortCreate != NULL)
        {
            AA_CloseCommunicationPort(LIFE_EXPORT_CREATE_CONNECTION_TYPE);
            GlobalData.ServerPortCreate = NULL;
        }

        // Close CONTROL server port
        if (GlobalData.ServerPortControl != NULL)
        {
            AA_CloseCommunicationPort(LIFE_EXPORT_CONTROL_CONNECTION_TYPE);
            GlobalData.ServerPortControl = NULL;
        }

        // Close security descriptor
        if (sd != NULL)
        {
            FltFreeSecurityDescriptor(sd);
            sd = NULL;
        }

        // Close filter handle
        if (GlobalData.FilterHandle != NULL)
        {
            FltUnregisterFilter(GlobalData.FilterHandle);
            GlobalData.FilterHandle = NULL;
        }

        return status;
    }


    // Start filtering
    status = FltStartFiltering(GlobalData.FilterHandle);
    if (!NT_SUCCESS(status))
    {
        // Close READ server port 
        if (GlobalData.ServerPortRead != NULL)
        {
            AA_CloseCommunicationPort(LIFE_EXPORT_READ_CONNECTION_TYPE);
            GlobalData.ServerPortRead = NULL;
        }

        // Close CREATE server port 
        if (GlobalData.ServerPortCreate != NULL)
        {
            AA_CloseCommunicationPort(LIFE_EXPORT_CREATE_CONNECTION_TYPE);
            GlobalData.ServerPortCreate = NULL;
        }

        // Close CONTROL server port
        if (GlobalData.ServerPortControl != NULL)
        {
            AA_CloseCommunicationPort(LIFE_EXPORT_CONTROL_CONNECTION_TYPE);
            GlobalData.ServerPortControl = NULL;
        }

        // Close security descriptor
        if (sd != NULL)
        {
            FltFreeSecurityDescriptor(sd);
            sd = NULL;
        }

        // Close filter handle
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
AA_InstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS    aFltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS aFlags,
    _In_ DEVICE_TYPE              aVolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE      aVolumeFilesystemType
)
{
    UNREFERENCED_PARAMETER(aFltObjects);
    UNREFERENCED_PARAMETER(aFlags);
    UNREFERENCED_PARAMETER(aVolumeDeviceType);
    UNREFERENCED_PARAMETER(aVolumeFilesystemType);

    NTSTATUS status = STATUS_SUCCESS;
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

    // Need send UNLOAD message to user-mode
    AA_SendUnloadingMessageToUserMode();

    // Close comminication port server for READ message
    AA_CloseCommunicationPort(LIFE_EXPORT_READ_CONNECTION_TYPE);
    GlobalData.ServerPortRead = NULL;

    // Close communication port server for CREATE messages
    AA_CloseCommunicationPort(LIFE_EXPORT_CREATE_CONNECTION_TYPE);
    GlobalData.ServerPortCreate = NULL;

    // Close communication port server for alert messages
    AA_CloseCommunicationPort(LIFE_EXPORT_CONTROL_CONNECTION_TYPE);
    GlobalData.ServerPortControl = NULL;

    // Unregister filter
    FltUnregisterFilter(GlobalData.FilterHandle);
    GlobalData.FilterHandle = NULL;

    // TODO: Free global data

    return STATUS_SUCCESS;
}


//
// Local function definition
//

NTSTATUS
AA_SendUnloadingMessageToUserMode (
    VOID
)
/*++

Routine Description:

    This routine sends unloading message to the user program.

Arguments:

    None.

Return Value:

    The return value is the status of the operation.

--*/
{
    PAGED_CODE();

    if (GlobalData.ClientPortControl == NULL)
    {
        return STATUS_HANDLES_CLOSED;
    }

    LIFE_EXPORT_CONTROL_NOTIFICATION_REQUEST request = { 0 };
    request.Message = CONTROL_RESULT_UNLOADING;

    LIFE_EXPORT_CONTROL_NOTIFICATION_RESPONSE response = { 0 };
    ULONG replyLength = sizeof(response);
    NTSTATUS status = FltSendMessage(GlobalData.FilterHandle,
        &GlobalData.ClientPortControl,
        &request,
        sizeof(request),
        &response,
        &replyLength,
        NULL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    if (response.ConnectionResult != CONTROL_RESULT_UNLOAD)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return status;
}


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
        LIFE_EXPORT_CREATE_NOTIFICATION_REQUEST request = { 0 };
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

        RtlCopyMemory(&request.FileId, &fileId, sizeof(fileId));

        LIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE response = { 0 };
        ULONG reponseLength = sizeof(response);

        status = FltSendMessage(GlobalData.FilterHandle,
            &GlobalData.ClientPortCreate,
            &request,
            sizeof(request),
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


FLT_PREOP_CALLBACK_STATUS
AA_PreRead(
    _Inout_                        PFLT_CALLBACK_DATA    aData,
    _In_                           PCFLT_RELATED_OBJECTS aFltObjects,
    _Flt_CompletionContext_Outptr_ PVOID                 *aCompletionContext
)
/*++

Routine Description:

    Handle pre-read.

Arguments:

    aData - Pointer to the filter callbackData that is passed to us.

    aFltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
    opaque handles to this filter, instance, its associated volume and
    file object.

    aCompletionContext - The context for the completion routine for this
    operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER(aFltObjects);
    UNREFERENCED_PARAMETER(aCompletionContext);

    //  Skip IRP_PAGING_IO, IRP_SYNCHRONOUS_PAGING_IO and TopLevelIrp
    if ((aData->Iopb->IrpFlags & IRP_PAGING_IO) ||
        (aData->Iopb->IrpFlags & IRP_SYNCHRONOUS_PAGING_IO) ||
        IoGetTopLevelIrp())
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (!FLT_IS_IRP_OPERATION(aData))
    {
        return FLT_PREOP_DISALLOW_FASTIO;
    }

    if (GlobalData.ServerPortRead == NULL)
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (GlobalData.ClientPortRead == NULL)
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    PAA_FILE_CONTEXT fileContext = NULL;

    NTSTATUS status = FltGetFileContext(aData->Iopb->TargetInstance,
        aData->Iopb->TargetFileObject,
        &fileContext);
    if (!NT_SUCCESS(status))
    {
        // It is not life exported file
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    LIFE_EXPORT_READ_NOTIFICATION_REQUEST request = { 0 };
    RtlCopyMemory(&request.FileId, &fileContext->FileID, sizeof(fileContext->FileID));
    request.BlockFileOffset = aData->Iopb->Parameters.Read.ByteOffset.QuadPart;
    request.BlockLength = aData->Iopb->Parameters.Read.Length;

    LIFE_EXPORT_READ_NOTIFICATION_RESPONSE response = { 0 };
    ULONG responseLength = sizeof(response);
    status = FltSendMessage(GlobalData.FilterHandle,
        &GlobalData.ClientPortRead,
        &request,
        sizeof(request),
        &response,
        &responseLength,
        NULL);
    if (!NT_SUCCESS(status) || (status == STATUS_TIMEOUT))
    {
        if (status == STATUS_PORT_DISCONNECTED)
        {
            FltCloseClientPort(GlobalData.FilterHandle, &GlobalData.ClientPortRead);
            GlobalData.ClientPortRead = NULL;
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }

        if (status != STATUS_TIMEOUT)
        {
            return FLT_PREOP_SUCCESS_NO_CALLBACK;
        }

        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


