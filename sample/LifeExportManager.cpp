#include "LifeExportManager.h"
#include "FilterCommunicationPort.h"
#include "Communication.h"

#ifndef NDEBUG
#include <iostream>
#endif // !NDEBUG


namespace LifeExportManagement
{

    LifeExportManager::LifeExportManager()
        : m_loadedInStart(true)
    {
        ZeroMemory(&m_context, sizeof(m_context));
        m_context.CurrentManager = this;

        HRESULT res = ::FilterLoad(FILTER_DRIVER_NAME);
        if ((res == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS))          ||
            (res == HRESULT_FROM_WIN32(ERROR_SERVICE_ALREADY_RUNNING)) ||
            (res == HRESULT_FROM_WIN32(ERROR_PRIVILEGE_NOT_HELD)))
        {
            m_loadedInStart = false;
        }

        // start CONTROL thread
        HRESULT result = initLifeExportControlThread();
        if (SUCCEEDED(result))
        {
            startLifeExportControlThread();
        }
    }


    LifeExportManager::~LifeExportManager()
    {
        stop();

        // stop CONTROL thread
        stopLifeExportControlThread();
        freeLifeExportControlThread();

        if (m_loadedInStart)
        {
            ::FilterUnload(FILTER_DRIVER_NAME);
        }
    }


    HRESULT LifeExportManager::exec()
    {
        HRESULT res = initLifeExportManager();
        if (FAILED(res))
        {
            return res;
        }

        res = startLifeExportManager();
        if (FAILED(res))
        {
            res = freeLifeExportManager();
        }

        return res;
    }


    HRESULT LifeExportManager::stop()
    {
        HRESULT res = stopLifeExportManager();
        if (FAILED(res))
        {
            return res;
        }

        return freeLifeExportManager();
    }


    HRESULT LifeExportManager::initLifeExportManager()
    {
        m_context.NeedFinalize = false;

        HRESULT result = S_OK;
        m_context.CreateThreadHandle = ::CreateThread( NULL,
            0,
            (LPTHREAD_START_ROUTINE)LifeExportManager::createMsgHandlerFunc,
            &m_context,
            CREATE_SUSPENDED,
            NULL);
        if (m_context.CreateThreadHandle == NULL)
        {
            result = HRESULT_FROM_WIN32(::GetLastError());
            return result;
        }

        m_context.ReadThreadHandle = ::CreateThread( NULL,
            0,
            (LPTHREAD_START_ROUTINE)&LifeExportManager::readMsgHandlerFunc,
            &m_context,
            CREATE_SUSPENDED,
            NULL);
        if (m_context.ReadThreadHandle == NULL)
        {
            result = HRESULT_FROM_WIN32(::GetLastError());

            ::CloseHandle(m_context.CreateThreadHandle);
            m_context.CreateThreadHandle = NULL;

            return result;
        }

        return result;
    }


    HRESULT LifeExportManager::startLifeExportManager()
    {
        HRESULT result = S_OK;

        if (::ResumeThread(m_context.CreateThreadHandle) == -1)
        {
            result = HRESULT_FROM_WIN32(::GetLastError());
        }

        if (::ResumeThread(m_context.ReadThreadHandle) == -1)
        {
            result = HRESULT_FROM_WIN32(::GetLastError());
        }

        return result;
    }


    HRESULT LifeExportManager::stopLifeExportManager()
    {
        m_context.NeedFinalize = true;

        if (m_context.CreateThreadHandle == NULL)
        {
            return E_INVALIDARG;
        }

        HRESULT result = S_OK;
        // wait create thread
        if (!WaitForSingleObject(m_context.CreateThreadHandle, 500))
        {
            result = HRESULT_FROM_WIN32(::GetLastError());
            if (result != S_OK)
            {
                return result;
            }
        }

        if (m_context.ReadThreadHandle == NULL)
        {
            return E_INVALIDARG;
        }

        if (!WaitForSingleObject(m_context.ReadThreadHandle, 500))
        {
            result = HRESULT_FROM_WIN32(::GetLastError());
            if (result != S_OK)
            {
                return result;
            }
        }

        return result;
    }


    HRESULT LifeExportManager::freeLifeExportManager()
    {
        HRESULT result = S_OK;

        if (m_context.ReadThreadHandle != NULL)
        {
            if (::CloseHandle(m_context.ReadThreadHandle))
            {
                result = HRESULT_FROM_WIN32(::GetLastError());
            }

            m_context.ReadThreadHandle = NULL;
        }

        if (m_context.CreateThreadHandle != NULL)
        {
            if (!::CloseHandle(m_context.CreateThreadHandle))
            {
                result = HRESULT_FROM_WIN32(::GetLastError());
            }

            m_context.CreateThreadHandle = NULL;
        }

        return result;
    }


    HRESULT LifeExportManager::initLifeExportControlThread()
    {
        HRESULT result = S_OK;
        m_context.ControlThreadHandle = ::CreateThread(NULL,
            0,
            (LPTHREAD_START_ROUTINE)&LifeExportManager::controlMsgHandleFunc,
            &m_context,
            CREATE_SUSPENDED,
            NULL);
        if (m_context.ControlThreadHandle == NULL)
        {
            result = ::GetLastError();
        }

        return result;
    }


    HRESULT LifeExportManager::startLifeExportControlThread()
    {
        HRESULT result = S_OK;
        if (::ResumeThread(m_context.ControlThreadHandle) == -1)
        {
            result = HRESULT_FROM_WIN32(::GetLastError());
        }

        return result;
    }

    HRESULT LifeExportManager::stopLifeExportControlThread()
    {
        if (m_context.ControlThreadHandle == NULL)
        {
            return E_INVALIDARG;
        }

        HRESULT result = S_OK;
        // wait thread
        if (!::WaitForSingleObject(m_context.ControlThreadHandle, 500))
        {
            result = HRESULT_FROM_WIN32(::GetLastError());
        }

        return result;
    }


    HRESULT LifeExportManager::freeLifeExportControlThread()
    {
        HRESULT result = S_OK;
        if (m_context.ControlThreadHandle != NULL)
        {
            if (::CloseHandle(m_context.ControlThreadHandle))
            {
                result = HRESULT_FROM_WIN32(::GetLastError());
            }

            m_context.ControlThreadHandle = NULL;
        }

        return result;
    }




    ///////////////////////////////////////////////////////////////////
    //  THREADS FUNCTIONS
    ///////////////////////////////////////////////////////////////////


    HRESULT LifeExportManager::createMsgHandlerFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext)
    {
#ifndef NDEBUG
        std::wcout << L"Start CREATE Thread" << std::endl;
#endif // !NDEBUG

        if (aContext == NULL)
        {
            return E_INVALIDARG;
        }

        // Need to create communication port
        FilterCommunicationPort portCreate;
        LIFE_EXPORT_CONNECTION_CONTEXT context;
        context.Type = LIFE_EXPORT_CREATE_CONNECTION_TYPE;
        HRESULT res = portCreate.connect(std::wstring(AA_CREATE_PORT_NAME), 0, &context, sizeof(context));
        if (FAILED(res))
        {
            // Connect error
            return res;
        }

        if (!portCreate.isConnected())
        {
            // port is not connected
            return E_FAIL;
        }

        typedef struct _USER_LIFE_EXPORT_GET_CREATE_NOTIFICATION
        {
            FILTER_MESSAGE_HEADER Header;

            LIFE_EXPORT_CREATE_NOTIFICATION_REQUEST Notification;

        } USER_LIFE_EXPORT_GET_CREATE_NOTIFICATION, *PUSER_LIFE_EXPORT_GET_CREATE_NOTIFICATION;

        typedef struct _USER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION
        {
            FILTER_REPLY_HEADER ReplyHeader;

            LIFE_EXPORT_CREATE_NOTIFICATION_RESPONSE ResponseData;

        } USER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION, *PUSER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION;

        while (!aContext->NeedFinalize) // need to check atomic variable here
        {

            // get driver request
            USER_LIFE_EXPORT_GET_CREATE_NOTIFICATION requestMessage{ 0 };
            res = portCreate.getMessage(&requestMessage.Header, sizeof(requestMessage), NULL);
            if (res == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
            {
                // FilterGetMessage aborted
                break;
            }
            else if (FAILED(res))
            {
                // Failed to get message from driver
                break;
            }


            // TODO: Need to realize main function logic HERE


            USER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION replyMessage{ 0 };
            replyMessage.ReplyHeader.MessageId = requestMessage.Header.MessageId;
            replyMessage.ReplyHeader.Status = 0x0; // STATUS_SUCCESS;
            CopyMemory(&replyMessage.ResponseData.FileId, &requestMessage.Notification.FileId, sizeof(replyMessage.ResponseData.FileId));

#ifndef NDEBUG
            std::wcout << L"CREATE File id: " << std::hex << requestMessage.Notification.FileId.FileId_64.Value << std::endl;
#endif // !NDEBUG

            ULONGLONG testID = 0x0003000000009BD9;
            if (replyMessage.ResponseData.FileId.FileId_64.Value == testID)
            {
                replyMessage.ResponseData.ConnectionResult = CREATE_RESULT_EXPORT_FILE;
#ifndef NDEBUG
                std::wcout << L"THIS IS TEST FILE" << std::endl;
#endif // !NDEBUG
            }
            else
            {
                replyMessage.ResponseData.ConnectionResult = CREATE_RESULT_SKIP_FILE;
            }

            // send answer to the driver
            res = portCreate.replyMessage(&replyMessage.ReplyHeader, sizeof(replyMessage));
            if (FAILED(res))
            {
                // Failed to reply message to driver
                break;
            }
        }

        portCreate.disconnect();

#ifndef NDEBUG
        std::wcout << L"FINISH CREATE Thread" << std::endl;
#endif // !NDEBUG

        return res;
    }


    HRESULT LifeExportManager::readMsgHandlerFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext)
    {
#ifndef NDEBUG
        std::wcout << L"Start READ Thread" << std::endl;
#endif //!NDEBUG

        if (aContext == NULL)
        {
            return E_INVALIDARG;
        }

        FilterCommunicationPort portRead;
        LIFE_EXPORT_CONNECTION_CONTEXT context;
        context.Type = LIFE_EXPORT_READ_CONNECTION_TYPE;
        HRESULT res = portRead.connect(std::wstring(AA_READ_PORT_NAME), 0, &context, sizeof(context));
        if (FAILED(res))
        {
            return res;
        }

        if (!portRead.isConnected())
        {
            return E_FAIL;
        }

        typedef struct _USER_LIFE_EXPORT_GET_READ_NOTIFICATION
        {
            FILTER_MESSAGE_HEADER Header;

            LIFE_EXPORT_READ_NOTIFICATION_REQUEST Notification;
        } USER_LIFE_EXPORT_GET_READ_NOTIFICATION, *PUSER_LIFE_EXPORT_GET_READ_NOTIFICATION;

        typedef struct _USER_LIFE_EXPORT_REPLY_READ_NOTIFICATION
        {
            FILTER_REPLY_HEADER ReplyHeader;

            LIFE_EXPORT_READ_NOTIFICATION_RESPONSE ResponseData;
        } USER_LIFE_EXPORT_REPLY_READ_NOTIFICATION, *PUSER_LIFE_EXPORT_REPLY_READ_NOTIFICATION;

        while (!aContext->NeedFinalize)
        {
            USER_LIFE_EXPORT_GET_READ_NOTIFICATION requestMessage{ 0 };
            res = portRead.getMessage(&requestMessage.Header, sizeof(requestMessage), NULL);
            if (res == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
            {
                break;
            }
            else if (FAILED(res))
            {
                break;
            }


            // TODO: Need to realize main function logic HERE


            USER_LIFE_EXPORT_REPLY_READ_NOTIFICATION replyMessage{ 0 };
            replyMessage.ReplyHeader.MessageId = requestMessage.Header.MessageId;
            replyMessage.ReplyHeader.Status = 0x0; // STATUS_SUCCESS;
            CopyMemory(&replyMessage.ResponseData.FileId, &requestMessage.Notification.FileId, sizeof(replyMessage.ResponseData.FileId));

#ifndef NDEBUG
            std::wcout << L"READ File id: " << std::hex << requestMessage.Notification.FileId.FileId_64.Value;
            std::wcout << L" file offset: " << requestMessage.Notification.BlockFileOffset;
            std::wcout << L" block size: " << requestMessage.Notification.BlockLength << std::endl;
#endif // !NDEBUG

            replyMessage.ResponseData.ReadResult = READ_RESULT_WAIT_BLOCK;

            res = portRead.replyMessage(&replyMessage.ReplyHeader, sizeof(replyMessage));
            if (FAILED(res))
            {
                break;
            }
        }

        portRead.disconnect();

#ifndef NDEBUG
        std::wcout << L"Finish READ Thread" << std::endl;
#endif // !NDEBUG

        return res;
    }


    HRESULT LifeExportManager::controlMsgHandleFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext)
    {
#ifndef NDEBUG
        std::wcout << L"Start CONTROL Thread" << std::endl;
#endif //!NDEBUG

        if (aContext == NULL)
        {
            // Connect error
            return E_INVALIDARG;
        }

        if (aContext->CurrentManager == NULL)
        {
            // Invalid manager pointer
            return E_INVALIDARG;
        }

        FilterCommunicationPort portControl;
        LIFE_EXPORT_CONNECTION_CONTEXT context;
        context.Type = LIFE_EXPORT_CONTROL_CONNECTION_TYPE;
        HRESULT res = portControl.connect(std::wstring(AA_CONTROL_PORT_NAME), 0, &context, sizeof(context));
        if (FAILED(res))
        {
            return res;
        }

        if (!portControl.isConnected())
        {
            return E_FAIL;
        }

        typedef struct _USER_LIFE_EXPORT_GET_CONTROL_NOTIFICATION
        {
            FILTER_MESSAGE_HEADER                    Header;
            LIFE_EXPORT_CONTROL_NOTIFICATION_REQUEST Notification;
        } USER_LIFE_EXPORT_GET_CONTROL_NOTIFICATION, *PUSER_LIFE_EXPORT_GET_CONTROL_NOTIFICATION;

        typedef struct _USER_LIFE_EXPORT_REPLY_CONTROL_NOTIFICATION
        {
            FILTER_REPLY_HEADER                       ReplyHeader;
            LIFE_EXPORT_CONTROL_NOTIFICATION_RESPONSE ResponseData;
        } USER_LIFE_EXPORT_REPLY_CONTROL_NOTIFICATION, *PUSER_LIFE_EXPORT_REPLY_CONTROL_NOTIFICATION;

        while (true)
        {
            USER_LIFE_EXPORT_GET_CONTROL_NOTIFICATION requestMessage{ 0 };
            res = portControl.getMessage(&requestMessage.Header, sizeof(requestMessage), NULL);
            if (res == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
            {
                break;
            }
            else if (FAILED(res))
            {
                break;
            }

            HRESULT unloadResult = S_OK;
            switch (requestMessage.Notification.Message)
            {
                case  CONTROL_RESULT_UNLOADING:
                {
                    // Stop all working threads
                    unloadResult = aContext->CurrentManager->stop();
                    break;
                }
                default:
                {
                    /* UNKNOWN MESSAGE */
                    break;
                }
            }

            USER_LIFE_EXPORT_REPLY_CONTROL_NOTIFICATION replyMessage{ 0 };
            replyMessage.ReplyHeader.MessageId = requestMessage.Header.MessageId;
            replyMessage.ReplyHeader.Status = unloadResult;
            replyMessage.ResponseData.ConnectionResult = CONTROL_RESULT_UNLOAD;
            res = portControl.replyMessage(&replyMessage.ReplyHeader, sizeof(replyMessage));
            if (FAILED(res))
            {
                break;
            }
        }

        portControl.disconnect();

#ifndef NDEBUG
        std::wcout << L"Finish CONTROL Thread" << std::endl;
#endif //!NDEBUG

        return res;
    }


} // namespace LifeExportManagement

