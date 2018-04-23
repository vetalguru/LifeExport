#ifndef _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_
#define _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_


#include <fltKernel.h>
#include "LifeExportFilterDriver.h"
#include "Callbacks.h"



//*************************************************************************
//    Global variables
//*************************************************************************
typedef struct _GLOBAL_FILTER_DATA
{
    PFLT_FILTER FilterHandle;     // results from a call to FltRegisterFilter

	PFLT_PORT   ServerPortCreate; // communication port handle for "CREATE" messages 
}GLOBAL_FILTER_DATA, *PGLOBAL_FILTER_DATA;

GLOBAL_FILTER_DATA GlobalData;

#endif // _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_
