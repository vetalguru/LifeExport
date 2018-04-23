#ifndef _LIFE_EXPORT_FILTER_COMMUNICATION_PORT_H_
#define _LIFE_EXPORT_FILTER_COMMUNICATION_PORT_H_


#include <fltKernel.h>
#include "Globals.h"
#include "Communication.h"



#define MAX_PORT_CONNECTIONS 1


NTSTATUS
AA_CreateCommunicationPort(
	_In_ PSECURITY_DESCRIPTOR        aSecurityDescription,
	_In_ LIFE_EXPORT_CONNECTION_TYPE aConnectionType
);

NTSTATUS
AA_CloseCommunicationPort(
	_In_ LIFE_EXPORT_CONNECTION_TYPE aConnectionType
);

#endif // _LIFE_EXPORT_FILTER_COMMUNICATION_PORT_H_
