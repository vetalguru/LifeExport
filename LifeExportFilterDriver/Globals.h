#ifndef _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_
#define _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_


#include <fltKernel.h>
#include "LifeExportFilterDriver.h"


//*************************************************************************
//    Global variables
//*************************************************************************
typedef struct _GLOBAL_FILTER_DATA
{
    PFLT_FILTER FilterHandle;   // results fro ma call to FltRegisterFilter
}GLOBAL_FILTER_DATA, *PGLOBAL_FILTER_DATA;

GLOBAL_FILTER_DATA GlobalData;


//
// This defines what we want to filter with FltMgr 
//
CONST FLT_REGISTRATION FilterRegistration = {
    sizeof(FLT_REGISTRATION),    // Size
    FLT_REGISTRATION_VERSION,    // Version
    0,                           // Flags

    NULL,                        // Context Registration
    NULL,                        // Operation Registration

    AA_Unload,                   // FilterUnload Callback
    NULL,                        // InstanceSetup Callback
    NULL,                        // InstanceQueryTeardown Callback
    NULL,                        // InstanceTeardownStart Callback
    NULL,                        // InstanceTeardownComplete Callback

    NULL,                        // GenerateFileName Callback
    NULL,                        // GenerateDestinationFileName Callback 
    NULL                         // NormalizeNameComponent Callback
};


#endif _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_
