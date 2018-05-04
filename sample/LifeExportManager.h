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
			std::atomic<bool> NeedFinalize;

			HANDLE CreateThreadHandle;

			HANDLE ReadThreadHandle;
		};

		public:
			LifeExportManager();
			~LifeExportManager();

			HRESULT exec();
			HRESULT stop();

		private:
			HRESULT initLifeExportManager();
			HRESULT startLifeExportManager();
			HRESULT stopLifeExportManager();
			HRESULT finalizeLifeExportManager();

		private:
			static HRESULT createMsgHandlerFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext);
			static HRESULT readMsgHandlerFunc(LIFE_EXPORT_MANAGER_CONTEXT* aContext);

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

