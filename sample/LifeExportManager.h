#ifndef _LIFE_EXPORT_MANAGER_H_
#define _LIFE_EXPORT_MANAGER_H_


#include "IDriverHandler.h"
#include "LifeExportDriverManager.h"



namespace LifeExportManagement
{
    class LifeExportManager : protected LifeExportDriverManagement::IDriverHandler
    {
        public:
            LifeExportManager();
            ~LifeExportManager();

            const HRESULT exec();
            const HRESULT stop() const;

        private:
            // Noncopyable class
            LifeExportManager(const LifeExportManager&) = delete;
            LifeExportManager& operator = (const LifeExportManager&) = delete;

        private:
            // Driver manager callbacks
            virtual HRESULT CreatingFileCallback(const PAA_FILE_ID_INFO aFileIdInfo,
                LIFE_EXPORT_CONNECTION_RESULT& aResult);
            virtual HRESULT ReadingLifeTrackingFileCallback(const PAA_FILE_ID_INFO aFileIdInfo,
                const PULONGLONG aBlockFileOffset,
                const PULONGLONG aBlockLength);
            virtual void DriverUnloadingCallback();

        private:
            LifeExportDriverManagement::LifeExportDriverManager* m_driverManager;
    };

}; // namespace LifeExportManagement


#endif // _LIFE_EXPORT_MANAGER_H_

