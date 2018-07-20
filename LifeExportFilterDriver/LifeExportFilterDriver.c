#include "LifeExportFilterDriver.h"
#include "Globals.h"
#include "Communication.h"
#include "Utility.h"

//*************************************************************************
//    Driver initialization and unload routines.
//*************************************************************************

//
// Local function declaration
//
NTSTATUS
AA_SendUnloadingMessageToUserMode(
    VOID
);

NTSTATUS
AA_CreateFileContext(
    _Outptr_ PAA_FILE_CONTEXT *aFileContext
);


VOID
AA_FileContextCleanup(
    _In_ PFLT_CONTEXT     aContext,
    _In_ FLT_CONTEXT_TYPE aContextType
);


VOID
AA_VolumeContextCleanup(
    _In_ PFLT_CONTEXT     aContext,
    _In_ FLT_CONTEXT_TYPE aContextType
);

NTSTATUS
AA_ConnectNotifyCallback(
    _In_                             PFLT_PORT aClientPort,
    _In_                             PVOID     aServerPortCookie,
    _In_reads_bytes_(aSizeOfContext) PVOID     aConnectionContext,
    _In_                             ULONG     aSizeOfContext,
    _Outptr_result_maybenull_        PVOID     *aConnectionCookie
);


NTSTATUS
AA_MessageNotifyCallback(
    _In_                                                                     PVOID  aConnectionCoockie,
    _In_reads_bytes_opt_(aInputBufferSize)                                   PVOID  aInputBuffer,
    _In_                                                                     ULONG  aInputBufferSize,
    _Out_writes_bytes_to_opt_(aOutputBufferSize, *aReturnOutputBufferLength) PVOID  aOutputBuffer,
    _In_                                                                     ULONG  aOutputBufferSize,
    _Out_                                                                    PULONG aReturnOutputBufferLength
);


VOID
AA_DisconnectNotifyCallback(
    _In_opt_ PVOID aConnectionCookie
);

FLT_POSTOP_CALLBACK_STATUS
AA_ChangePostReadBuffersWhenSafe(
    _Inout_ PFLT_CALLBACK_DATA       aData,
    _In_    PCFLT_RELATED_OBJECTS    aFltObjects,
    _In_    PVOID                    aCompletionContext,
    _In_    FLT_POST_OPERATION_FLAGS aFlags
);


