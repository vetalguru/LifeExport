#ifndef _LIFE_EXPORT_FILTER_CALLBACKS_H_
#define _LIFE_EXPORT_FILTER_CALLBACKS_H_



#include <fltKernel.h>



FLT_PREOP_CALLBACK_STATUS
AA_PreCreate(
	_Inout_                        PFLT_CALLBACK_DATA     aData,
	_In_                           PCFLT_RELATED_OBJECTS  aFltObjects,
	_Flt_CompletionContext_Outptr_ PVOID                 *aCompletionContext
);




#endif _LIFE_EXPORT_FILTER_CALLBACKS_H_
