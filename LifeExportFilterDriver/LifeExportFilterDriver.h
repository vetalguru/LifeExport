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


#endif // _LIFE_EXPORT_FILTER_DRIVER_H_

