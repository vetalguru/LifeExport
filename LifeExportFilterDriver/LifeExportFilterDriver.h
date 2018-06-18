#ifndef _LIFE_EXPORT_FILTER_DRIVER_H_
#define _LIFE_EXPORT_FILTER_DRIVER_H_

#include <fltKernel.h>


//*************************************************************************
//    Driver Function Prototypes
//*************************************************************************


DRIVER_INITIALIZE DriverEntry;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  aDriverObject,
    _In_ PUNICODE_STRING aRegistryPath
);


NTSTATUS
AA_Unload(
    _Unreferenced_parameter_ FLT_FILTER_UNLOAD_FLAGS aFlags
);


FLT_PREOP_CALLBACK_STATUS
AA_PreCreate(
    _Inout_                        PFLT_CALLBACK_DATA     aData,
    _In_                           PCFLT_RELATED_OBJECTS  aFltObjects,
    _Flt_CompletionContext_Outptr_ PVOID                 *aCompletionContext
);


FLT_POSTOP_CALLBACK_STATUS
AA_PostCreate(
    _Inout_  PFLT_CALLBACK_DATA       aData,
    _In_     PCFLT_RELATED_OBJECTS    aFltObjects,
    _In_opt_ PVOID                    aCompletionContext,
    _In_     FLT_POST_OPERATION_FLAGS aFlags
);


FLT_PREOP_CALLBACK_STATUS
AA_PreClose(
    _Inout_                        PFLT_CALLBACK_DATA    aData,
    _In_                           PCFLT_RELATED_OBJECTS aFltObjects,
    _Flt_CompletionContext_Outptr_ PVOID                 *aCompletionContext
);


FLT_PREOP_CALLBACK_STATUS
AA_PreRead(
    _Inout_                        PFLT_CALLBACK_DATA    aData,
    _In_                           PCFLT_RELATED_OBJECTS aFltObjects,
    _Flt_CompletionContext_Outptr_ PVOID                 *aCompletionContext
);


#endif // _LIFE_EXPORT_FILTER_DRIVER_H_

