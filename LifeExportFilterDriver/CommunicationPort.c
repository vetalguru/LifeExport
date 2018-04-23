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

	return STATUS_SUCCESS;
}


VOID
AA_DisconnectNotifyCallback(
	_In_opt_ PVOID aConnectionCookie
)
{
	UNREFERENCED_PARAMETER(aConnectionCookie);

	PAGED_CODE();
}
