#ifndef _LIFE_EXPORT_FILTER_CONTEXT_H_
#define _LIFE_EXPORT_FILTER_CONTEXT_H_


#include <fltKernel.h>
#include "Globals.h"


#define     AA_FILE_CONTEXT_TAG             'cfAA'
#define     AA_VOLUME_CONTEXT_TAG           'vcAA'


typedef struct _AA_FILE_CONTEXT
{
    ULONGLONG           FileSize;
    AA_FILE_ID_INFO     FileID;
} AA_FILE_CONTEXT, *PAA_FILE_CONTEXT;

#define AA_FILE_CONTEXT_SIZE    sizeof(AA_FILE_CONTEXT)


typedef struct _AA_VOLUME_CONTEXT
{
    UNICODE_STRING  Name;       // Holds the name to display
    ULONG           SectorSize; // Holds the sector size for this volume
} AA_VOLUME_CONTEXT, *PAA_VOLUME_CONTEXT;

#define AA_VOLUME_CONTEXT_SIZE  siezof(AA_VOLUME_CONTEXT)


NTSTATUS
AA_CreateFileContext(
    _Outptr_ PAA_FILE_CONTEXT *aFileContext
);


VOID
AA_FileContextCleanup(
    _In_ PFLT_CONTEXT     aContext,
    _In_ FLT_CONTEXT_TYPE aContextType
);


VOID
AA_VolumeContextCleanup(
    _In_ PFLT_CONTEXT     aContext,
    _In_ FLT_CONTEXT_TYPE aContextType
);


#endif // _LIFE_EXPORT_FILTER_CONTEXT_H_

