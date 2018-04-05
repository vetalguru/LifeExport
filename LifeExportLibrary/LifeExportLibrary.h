#ifndef _LIFE_EXPORT_LIBRARY_H_
#define _LIFE_EXPORT_LIBRARY_H_


#ifdef DLL_EXPORTS
    #define LIFEEXPORTDLL_API __declspec(dllexport) 
#else
    #define LIFEEXPORTDLL_API __declspec(dllimport) 
#endif






#endif _LIFE_EXPORT_LIBRARY_H_
