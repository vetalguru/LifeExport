#include <Windows.h>
#include <iostream>
#include <fltUser.h>

#include "Communication.h"
#include "FilterCommunicationPort.h"

#define LIFE_EXPORT_LIBRARY_NAME "LifeExportLibrary.dll"


int wmain(int argc, wchar_t* argv[])
{
    // Load library
    HINSTANCE dllInstance = ::LoadLibrary(LIFE_EXPORT_LIBRARY_NAME);
    if (dllInstance == NULL)
    {
        std::cerr << "Unable to load library" << std::endl;
        return -1;
    }
 
    std::wcout << "Library was loaded" << std::endl;

	FilterCommunicationPort portCreate;
	LIFE_EXPORT_CONNECTION_CONTEXT context;
	context.Type = LIFE_EXPORT_CREATE_CONNECTION_TYPE;
	HRESULT res = portCreate.connect(std::wstring(AA_CREATE_PORT_NAME), 0, &context, sizeof(LIFE_EXPORT_CONNECTION_CONTEXT));
	if (FAILED(res))
	{
		std::wcout << L"Connect error. Error code " << res << std::endl;
	}

	if (!portCreate.isConnected())
	{
		std::wcout << L"port is not connected" << std::endl;
		return -1;
	}


	ULONGLONG counter = 0ULL;
	for (;;)
	{
		typedef struct _USER_LIFE_EXPORT_GET_CREATE_NOTIFICATION
		{
			FILTER_MESSAGE_HEADER Header;

			LIFE_EXPORT_CREATE_NOTIFICATION_REQUEST Notification;

		} USER_LIFE_EXPORT_GET_CREATE_NOTIFICATION, *PUSER_LIFE_EXPORT_GET_CREATE_NOTIFICATION;

		USER_LIFE_EXPORT_GET_CREATE_NOTIFICATION sendMessage = { 0 };

		res = portCreate.getMessage(&sendMessage.Header, sizeof(USER_LIFE_EXPORT_GET_CREATE_NOTIFICATION), NULL);
		if (res == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
		{
			std::wcout << L"FilterGetMessage aborted" << std::endl;
			res = S_OK;
			break;
		}
		else if (FAILED(res))
		{
			std::wcout << L"Failed to get message from driver" << std::endl;
			break;
		}

		counter++;
		std::wcout << counter << L" | Opened FileId: " << std::hex << sendMessage.Notification.FileId.FileId_64.Value << std::endl;

		typedef struct _USER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION
		{
			FILTER_REPLY_HEADER ReplyHeader;

			LIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE ResponseData;

		} USER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION, *PUSER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION;

		USER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION replyMessage;
		ZeroMemory(&replyMessage, sizeof(USER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION));
		
		replyMessage.ReplyHeader.MessageId = sendMessage.Header.MessageId;
		replyMessage.ReplyHeader.Status = 0; // STATUS_SUCCESS

		CopyMemory(&replyMessage.ResponseData.FileId, &sendMessage.Notification.FileId, sizeof(AA_FILE_ID_INFO));
		replyMessage.ResponseData.ConnectionResult = CREATE_RESULT_SKIP_FILE;

		res = portCreate.replyMessage(&replyMessage.ReplyHeader, sizeof(USER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION));
		if (FAILED(res))
		{
			std::wcout << L"Failed to reply message to driver" << std::endl;
			break;
		}
	}

	portCreate.disconnect();

    // Unload library
    ::FreeLibrary(dllInstance);
    dllInstance = NULL;

    return 0;
}
