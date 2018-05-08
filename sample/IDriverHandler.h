#ifndef _LIFE_EXPORT_I_DRIVER_HANDLER_H_
#define _LIFE_EXPORT_I_DRIVER_HANDLER_H_

#include <Windows.h>
#include "Communication.h"


namespace LifeExportDriverManagement
{
    class IDriverHandler
    {
    public:
        IDriverHandler() {}
        virtual ~IDriverHandler() {}


        virtual HRESULT CreatingFileCallback(const PAA_FILE_ID_INFO aFileIdInfo,
                                             LIFE_EXPORT_CONNECTION_RESULT& aResult) = 0;

        virtual HRESULT ReadingLifeTrackingFileCallback(const PAA_FILE_ID_INFO aFileIdInfo,
                                                        const PULONGLONG aBlockFileOffset,
                                                        const PULONGLONG aBlockLength) = 0;

        virtual void DriverUnloadingCallback() = 0;
    };

}; // LifeExportDriverManagement


#endif // _LIFE_EXPORT_I_DRIVER_HANDLER_H_

