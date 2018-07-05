#ifndef _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_
#define _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_


#include <fltKernel.h>
#include "LifeExportFilterDriver.h"



//*************************************************************************
//    Global variables
//*************************************************************************


//
// Constants
//
#define     AA_MIN_SECTOR_SIZE                           0x200
#define     AA_MAX_PORT_CONNECTIONS                      1


//
// Tags
//
#define     AA_LIFE_EXPORT_CONNECTION_CONTEXT_TAG       'ccAA'
#define     AA_LIFE_EXPORT_VOLUME_NAME_TAG              'tvAA'
#define     AA_LIFE_EXPORT_FILE_CONTEXT_TAG             'cfAA'
#define     AA_LIFE_EXPORT_VOLUME_CONTEXT_TAG           'vcAA'
#define     AA_LIFE_EXPORT_PRE_2_POST_CONTEXT_TAG       'ppAA'


//
// Contexts
//
typedef union _AA_LIFE_EXPORT_FILE_ID_INFO
{
    struct
    {
        ULONGLONG Value;
        ULONGLONG UpperZeroes;
    } FileId_64;

    FILE_ID_128 FileId_128;

} AA_LIFE_EXPORT_FILE_ID_INFO, *PAA_LIFE_EXPORT_FILE_ID_INFO;

typedef struct _AA_FILE_CONTEXT
{
    ULONGLONG                       FileSize;
    AA_LIFE_EXPORT_FILE_ID_INFO     FileID;
} AA_FILE_CONTEXT, *PAA_FILE_CONTEXT;

#define AA_FILE_CONTEXT_SIZE    sizeof(AA_FILE_CONTEXT)


typedef struct _AA_VOLUME_CONTEXT
{
    UNICODE_STRING  Name;       // Holds the name to display
    ULONG           SectorSize; // Holds the sector size for this volume
} AA_VOLUME_CONTEXT, *PAA_VOLUME_CONTEXT;

#define AA_VOLUME_CONTEXT_SIZE  sizeof(AA_VOLUME_CONTEXT)


typedef struct _AA_PRE_2_POST_READ_CONTEXT_BUFFER
{
    PVOID     BufferPtr;
    ULONG     BufferSize;
    ULONG     CurrentProcessId;
} AA_PRE_2_POST_READ_CONTEXT_BUFFER, *PAA_PRE_2_POST_READ_CONTEXT_BUFFER;


typedef struct _AA_PRE_2_POST_READ_CONTEXT
{
    PAA_VOLUME_CONTEXT                  VolumeContext;
    AA_PRE_2_POST_READ_CONTEXT_BUFFER   Buffer;
} AA_PRE_2_POST_READ_CONTEXT, *PAA_PRE_2_POST_READ_CONTEXT;


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

    NPAGED_LOOKASIDE_LIST   Pre2PostContextList;    // This is a list used to allocate pre-2-post context structure

} GLOBAL_FILTER_DATA, *PGLOBAL_FILTER_DATA;

GLOBAL_FILTER_DATA GlobalData;


#endif // _LIFE_EXPORT_FILTER_DRIVER_GLOBALS_H_