NTSTATUS
AA_CopyUserModeBufferToKernelByProcessId(
    ULONG aCurrentProcessId,
    PVOID aDestinationKernelBufferPrt,
    PVOID aSourceUserModeBufferPtr,
    ULONG aCopySize
);



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
        AA_PostRead
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
        AA_FILE_CONTEXT_SIZE,
        AA_LIFE_EXPORT_FILE_CONTEXT_TAG
    },

    {
        FLT_VOLUME_CONTEXT,
        0,
        AA_VolumeContextCleanup,
        AA_VOLUME_CONTEXT_SIZE,
        AA_LIFE_EXPORT_VOLUME_CONTEXT_TAG
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


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(INIT, DriverEntry)
    #pragma alloc_text(PAGE, AA_InstanceSetup)
    #pragma alloc_text(PAGE, AA_Unload)
    #pragma alloc_text(PAGE, AA_PreCreate)
    #pragma alloc_text(PAGE, AA_PostCreate)
    #pragma alloc_text(PAGE, AA_PreClose)
    #pragma alloc_text(PAGE, AA_PreRead)
    #pragma alloc_text(PAGE, AA_PostRead)
    #pragma alloc_text(PAGE, AA_CreateCommunicationPort)
    #pragma alloc_text(PAGE, AA_CloseCommunicationPort)
    // Local function
    #pragma alloc_text(PAGE, AA_SendUnloadingMessageToUserMode)
    #pragma alloc_text(PAGE, AA_CreateFileContext)
    #pragma alloc_text(PAGE, AA_FileContextCleanup)
    #pragma alloc_text(PAGE, AA_VolumeContextCleanup)
    #pragma alloc_text(PAGE, AA_ConnectNotifyCallback)
    #pragma alloc_text(PAGE, AA_MessageNotifyCallback)
    #pragma alloc_text(PAGE, AA_DisconnectNotifyCallback)
    #pragma alloc_text(PAGE, AA_ChangePostReadBuffersWhenSafe)
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

    //
    // Init lookaside list used to allocate our context structure used to
    // pass information from out preOperation callback to our postOperation
    // callback.
    //
    ExInitializeNPagedLookasideList(&GlobalData.Pre2PostContextList,
        NULL,
        NULL,
        0,
        sizeof(AA_PRE_2_POST_READ_CONTEXT),
        AA_LIFE_EXPORT_PRE_2_POST_CONTEXT_TAG,
        0);

    // Register filter with FltMgr
    status = FltRegisterFilter(aDriverObject, &FilterRegistration, &GlobalData.FilterHandle);
    FLT_ASSERT(NT_SUCCESS(status));
    if (!NT_SUCCESS(status))
    {
        ExDeleteNPagedLookasideList(&GlobalData.Pre2PostContextList);
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

        ExDeleteNPagedLookasideList(&GlobalData.Pre2PostContextList);
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

        ExDeleteNPagedLookasideList(&GlobalData.Pre2PostContextList);
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

        ExDeleteNPagedLookasideList(&GlobalData.Pre2PostContextList);
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

        ExDeleteNPagedLookasideList(&GlobalData.Pre2PostContextList);
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

        ExDeleteNPagedLookasideList(&GlobalData.Pre2PostContextList);
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
    UNREFERENCED_PARAMETER(aFlags);
    UNREFERENCED_PARAMETER(aVolumeDeviceType);

    NTSTATUS status = STATUS_SUCCESS;
    PAA_VOLUME_CONTEXT volumeContext = NULL;
    UCHAR volPropBuffer[sizeof(FLT_VOLUME_PROPERTIES) + 512];
    PFLT_VOLUME_PROPERTIES volProp = (PFLT_VOLUME_PROPERTIES)volPropBuffer;
    PDEVICE_OBJECT deviceObject = NULL;

    try
    {
        // Check if file filesystem is supported
        if (!AA_SUPPORTS_FILE_SYSTEM_TYPES(aVolumeFilesystemType))
        {
            // Unsupported file system type
            status = STATUS_FLT_DO_NOT_ATTACH;
            leave;
        }

        // Get the disk device object and make sure it is a disk device type and does not
        // have any of the device characteristics we do not support
        status = FltGetDiskDeviceObject(aFltObjects->Volume,
            &deviceObject);
        if (!NT_SUCCESS(status))
        {
            // This is not disk device. We does no support it
            status = STATUS_FLT_DO_NOT_ATTACH;
            leave;
        }

        // Check device type
        if (deviceObject->DeviceType != FILE_DEVICE_DISK)
        {
            // Not supported
            status = STATUS_FLT_DO_NOT_ATTACH;
            leave;
        }

        if (FlagOn(deviceObject->Characteristics, FILE_FLOPPY_DISKETTE))
        {
            // Not supported
            status = STATUS_FLT_DO_NOT_ATTACH;
            leave;
        }

        if (aVolumeDeviceType != FILE_DEVICE_DISK_FILE_SYSTEM)
        {
            // Not supported
            status = STATUS_FLT_DO_NOT_ATTACH;
            leave;
        }

        // Check if context not created for this volume earlier
        status = FltGetVolumeContext(aFltObjects->Filter,
            aFltObjects->Volume,
            &volumeContext);
        if (NT_SUCCESS(status))
        {
            // Do nothing. Volume context was created earlier
            leave;
        }

        //
        // The volume context was not found. We need to create it
        //

        //
        // Allocate a volume context structure
        //
        status = FltAllocateContext(aFltObjects->Filter,
            FLT_VOLUME_CONTEXT,
            sizeof(AA_VOLUME_CONTEXT),
            NonPagedPool,
            &volumeContext);
        if (!NT_SUCCESS(status))
        {
            //
            // We could not allocate a context, quit now
            //
            status = STATUS_FLT_DO_NOT_ATTACH;
            leave;
        }

        //
        // Always get the volume properties, so I can get a sector size
        //
        ULONG resultLen = 0LU;
        status = FltGetVolumeProperties(aFltObjects->Volume,
            volProp,
            sizeof(volPropBuffer),
            &resultLen);
        if (!NT_SUCCESS(status))
        {
            status = STATUS_FLT_DO_NOT_ATTACH;
            leave;
        }

        //
        // Save the sector size in the context for later use.  Note that
        // we will pick a minimum sector size if a sector size is not
        // specified.
        //

        FLT_ASSERT((volProp->SectorSize == 0) || (volProp->SectorSize >= AA_MIN_SECTOR_SIZE));

        volumeContext->SectorSize = max(volProp->SectorSize, AA_MIN_SECTOR_SIZE);

        //
        // Init the buffer field (which may be allocated later).
        //
        volumeContext->Name.Buffer = NULL;

        //
        // Try and get the DOS name. If it succeeds we will have
        // an allocated name buffer. If not, it will be NULL
        //
        status = IoVolumeDeviceToDosName(deviceObject, &volumeContext->Name);
        if (!NT_SUCCESS(status))
        {
            // Unable to get device name
            status = STATUS_FLT_DO_NOT_ATTACH;
            leave;
        }

        // Set context
        status = FltSetVolumeContext(aFltObjects->Volume,
            FLT_SET_CONTEXT_KEEP_IF_EXISTS,
            volumeContext,
            NULL);
        if (!NT_SUCCESS(status))
        {
            // It is OK for the context to already be defined
            if (status == STATUS_FLT_CONTEXT_ALREADY_DEFINED)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = STATUS_FLT_DO_NOT_ATTACH;
            }
        }
    }
    finally
    {
        //
        // Always release the context.  If the set failed, it will free the
        // context.  If not, it will remove the reference added by the set.
        // Note that the name buffer in the ctx will get freed by the context
        // cleanup routine.
        //

        if (volumeContext)
        {
            FltReleaseContext(volumeContext);
            volumeContext = NULL;
        }

        //
        // Remove the reference added to the device object by
        // FltGetDiskDeviceObject.
        //
        if (deviceObject != NULL)
        {
            ObDereferenceObject(deviceObject);
        }
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

    // TODO: Need to free all volume context

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

    // Delete lookaside list
    ExDeleteNPagedLookasideList(&GlobalData.Pre2PostContextList);

    return STATUS_SUCCESS;
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

    // Check volume context
    PFLT_CONTEXT volContext = NULL;
    NTSTATUS status = FltGetVolumeContext(aFltObjects->Filter,
        aFltObjects->Volume,
        &volContext);
    if (!NT_SUCCESS(status))
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (volContext != NULL)
    {
        FltReleaseContext(volContext);
        volContext = NULL;
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

    // After creation, skip it if it is directory
    BOOLEAN isDir = FALSE;
    status = FltIsDirectory(aFltObjects->FileObject,
        aFltObjects->Instance,
        &isDir);
    // If FltIsDirectory failed, we do not know if it is a directoy,
    // we let it go through because if it is a directory, it will fail
    // at section creation anyway.
    if (!NT_SUCCESS(status) && isDir)
    {
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    // We skip the encrypted file open without FILE_WRITE_DATA and FILE_READ_DATA
    // This is because if application calls OpenEncryptedFileRaw(...) for backup, 
    // it won't have to decrypt the file. In such case, if we scan it, we will hit 
    // an assertion error in NTFS because it does not have the encryption context.
    // Thus, we have to skip the encrypted file not open for read/write.
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

    // Skip the alternate data stream
    if (AA_IsStreamAlternate(aData))
    {
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    // Check volume context
    PFLT_CONTEXT volContext = NULL;
    status = FltGetVolumeContext(aFltObjects->Filter,
        aFltObjects->Volume,
        &volContext);
    if (!NT_SUCCESS(status))
    {
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    if (volContext != NULL)
    {
        FltReleaseContext(volContext);
        volContext = NULL;
    }

    //
    // Get FileID and send it to usermode
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

    // Get opened file ID
    AA_FILE_ID_INFO fileId = { 0 };
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

    // Send file ID to user-mode
    LIFE_EXPORT_CREATE_NOTIFICATION_REQUEST request = { 0 };
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
    if (!NT_SUCCESS(status) || status == STATUS_TIMEOUT)
    {
        if (status == STATUS_PORT_DISCONNECTED)
        {
            FltCloseClientPort(GlobalData.FilterHandle, &GlobalData.ClientPortCreate);
            GlobalData.ClientPortCreate = NULL;
        }

        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    // Process  user-mode responce
    if (response.ConnectionResult == CREATE_RESULT_EXPORT_FILE)
    {
        // Check if file context exists
        PAA_FILE_CONTEXT fileContext = NULL;
        status = FltGetFileContext(aData->Iopb->TargetInstance,
            aData->Iopb->TargetFileObject,
            (PFLT_CONTEXT*)&fileContext);
        if (status == STATUS_NOT_FOUND)
        {
            // Create new File Context and attach it to this file

            status = AA_CreateFileContext(&fileContext);
            if (!NT_SUCCESS(status))
            {
                // Unable to create file context
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
                fileContext = NULL;
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

    // Check volume context
    PFLT_CONTEXT volContext = NULL;
    NTSTATUS status = FltGetVolumeContext(aFltObjects->Filter,
        aFltObjects->Volume,
        &volContext);
    if (!NT_SUCCESS(status))
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (volContext != NULL)
    {
        FltReleaseContext(volContext);
        volContext = NULL;
    }

    // Check file context and release it if it is present
    PAA_FILE_CONTEXT fileContext = NULL;
    status = FltGetFileContext(aFltObjects->Instance,
        aFltObjects->FileObject,
        (PFLT_CONTEXT*)&fileContext);
    if (NT_SUCCESS(status))
    {
        FltReleaseContext(fileContext);
        fileContext = NULL;
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
    FLT_PREOP_CALLBACK_STATUS retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
    NTSTATUS status = STATUS_SUCCESS;
    PAA_VOLUME_CONTEXT volumeContext = NULL;
    ULONG readLen = aData->Iopb->Parameters.Read.Length;
    PVOID newBuffer = NULL;
    PMDL newMdl = NULL;
    PAA_PRE_2_POST_READ_CONTEXT p2pContext = NULL;
    PAA_FILE_CONTEXT fileContext = NULL;


    try
    {
        if (aData->Iopb->Parameters.Read.Length == 0)
        {
            leave;
        }

        if (GlobalData.ServerPortRead == NULL)
        {
            leave;
        }

        if (GlobalData.ClientPortRead == NULL)
        {
            leave;
        }

        status = FltGetVolumeContext(aFltObjects->Filter, aFltObjects->Volume, &volumeContext);
        if (!NT_SUCCESS(status))
        {
            leave;
        }

        status = FltGetFileContext(aData->Iopb->TargetInstance,
            aData->Iopb->TargetFileObject,
            (PFLT_CONTEXT*)&fileContext);
        if (!NT_SUCCESS(status))
        {
            // It is not life exported file
            leave;
        }

        if (FlagOn(IRP_NOCACHE, aData->Iopb->IrpFlags))
        {
            readLen = (ULONG)ROUND_TO_SIZE(readLen, volumeContext->SectorSize);
        }

        // TODO: Need to send the message to user-mode

        // Create request
        LIFE_EXPORT_READ_NOTIFICATION_REQUEST request = { 0 };
        RtlCopyMemory(&request.FileId, &fileContext->FileID, sizeof(fileContext->FileID));
        request.BlockFileOffset = aData->Iopb->Parameters.Read.ByteOffset.QuadPart;
        request.BlockLength = readLen;
        request.RequestType = READ_NOTIFICATION_PRE_READ_TYPE;

        // Create responce
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
            }

            leave;
        }

        switch (response.ReadResultAction)
        {
            case SUCCESS_WITH_NO_POST_CALLBACK:
            {
                retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
                break;
            }
            case SUCCESS_WITH_POST_CALLBACK:
            {
                newBuffer = FltAllocatePoolAlignedWithTag(aFltObjects->Instance,
                    NonPagedPool,
                    (SIZE_T)readLen,
                    'buff');
                if (newBuffer == NULL)
                {
                    leave;
                }

                RtlZeroMemory(newBuffer, readLen);

                if (FlagOn(aData->Flags, FLTFL_CALLBACK_DATA_IRP_OPERATION))
                {
                    newMdl = IoAllocateMdl(newBuffer, readLen, FALSE, FALSE, NULL);
                    if (newMdl == NULL)
                    {
                        leave;
                    }

                    MmBuildMdlForNonPagedPool(newMdl);

                }

                p2pContext = ExAllocateFromNPagedLookasideList(&GlobalData.Pre2PostContextList);
                if (p2pContext == NULL)
                {
                    leave;
                }

                aData->Iopb->Parameters.Read.ReadBuffer = newBuffer;
                aData->Iopb->Parameters.Read.MdlAddress = newMdl;
                FltSetCallbackDataDirty(aData);

                p2pContext->Buffer.BufferPtr = newBuffer;
                p2pContext->Buffer.BufferSize = readLen;

                p2pContext->VolumeContext = volumeContext;

                *aCompletionContext = p2pContext;

                retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;

                break;
            }
            case COMPLETE_WITH_NO_POST_CALLBACK:
            {
                retValue = FLT_PREOP_COMPLETE;
                break;
            }
            default:
            {
                retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
            }
        }
    }
    finally
    {
        if (fileContext != NULL)
        {
            FltReleaseContext(fileContext);
            fileContext = NULL;
        }

        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK)
        {
            if (newBuffer != NULL)
            {
                FltFreePoolAlignedWithTag(aFltObjects->Instance,
                        newBuffer,
                        'buff');
                newBuffer = NULL;
            }

            if (newMdl != NULL)
            {
                IoFreeMdl(newMdl);
            }

            if (volumeContext != NULL)
            {
                FltReferenceContext(volumeContext);
                volumeContext = NULL;
            }
        }
    }

    /*
    // Skip IRP_PAGING_IO, IRP_SYNCHRONOUS_PAGING_IO and TopLevelIrp
    if ((aData->Iopb->IrpFlags & IRP_PAGING_IO) ||
        (aData->Iopb->IrpFlags & IRP_SYNCHRONOUS_PAGING_IO) ||
        IoGetTopLevelIrp())
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    // Since Fast I/O operations cannot be queued, we could return 
    // FLT_PREOP_SUCCESS_NO_CALLBACK at this point.
    if (!FLT_IS_IRP_OPERATION(aData))
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (GlobalData.ServerPortRead == NULL)
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    if (GlobalData.ClientPortRead == NULL)
    {
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    }

    PAA_VOLUME_CONTEXT volContext = NULL;
    PAA_FILE_CONTEXT fileContext = NULL;
    try
    {
        //
        // If they are trying to read ZERO bytes, then don't do anything and
        // we don't need a post-operation callback.
        //

        PFLT_IO_PARAMETER_BLOCK iopb = aData->Iopb;
        ULONG readLen = iopb->Parameters.Read.Length;
        if (readLen == 0)
        {
            leave;
        }

        status = FltGetVolumeContext(aFltObjects->Filter,
            aFltObjects->Volume,
            (PFLT_CONTEXT*)&volContext);
        if (!NT_SUCCESS(status))
        {
            leave;
        }

        if (FlagOn(IRP_NOCACHE, iopb->IrpFlags))
        {
            readLen = (ULONG)ROUND_TO_SIZE(readLen, volContext->SectorSize);
        }

        status = FltGetFileContext(aData->Iopb->TargetInstance,
            aData->Iopb->TargetFileObject,
            (PFLT_CONTEXT*)&fileContext);
        if (!NT_SUCCESS(status))
        {
            // It is not life exported file
            leave;
        }

        // Create request
        LIFE_EXPORT_READ_NOTIFICATION_REQUEST request;
        RtlZeroMemory(&request, sizeof(request));
        RtlCopyMemory(&request.FileId, &fileContext->FileID, sizeof(fileContext->FileID));
        request.BlockFileOffset = aData->Iopb->Parameters.Read.ByteOffset.QuadPart;
        request.BlockLength = aData->Iopb->Parameters.Read.Length;
        request.RequestType = READ_NOTIFICATION_PRE_READ_TYPE;

        // Create response
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
            }

            leave;
        }

        switch (response.ReadResultAction)
        {
            case SUCCESS_WITH_NO_POST_CALLBACK:
            {
                retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
                break;
            }
            case SUCCESS_WITH_POST_CALLBACK:
            {
                retValue = FLT_PREOP_SUCCESS_WITH_CALLBACK;
                break;
            }
            case COMPLETE_WITH_NO_POST_CALLBACK:
            {
                retValue = FLT_PREOP_COMPLETE;
                break;
            }
            default:
            {
                retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
            }
        }

        if (retValue != FLT_PREOP_SUCCESS_WITH_CALLBACK)
        {
            leave;
        }

        if (response.UserBuffer.BufferPtr == NULL ||
            response.UserBuffer.BufferSize == 0LU)
        {
            retValue = FLT_PREOP_SUCCESS_NO_CALLBACK;
            leave;
        }

        PAA_PRE_2_POST_READ_CONTEXT p2pContext = NULL;
        p2pContext = (PAA_PRE_2_POST_READ_CONTEXT)ExAllocateFromNPagedLookasideList(&GlobalData.Pre2PostContextList);
        if (p2pContext == NULL)
        {
            leave;
        }

        // Set user-mode buffer pointer
        p2pContext->Buffer.BufferPtr = response.UserBuffer.BufferPtr;
        p2pContext->Buffer.BufferSize = response.UserBuffer.BufferSize;
        p2pContext->Buffer.CurrentProcessId = response.UserBuffer.CurrentProcessId;

        p2pContext->VolumeContext = volContext;
        FltReferenceContext(volContext);

        *aCompletionContext = p2pContext;
    }
    finally
    {
        if (fileContext != NULL)
        {
            FltReleaseContext(fileContext);
            fileContext = NULL;
        }

        if (volContext != NULL)
        {
            FltReleaseContext(volContext);
            volContext = NULL;
        }
    }*/

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS
AA_PostRead(
    _Inout_     PFLT_CALLBACK_DATA       aData,
    _In_        PCFLT_RELATED_OBJECTS    aFltObjects,
    _In_opt_    PVOID                    aCompletionContext,
    _In_        FLT_POST_OPERATION_FLAGS aFlags
)
/*++
 
Routine Description:
 
    Handle post-read.
    
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
    PVOID originBuffer = NULL;
    PFLT_IO_PARAMETER_BLOCK iopb = aData->Iopb;
    FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PAA_PRE_2_POST_READ_CONTEXT p2pContext = aCompletionContext;
    BOOLEAN cleantupAllocateBuffer = TRUE;

    FLT_ASSERT(!FlagOn(aFlags, FLTFL_POST_OPERATION_DRAINING));

    try
    {
        if (p2pContext == NULL)
        {
            leave;
        }

        if (!NT_SUCCESS(aData->IoStatus.Status) ||
            (aData->IoStatus.Information == 0))
        {
            leave;
        }

        if (iopb->Parameters.Read.MdlAddress != NULL)
        {
            FLT_ASSERT(((PMDL)iopb->Parameters.Read.MdlAddress)->Next == NULL);

            originBuffer = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress,
                    NormalPagePriority | MdlMappingNoExecute);
            if (originBuffer == NULL)
            {
                aData->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                aData->IoStatus.Information = 0;

                leave;
            }
        }
        else if (FlagOn(aData->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
                 FlagOn(aData->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
        {
            originBuffer = iopb->Parameters.Read.ReadBuffer;
        }
        else
        {
            if (FltDoCompletionProcessingWhenSafe(aData,
                        aFltObjects,
                        aCompletionContext,
                        aFlags,
                        AA_ChangePostReadBuffersWhenSafe,
                        &retValue))
            {
                cleantupAllocateBuffer = FALSE;
            }
            else
            {
                aData->IoStatus.Status = STATUS_UNSUCCESSFUL;
                aData->IoStatus.Information = 0;
            }

            leave;
        }

        try
        {
            // TODO: Send message to user-mode
            // Create request
            LIFE_EXPORT_READ_NOTIFICATION_REQUEST request = { 0 };
            request.BlockFileOffset = iopb->Parameters.Read.ByteOffset.QuadPart;
            request.BlockLength = aData->IoStatus.Information;
            request.RequestType = READ_NOTIFICATION_POST_READ_TYPE;

            request.UserBuffer.BufferPtr = p2pContext->Buffer.BufferPtr;
            request.UserBuffer.BufferSize = p2pContext->Buffer.BufferSize;

            request.Status = aData->IoStatus.Status;

            // Create response
            LIFE_EXPORT_READ_NOTIFICATION_RESPONSE response = { 0 };
            ULONG responseSize = sizeof(response);

            NTSTATUS status = FltSendMessage(GlobalData.FilterHandle,
                &GlobalData.ClientPortRead,
                &request,
                sizeof(request),
                &response,
                &responseSize,
                NULL);
            if (!NT_SUCCESS(status) || (status == STATUS_TIMEOUT))
            {
                if (status == STATUS_PORT_DISCONNECTED)
                {
                    FltCloseClientPort(GlobalData.FilterHandle, &GlobalData.ClientPortRead);
                    GlobalData.ClientPortRead = NULL;
                }
            }

            //memset(p2pContext->Buffer.BufferPtr, 'C', p2pContext->Buffer.BufferSize);

            RtlCopyMemory(originBuffer,
                    p2pContext->Buffer.BufferPtr,
                    aData->IoStatus.Information);
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            aData->IoStatus.Status = GetExceptionCode();
            aData->IoStatus.Information = 0;
        }


    }
    finally
    {
        if (cleantupAllocateBuffer == TRUE)
        {
            if (p2pContext->Buffer.BufferPtr != NULL)
            {
                FltFreePoolAlignedWithTag(aFltObjects->Instance,
                    p2pContext->Buffer.BufferPtr,
                    'buff');
                p2pContext->Buffer.BufferPtr = NULL;
            }

            FltReleaseContext(p2pContext->VolumeContext);

            if (p2pContext != NULL)
            {
                ExFreeToNPagedLookasideList(&GlobalData.Pre2PostContextList, p2pContext);
                p2pContext = NULL;
            }
        }
    }







    /*FLT_POSTOP_CALLBACK_STATUS retValue = FLT_POSTOP_FINISHED_PROCESSING;
    PAA_PRE_2_POST_READ_CONTEXT p2pContext = (PAA_PRE_2_POST_READ_CONTEXT)aCompletionContext;
    BOOLEAN cleanupAllocatedBuffer = TRUE;
    PVOID originBuf = NULL;
    PFLT_IO_PARAMETER_BLOCK iopb = aData->Iopb;
    NTSTATUS status = STATUS_SUCCESS;

    FLT_ASSERT(!FlagOn(aFlags, FLTFL_POST_OPERATION_DRAINING));

    PAA_FILE_CONTEXT fileContext = NULL;
    try
    {
        if (!FLT_IS_IRP_OPERATION(aData))
        {
            leave;
        }

        if (!NT_SUCCESS(aData->IoStatus.Status) ||
            aData->IoStatus.Information == 0)
        {
            leave;
        }

        //  Skip IRP_PAGING_IO, IRP_SYNCHRONOUS_PAGING_IO and TopLevelIrp
        if ((aData->Iopb->IrpFlags & IRP_PAGING_IO) ||
            (aData->Iopb->IrpFlags & IRP_SYNCHRONOUS_PAGING_IO) ||
            IoGetTopLevelIrp())
        {
            leave;
        }

        if (GlobalData.ServerPortRead == NULL)
        {
            leave;
        }

        if (GlobalData.ClientPortRead == NULL)
        {
            leave;
        }

        status = FltGetFileContext(aData->Iopb->TargetInstance,
            aData->Iopb->TargetFileObject,
            (PFLT_CONTEXT*)&fileContext);
        if (!NT_SUCCESS(status))
        {
            // It is not life exported file
            leave;
        }

        // Try get sustem buffer pointer
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/ifs/accessing-user-buffers-in-a-postoperation-callback-routine
        if (iopb->Parameters.Read.MdlAddress != NULL)
        {
            //
            //  This should be a simple MDL. We don't expect chained MDLs
            //  this high up the stack
            //
            FLT_ASSERT(((PMDL)iopb->Parameters.Read.MdlAddress)->Next == NULL);

            //
            //  Since there is a MDL defined for the original buffer, get a
            //  system address for it so we can copy the data back to it.
            //  We must do this because we don't know what thread context
            //  we are in.
            //
            originBuf = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress,
                NormalPagePriority | MdlMappingNoExecute);
            if (originBuf == NULL)
            {
                //
                //  If we failed to get a SYSTEM address, mark that the read
                //  failed and return.
                //
                aData->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                aData->IoStatus.Information = 0;

                leave;
            }
        }
        else if (FlagOn(aData->Flags, FLTFL_CALLBACK_DATA_SYSTEM_BUFFER) ||
            FlagOn(aData->Flags, FLTFL_CALLBACK_DATA_FAST_IO_OPERATION))
        {
            //
            // If this is a system buffer, just use the given address because
            // it is valid in all thread contexts.
            // If this is a FASTIO operation, we can just use the
            // buffer (inside a try/except) since we know we are in
            // the correct thread context (you can't pend FASTIO's).
            //

            originBuf = iopb->Parameters.Read.ReadBuffer;
        }
        else
        {
            //
            // They don't have a MDL and this is not a system buffer
            // or a fastio so this is probably some arbitrary user
            // buffer.  We can not do the processing at DPC level so
            // try and get to a safe IRQL so we can do the processing.
            //

            if (FltDoCompletionProcessingWhenSafe(aData,
                aFltObjects,
                aCompletionContext,
                aFlags,
                AA_ChangePostReadBuffersWhenSafe,
                &retValue))
            {
                //
                // This operation has been moved to a safe IRQL, the called
                // routine will do (or has done) the freeing so don't do it
                // in our routine.
                //

                cleanupAllocatedBuffer = FALSE;
            }
            else
            {
                //
                // We are in a state where we can not get to a safe IRQL and
                // we do not have a MDL.  There is nothing we can do to safely
                // copy the data back to the users buffer, fail the operation
                // and return.  This shouldn't ever happen because in those
                // situations where it is not safe to post, we should have
                // a MDL.
                //

                aData->IoStatus.Status = STATUS_UNSUCCESSFUL;
                aData->IoStatus.Information = 0;
            }

            leave;
        }

        //
        // We either have a system buffer or this is a fastio operation
        // so we are in the proper context.  Copy the data handling an
        // exception.
        //

        try
        {
            if (p2pContext != NULL)
            {
                if (originBuf != NULL)
                {
                    ULONG copySize = (ULONG)aData->IoStatus.Information;
                    if (copySize > p2pContext->Buffer.BufferSize)
                    {
                         copySize = p2pContext->Buffer.BufferSize;
                    }

                    status = AA_CopyUserModeBufferToKernelByProcessId(
                        p2pContext->Buffer.CurrentProcessId,
                        originBuf,
                        p2pContext->Buffer.BufferPtr,
                        copySize);
                    if (NT_SUCCESS(status))
                    {
                        // Copy buffers error
                    }
                }
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // The copy failed, return an error, failing the operation.
            //

            aData->IoStatus.Status = GetExceptionCode();
            aData->IoStatus.Information = 0;
        }
    }
    finally
    {
        if (cleanupAllocatedBuffer)
        {
            //
            // Sent result to user-mode
            //

            // Create request with results and user-buffer pointer
            LIFE_EXPORT_READ_NOTIFICATION_REQUEST request;
            RtlZeroMemory(&request, sizeof(request));

            if (fileContext != NULL)
            {
                RtlCopyMemory(&request.FileId, &fileContext->FileID, sizeof(fileContext->FileID));
            }

            request.BlockFileOffset = iopb->Parameters.Read.ByteOffset.QuadPart;
            request.BlockLength = iopb->Parameters.Read.Length;
            request.RequestType = READ_NOTIFICATION_POST_READ_TYPE;

            if (p2pContext != NULL)
            {
                request.UserBuffer.BufferPtr = p2pContext->Buffer.BufferPtr;
                request.UserBuffer.BufferSize = p2pContext->Buffer.BufferSize;
            }

            request.Status = aData->IoStatus.Status;

            // Response
            LIFE_EXPORT_READ_NOTIFICATION_RESPONSE response = { 0 };
            ULONG responseSize = sizeof(response);

            status = FltSendMessage(GlobalData.FilterHandle,
                &GlobalData.ClientPortRead,
                &request,
                sizeof(request),
                &response,
                &responseSize,
                NULL);
            if (!NT_SUCCESS(status) || status == STATUS_TIMEOUT)
            {
                if (status == STATUS_PORT_DISCONNECTED)
                {
                    FltCloseClientPort(GlobalData.FilterHandle, &GlobalData.ClientPortRead);
                    GlobalData.ClientPortRead = NULL;
                }
            }

            if (p2pContext != NULL)
            {
                FltReleaseContext(p2pContext->VolumeContext);

                ExFreeToNPagedLookasideList(&GlobalData.Pre2PostContextList, p2pContext);
            }
        }

        if (fileContext != NULL)
        {
            FltReleaseContext(fileContext);
            fileContext = NULL;
        }
    }*/

    return retValue;
}


FLT_POSTOP_CALLBACK_STATUS
AA_ChangePostReadBuffersWhenSafe(
    _Inout_ PFLT_CALLBACK_DATA       aData,
    _In_    PCFLT_RELATED_OBJECTS    aFltObjects,
    _In_    PVOID                    aCompletionContext,
    _In_    FLT_POST_OPERATION_FLAGS aFlags
)
{
    PFLT_IO_PARAMETER_BLOCK iopb = aData->Iopb;
    PAA_PRE_2_POST_READ_CONTEXT p2pContext = aCompletionContext;
    PVOID originBuffer = NULL;

    UNREFERENCED_PARAMETER(aFltObjects);
    UNREFERENCED_PARAMETER(aFlags);

    FLT_ASSERT(aData->IoStatus.Information != 0);

    NTSTATUS status = FltLockUserBuffer(aData);
    if (!NT_SUCCESS(status))
    {
        aData->IoStatus.Status = status;
        aData->IoStatus.Information = 0;
    }
    else
    {
        originBuffer = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress,
                NormalPagePriority | MdlMappingNoExecute);
        if (originBuffer == NULL)
        {
            aData->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            aData->IoStatus.Information = 0;
        }
        else
        {
            memset(p2pContext->Buffer.BufferPtr, 'F', p2pContext->Buffer.BufferSize);

            RtlCopyMemory(originBuffer,
                    p2pContext->Buffer.BufferPtr,
                    aData->IoStatus.Information);
        }
    }

    FltFreePoolAlignedWithTag(aFltObjects->Instance,
            p2pContext->Buffer.BufferPtr,
            'buff');

    FltReleaseContext(p2pContext->VolumeContext);

    ExFreeToNPagedLookasideList(&GlobalData.Pre2PostContextList, p2pContext);

    return FLT_POSTOP_FINISHED_PROCESSING;



/*  
    UNREFERENCED_PARAMETER(aFltObjects);
    UNREFERENCED_PARAMETER(aFlags);

    if (aData == NULL)
    {
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    PFLT_IO_PARAMETER_BLOCK iopb = aData->Iopb;
    PAA_PRE_2_POST_READ_CONTEXT p2pContext = (PAA_PRE_2_POST_READ_CONTEXT)aCompletionContext;
    if (p2pContext == NULL)
    {
        return FLT_POSTOP_FINISHED_PROCESSING;
    }

    FLT_ASSERT(aData->IoStatus.Information != 0);

    //
    //  This is some sort of user buffer without a MDL, lock the user buffer
    //  so we can access it.  This will create a MDL for it.
    //
    NTSTATUS status = FltLockUserBuffer(aData);
    if (!NT_SUCCESS(status))
    {
        //
        // If we can't lock the buffer, fail the operation
        //
        aData->IoStatus.Status = status;
        aData->IoStatus.Information = 0;
    }
    else
    {
        //
        // Get a system address for this buffer
        //
        PVOID originBuffer = MmGetSystemAddressForMdlSafe(iopb->Parameters.Read.MdlAddress,
            NormalPagePriority | MdlMappingNoExecute);
        if (originBuffer == NULL)
        {
            //
            // If we couldn't get a SYSTEM buffer address, fail the operation
            //
            aData->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            aData->IoStatus.Information = 0;
        }
        else
        {
            //
            // Copy the data to the original buffer.  Note that we
            // don't need a try/except because we will always have a system
            // buffer address.
            //
            if (p2pContext != NULL && originBuffer != NULL)
            {
                ULONG copySize = (ULONG)aData->IoStatus.Information;
                if (copySize > p2pContext->Buffer.BufferSize)
                {
                     copySize = p2pContext->Buffer.BufferSize;
                }

                status = AA_CopyUserModeBufferToKernelByProcessId(
                    p2pContext->Buffer.CurrentProcessId,
                    originBuffer,
                    p2pContext->Buffer.BufferPtr,
                    copySize);
                if (NT_SUCCESS(status))
                {
                    // Copy buffers error
                }
            }
        }
    }

    //
    // Send result to user-mode
    //
    PAA_FILE_CONTEXT fileContext = NULL;
    status = FltGetFileContext(aData->Iopb->TargetInstance,
        aData->Iopb->TargetFileObject,
        (PFLT_CONTEXT*)&fileContext);
    if (!NT_SUCCESS(status))
    {
        // TODO: Need to process this case
    }

    LIFE_EXPORT_READ_NOTIFICATION_REQUEST request;
    RtlZeroMemory(&request, sizeof(request));
    RtlCopyMemory(&request.FileId, &fileContext->FileID, sizeof(fileContext->FileID));
    request.BlockFileOffset = iopb->Parameters.Read.ByteOffset.QuadPart;
    request.BlockLength = iopb->Parameters.Read.Length;
    request.RequestType = READ_NOTIFICATION_POST_READ_TYPE;

    if (p2pContext != NULL)
    {
        request.UserBuffer.BufferPtr = p2pContext->Buffer.BufferPtr;
        request.UserBuffer.BufferSize = p2pContext->Buffer.BufferSize;
    }

    request.Status = aData->IoStatus.Status;
    ULONG requestSize = sizeof(request);

    // Request
    LIFE_EXPORT_READ_NOTIFICATION_RESPONSE response = { 0 };
    ULONG responseSize = sizeof(response);

    status = FltSendMessage(GlobalData.FilterHandle,
        &GlobalData.ClientPortRead,
        &request,
        requestSize,
        &response,
        &responseSize,
        NULL);
    if (!NT_SUCCESS(status) || status == STATUS_TIMEOUT)
    {
        if (status == STATUS_PORT_DISCONNECTED)
        {
            FltCloseClientPort(GlobalData.FilterHandle, &GlobalData.ClientPortRead);
            GlobalData.ClientPortRead = NULL;
        }
    }

    if (fileContext != NULL)
    {
        FltReleaseContext(fileContext);
        fileContext = NULL;
    }

    if (p2pContext != NULL)
    {
        FltReleaseContext(p2pContext->VolumeContext);
        ExFreeToNPagedLookasideList(&GlobalData.Pre2PostContextList, p2pContext);
    }

    return FLT_POSTOP_FINISHED_PROCESSING;
*/
}


NTSTATUS
AA_CreateCommunicationPort(
    _In_ PSECURITY_DESCRIPTOR        aSecurityDescription,
    _In_ LIFE_EXPORT_CONNECTION_TYPE aConnectionType
)
{
    PAGED_CODE();

    if (aSecurityDescription == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Get port
    PCWSTR portName = NULL;
    PFLT_PORT *serverPort = NULL;
    switch (aConnectionType)
    {
        case LIFE_EXPORT_CREATE_CONNECTION_TYPE:
        {
            portName = AA_CREATE_PORT_NAME;
            serverPort = &GlobalData.ServerPortCreate;
            break;
        }
        case LIFE_EXPORT_READ_CONNECTION_TYPE:
        {
            portName = AA_READ_PORT_NAME;
            serverPort = &GlobalData.ServerPortRead;
            break;
        }
        case LIFE_EXPORT_CONTROL_CONNECTION_TYPE:
        {
            portName = AA_CONTROL_PORT_NAME;
            serverPort = &GlobalData.ServerPortControl;
            break;
        }
        default:
        {
            FLT_ASSERTMSG("No such connection type\n", FALSE);
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    UNICODE_STRING strPortName;
    RtlInitUnicodeString(&strPortName, portName);

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa,
        &strPortName,
        OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
        NULL,
        aSecurityDescription
    );

    NTSTATUS status = FltCreateCommunicationPort(GlobalData.FilterHandle,
        serverPort,
        &oa,
        NULL,
        AA_ConnectNotifyCallback,
        AA_DisconnectNotifyCallback,
        AA_MessageNotifyCallback,
        AA_MAX_PORT_CONNECTIONS);

    return status;
}


NTSTATUS
AA_CloseCommunicationPort(
    _In_ LIFE_EXPORT_CONNECTION_TYPE aConnectionType
)
{
    PFLT_PORT *serverPort = NULL;
    switch (aConnectionType)
    {
        case LIFE_EXPORT_CREATE_CONNECTION_TYPE:
        {
            serverPort = &GlobalData.ServerPortCreate;
            break;
        }
        case LIFE_EXPORT_READ_CONNECTION_TYPE:
        {
            serverPort = &GlobalData.ServerPortRead;
            break;
        }
        case LIFE_EXPORT_CONTROL_CONNECTION_TYPE:
        {
            serverPort = &GlobalData.ServerPortControl;
            break;
        }
        default:
        {
            FLT_ASSERTMSG("No such connection type\n", FALSE);
            return STATUS_INVALID_PARAMETER;
        }
    }

    if (serverPort == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    FltCloseCommunicationPort(*serverPort);
    serverPort = NULL;

    return STATUS_SUCCESS;
}







//
// Local function definition
//

NTSTATUS
AA_SendUnloadingMessageToUserMode(
    VOID
)
/*++

Routine Description:

    This routine sends unloading message to the user program.

Arguments:

    None

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

    /* PAA_FILE_CONTEXT fileContext = (PAA_FILE_CONTEXT)aContext; */
}


VOID
AA_VolumeContextCleanup(
    _In_ PFLT_CONTEXT     aContext,
    _In_ FLT_CONTEXT_TYPE aContextType
)
{
    PAA_VOLUME_CONTEXT volumeContext = aContext;

    PAGED_CODE();

    FLT_ASSERT(aContextType == FLT_VOLUME_CONTEXT);

    if (volumeContext->Name.Buffer != NULL)
    {
        ExFreePool(volumeContext->Name.Buffer);
        volumeContext->Name.Buffer = NULL;
    }
    UNREFERENCED_PARAMETER(aContextType);
}


NTSTATUS
AA_ConnectNotifyCallback(
    _In_                             PFLT_PORT aClientPort,
    _In_                             PVOID     aServerPortCookie,
    _In_reads_bytes_(aSizeOfContext) PVOID     aConnectionContext,
    _In_                             ULONG     aSizeOfContext,
    _Outptr_result_maybenull_        PVOID     *aConnectionCookie
)
{
    UNREFERENCED_PARAMETER(aClientPort);
    UNREFERENCED_PARAMETER(aServerPortCookie);
    UNREFERENCED_PARAMETER(aConnectionContext);
    UNREFERENCED_PARAMETER(aSizeOfContext);
    UNREFERENCED_PARAMETER(aConnectionCookie);

    PAGED_CODE();

    PLIFE_EXPORT_CONNECTION_CONTEXT connectionContext = (PLIFE_EXPORT_CONNECTION_CONTEXT)aConnectionContext;
    if (connectionContext == NULL)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    PLIFE_EXPORT_CONNECTION_TYPE connectionCookie = ExAllocatePoolWithTag(PagedPool,
        sizeof(LIFE_EXPORT_CONNECTION_TYPE),
        AA_LIFE_EXPORT_CONNECTION_CONTEXT_TAG);
    if (connectionCookie == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *connectionCookie = connectionContext->Type;
    switch (connectionContext->Type)
    {
        case LIFE_EXPORT_CREATE_CONNECTION_TYPE:
        {
            GlobalData.ClientPortCreate = aClientPort;
            *aConnectionCookie = connectionCookie;
            break;
        }
        case LIFE_EXPORT_READ_CONNECTION_TYPE:
        {
            GlobalData.ClientPortRead = aClientPort;
            *aConnectionCookie = connectionCookie;
            break;
        }
        case LIFE_EXPORT_CONTROL_CONNECTION_TYPE:
        {
            GlobalData.ClientPortControl = aClientPort;
            *aConnectionCookie = connectionCookie;
            break;
        }
        default:
        {
            ExFreePoolWithTag(connectionCookie, AA_LIFE_EXPORT_CONNECTION_CONTEXT_TAG);
            *aConnectionCookie = NULL;
            return STATUS_INVALID_PARAMETER_3;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
AA_MessageNotifyCallback(
    _In_                                                                     PVOID  aConnectionCoockie,
    _In_reads_bytes_opt_(aInputBufferSize)                                   PVOID  aInputBuffer,
    _In_                                                                     ULONG  aInputBufferSize,
    _Out_writes_bytes_to_opt_(aOutputBufferSize, *aReturnOutputBufferLength) PVOID  aOutputBuffer,
    _In_                                                                     ULONG  aOutputBufferSize,
    _Out_                                                                    PULONG aReturnOutputBufferLength
)
{
    UNREFERENCED_PARAMETER(aConnectionCoockie);
    UNREFERENCED_PARAMETER(aInputBuffer);
    UNREFERENCED_PARAMETER(aInputBufferSize);
    UNREFERENCED_PARAMETER(aOutputBuffer);
    UNREFERENCED_PARAMETER(aOutputBufferSize);
    UNREFERENCED_PARAMETER(aReturnOutputBufferLength);

    PAGED_CODE();

    if (aInputBuffer == NULL || aInputBufferSize == 0UL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}


VOID
AA_DisconnectNotifyCallback(
    _In_opt_ PVOID aConnectionCookie
)
{
    PAGED_CODE();

    PLIFE_EXPORT_CONNECTION_TYPE connectionType = (PLIFE_EXPORT_CONNECTION_TYPE)aConnectionCookie;
    if (connectionType == NULL)
    {
        return;
    }

    switch (*connectionType)
    {
        case LIFE_EXPORT_CREATE_CONNECTION_TYPE:
        {
            FltCloseClientPort(GlobalData.FilterHandle, &GlobalData.ClientPortCreate);
            GlobalData.ClientPortCreate = NULL;
            break;
        }
        case LIFE_EXPORT_READ_CONNECTION_TYPE:
        {
            FltCloseClientPort(GlobalData.FilterHandle, &GlobalData.ClientPortRead);
            GlobalData.ClientPortRead = NULL;
            break;
        }
        case LIFE_EXPORT_CONTROL_CONNECTION_TYPE:
        {
            FltCloseClientPort(GlobalData.FilterHandle, &GlobalData.ClientPortControl);
            GlobalData.ClientPortControl = NULL;
            break;
        }
        default:
        {
            return;
        }
    }

    ExFreePoolWithTag(connectionType, AA_LIFE_EXPORT_CONNECTION_CONTEXT_TAG);
    connectionType = NULL;

    return;
}


NTSTATUS
AA_CopyUserModeBufferToKernelByProcessId(
    ULONG aCurrentProcessId,
    PVOID aDestinationKernelBufferPrt,
    PVOID aSourceUserModeBufferPtr,
    ULONG aCopySize
)
{
    // TODO: Need to check num,bers of parameters when parameter is incorrent
    
    if (aCurrentProcessId == 0LU)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (aSourceUserModeBufferPtr == NULL)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (aDestinationKernelBufferPrt == NULL)
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    if (aCopySize == 0LU)
    {
        return STATUS_INVALID_PARAMETER_4;
    }

    PEPROCESS workingProcess = NULL;
    NTSTATUS status = PsLookupProcessByProcessId((HANDLE)aCurrentProcessId, &workingProcess);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    KAPC_STATE state = { 0 };
    PVOID tempBuffer = NULL;

    try
    {
        // Allocate tempBuffer
        tempBuffer = ExAllocatePoolWithTag(NonPagedPool, aCopySize, 'temp');
        if (tempBuffer == NULL)
        {
            if (workingProcess != NULL)
            {
                FltObjectDereference(workingProcess);
                workingProcess = NULL;
            }

            return STATUS_MEMORY_NOT_ALLOCATED;
        }

        RtlZeroMemory(tempBuffer, aCopySize);

        // Copy origin driver data to the temp buffer
        RtlCopyMemory(tempBuffer, aDestinationKernelBufferPrt, aCopySize);


        KeStackAttachProcess(workingProcess, &state);

        // Copy user-mode buffer to driver buffer
        RtlCopyMemory(aDestinationKernelBufferPrt, aSourceUserModeBufferPtr, aCopySize);

        // Copy temp buffer to the user-mode buffer
        RtlCopyMemory(aSourceUserModeBufferPtr, tempBuffer, aCopySize);

        KeUnstackDetachProcess(&state);

        if (workingProcess != NULL)
        {
            FltObjectDereference(workingProcess);
            workingProcess = NULL;
        }

        // Free temp buffer
        if (tempBuffer != NULL)
        {
            ExFreePoolWithTag(tempBuffer, 'temp');
            tempBuffer = NULL;
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        KeUnstackDetachProcess(&state);

        if (workingProcess != NULL)
        {
            FltObjectDereference(workingProcess);
            workingProcess = NULL;
        }

        // Free usermode buffer
        if (tempBuffer != NULL)
        {
            ExFreePoolWithTag(tempBuffer, 'temp');
            tempBuffer = NULL;
        }
    }

    return status;
}

