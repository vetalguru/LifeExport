#ifndef _LIFE_EXPORT_LIBRARY_SIMPLE_TESTS_H_
#define _LIFE_EXPORT_LIBRARY_SIMPLE_TESTS_H_

#include "gtest\gtest.h"
#include <Windows.h>

#define LIFE_EXPORT_LIBRARY_NAME L"LifeExportLibrary.dll"


TEST(LifeExportLibrary_Simple, TryLoadLibrary)
{
    // Load library
    HINSTANCE dllInstance = ::LoadLibrary(LIFE_EXPORT_LIBRARY_NAME);
    ASSERT_FALSE(dllInstance == NULL);
    
    if (dllInstance != NULL)
    {
        ::FreeLibrary(dllInstance);
        dllInstance = NULL;
    }
}

#endif _LIFE_EXPORT_LIBRARY_SIMPLE_TESTS_H_
