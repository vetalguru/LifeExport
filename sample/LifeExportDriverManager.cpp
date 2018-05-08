#include "LifeExportDriverManager.h"
#include "FilterCommunicationPort.h"
#include "Communication.h"



namespace LifeExportDriverManagement
{

    LifeExportDriverManager::LifeExportDriverManager(IDriverHandler* aDriverHandler)
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

        m_context.DriverHandler = aDriverHandler;

        // start CONTROL thread
        HRESULT result = initLifeExportControlThread();
        if (SUCCEEDED(result))
        {
            startLifeExportControlThread();
        }
    }


    LifeExportDriverManager::~LifeExportDriverManager()
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


    const HRESULT LifeExportDriverManager::exec()
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


    const HRESULT LifeExportDriverManager::stop()
    {
        HRESULT res = stopLifeExportManager();
        if (FAILED(res))
        {
            return res;
        }

        return freeLifeExportManager();
    }


    HRESULT LifeExportDriverManager::initLifeExportManager()
    {
        m_context.NeedFinalize = false;

        HRESULT result = S_OK;
        m_context.CreateThreadHandle = ::CreateThread( NULL,
            0,
            (LPTHREAD_START_ROUTINE)LifeExportDriverManager::createMsgHandlerFunc,
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
            (LPTHREAD_START_ROUTINE)&LifeExportDriverManager::readMsgHandlerFunc,
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


    HRESULT LifeExportDriverManager::startLifeExportManager()
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


    HRESULT LifeExportDriverManager::stopLifeExportManager()
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


    HRESULT LifeExportDriverManager::freeLifeExportManager()
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


    HRESULT LifeExportDriverManager::initLifeExportControlThread()
    {
        HRESULT result = S_OK;
        m_context.ControlThreadHandle = ::CreateThread(NULL,
            0,
            (LPTHREAD_START_ROUTINE)&LifeExportDriverManager::controlMsgHandleFunc,
            &m_context,
            CREATE_SUSPENDED,
            NULL);
        if (m_context.ControlThreadHandle == NULL)
        {
            result = ::GetLastError();
        }

        return result;
    }


    HRESULT LifeExportDriverManager::startLifeExportControlThread()
    {
        HRESULT result = S_OK;
        if (::ResumeThread(m_context.ControlThreadHandle) == -1)
        {
            result = HRESULT_FROM_WIN32(::GetLastError());
        }

        return result;
    }


    HRESULT LifeExportDriverManager::stopLifeExportControlThread()
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


    HRESULT LifeExportDriverManager::freeLifeExportControlThread()
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

    HRESULT LifeExportDriverManager::createMsgHandlerFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext)
    {
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

            LIFE_EXPORT_CONNECTION_RESULT callbackConnectionResult = CREATE_RESULT_SKIP_FILE;
            HRESULT callbackResult = S_OK;
            if (aContext && aContext->DriverHandler)
            {
                // Callback function
                callbackResult = aContext->DriverHandler->CreatingFileCallback(&requestMessage.Notification.FileId,
                    callbackConnectionResult);
            }

            USER_LIFE_EXPORT_REPLY_CREATE_NOTIFICATION replyMessage{ 0 };
            replyMessage.ReplyHeader.MessageId = requestMessage.Header.MessageId;
            replyMessage.ReplyHeader.Status = callbackResult;
            CopyMemory(&replyMessage.ResponseData.FileId, &requestMessage.Notification.FileId, sizeof(replyMessage.ResponseData.FileId));

            replyMessage.ResponseData.ConnectionResult = callbackConnectionResult;

            // send answer to the driver
            res = portCreate.replyMessage(&replyMessage.ReplyHeader, sizeof(replyMessage));
            if (FAILED(res))
            {
                // Failed to reply message to driver
                break;
            }
        }

        portCreate.disconnect();

        return res;
    }


    HRESULT LifeExportDriverManager::readMsgHandlerFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext)
    {
        if (aContext == NULL)
        {
            return E_INVALIDARG;
        }

        FilterCommunicationPort portRead;
        LIFE_EXPORT_CONNECTION_CONTEXT context;
        context.Type = LIFE_EXPORT_READ_CONNECTION_TYPE;
        HRESULT result = portRead.connect(std::wstring(AA_READ_PORT_NAME), 0, &context, sizeof(context));
        if (FAILED(result))
        {
            return result;
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
            result = portRead.getMessage(&requestMessage.Header, sizeof(requestMessage), NULL);
            if (result == HRESULT_FROM_WIN32(ERROR_OPERATION_ABORTED))
            {
                break;
            }
            else if (FAILED(result))
            {
                break;
            }

            result = S_OK;
            if (aContext && aContext->DriverHandler)
            {
                // Callback function
                result = aContext->DriverHandler->ReadingLifeTrackingFileCallback(&requestMessage.Notification.FileId,
                    &requestMessage.Notification.BlockFileOffset,
                    &requestMessage.Notification.BlockLength);
            }

            USER_LIFE_EXPORT_REPLY_READ_NOTIFICATION replyMessage{ 0 };
            replyMessage.ReplyHeader.MessageId = requestMessage.Header.MessageId;
            replyMessage.ReplyHeader.Status = result;
            CopyMemory(&replyMessage.ResponseData.FileId, &requestMessage.Notification.FileId, sizeof(replyMessage.ResponseData.FileId));

            replyMessage.ResponseData.ReadResult = READ_RESULT_WAIT_BLOCK;

            result = portRead.replyMessage(&replyMessage.ReplyHeader, sizeof(replyMessage));
            if (FAILED(result))
            {
                break;
            }
        }

        portRead.disconnect();

        return result;
    }


    HRESULT LifeExportDriverManager::controlMsgHandleFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext)
    {
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
                    if (aContext && aContext->DriverHandler)
                    {
                        // Callback
                        aContext->DriverHandler->DriverUnloadingCallback();
                    }

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

        return res;
    }


} // namespace LifeExportDriverManagement

