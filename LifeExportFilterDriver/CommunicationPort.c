#include "CommunicationPort.h"


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


#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, AA_ConnectNotifyCallback)
    #pragma alloc_text(PAGE, AA_MessageNotifyCallback)
    #pragma alloc_text(PAGE, AA_DisconnectNotifyCallback)

    #pragma alloc_text(PAGE, AA_CreateCommunicationPort)
    #pragma alloc_text(PAGE, AA_CloseCommunicationPort)
#endif



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
        MAX_PORT_CONNECTIONS);

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



//**********************************************
//	Callbacks
//**********************************************

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
        default:
        {
            return;
        }
    }

    ExFreePoolWithTag(connectionType, AA_LIFE_EXPORT_CONNECTION_CONTEXT_TAG);
    connectionType = NULL;
}

