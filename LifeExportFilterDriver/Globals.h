#ifndef _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_
#define _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_


#include <fltKernel.h>
#include "LifeExportFilterDriver.h"
#include "Communication.h"



//*************************************************************************
//    Global variables
//*************************************************************************
#define AA_LIFE_EXPORT_CONNECTION_CONTEXT_TAG	'AACc'


typedef struct _AA_STREAM_CONTEXT
{
    //  File ID, obtained from querying the file system for
    //  FileInternalInformation or FileIdInformation.
    AA_FILE_ID_INFO		FileID;

} AA_STREAM_CONTEXT, *PAA_STREAM_CONTEXT;

#define AA_STREAM_CONTEXT_SIZE	sizeof(AA_STREAM_CONTEXT)


typedef struct _GLOBAL_FILTER_DATA
{
    PFLT_FILTER FilterHandle;     // results from a call to FltRegisterFilter

    //
    // The server ports.
    //
    PFLT_PORT   ServerPortCreate;  // communication port handle for "CREATE" messages
    PFLT_PORT   ServerPortRead;    // communication port handle for "READ" messages
    PFLT_PORT   ServerPortControl; // communication port handle for "CONTROL" messages 

    //
    //  The client ports.
    //  These ports are assigned at ConnectNotifyCallback and cleaned at DisconnectNotifyCallback
    //
    PFLT_PORT   ClientPortCreate;  // connection port regarding the "CREATE" message
    PFLT_PORT   ClientPortRead;    // connection port regarding the "READ" message
    PFLT_PORT   ClientPortControl; // coneection port regarding the "CONTROL" message

} GLOBAL_FILTER_DATA, *PGLOBAL_FILTER_DATA;

GLOBAL_FILTER_DATA GlobalData;


#endif // _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_
