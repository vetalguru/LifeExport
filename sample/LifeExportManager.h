#ifndef _LIFE_EXPORT_MANAGER_H_
#define _LIFE_EXPORT_MANAGER_H_

#include <Windows.h>
#include <atomic>
#include <thread>

namespace LifeExportManagement
{

    class LifeExportManager
    {
        struct LIFE_EXPORT_MANAGER_CONTEXT
        {
            std::atomic<bool> NeedFinalize; // Finalize of all CREATE and READ threads
            HANDLE CreateThreadHandle;      // CREATE thread handle (process CREATE messages from driver filter)
            HANDLE ReadThreadHandle;        // READ thread handle (process READ messages from driver filter)
            HANDLE ControlThreadHandle;     // thread to process contol messages

            LifeExportManager* CurrentManager;
        };

    public:
        LifeExportManager();
        ~LifeExportManager();

        HRESULT exec();
        HRESULT stop();

    private:
        // Manage CREATE and READ threads
        HRESULT initLifeExportManager();
        HRESULT startLifeExportManager();
        HRESULT stopLifeExportManager();
        HRESULT freeLifeExportManager();

    private:
        // Manage CONTROL communication port
        HRESULT initLifeExportControlThread();
        HRESULT startLifeExportControlThread();
        HRESULT stopLifeExportControlThread();
        HRESULT freeLifeExportControlThread();
      

    private:
        static HRESULT createMsgHandlerFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext);
        static HRESULT readMsgHandlerFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext);
        static HRESULT controlMsgHandleFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext);

    private:
        // Noncopyable class
        LifeExportManager(const LifeExportManager&) = delete;
        LifeExportManager& operator = (const LifeExportManager&) = delete;

    private:
        LIFE_EXPORT_MANAGER_CONTEXT m_context;

        bool m_loadedInStart;

    };

} // namespace LifeExportManagement

#endif // _LIFE_EXPORT_MANAGER_H_

