#include <Windows.h>
#include <iostream>
#include <fltUser.h>


#include "LifeExportManager.h"

#define LIFE_EXPORT_LIBRARY_NAME "LifeExportLibrary.dll"

using namespace LifeExportManagement;

int wmain(int argc, wchar_t* argv[])
{
    // Load library
    HINSTANCE dllInstance = ::LoadLibrary(LIFE_EXPORT_LIBRARY_NAME);
    if (dllInstance == NULL)
    {
        std::cerr << "Unable to load library" << std::endl;
        return -1;
    }

#ifndef NDEBUG
    std::wcout << "Library was loaded" << std::endl;
#endif // !NDEBUG

    LifeExportManager exportManager;
    HRESULT err = exportManager.exec();
    if (FAILED(err))
    {
        // Unload library
        ::FreeLibrary(dllInstance);
        dllInstance = NULL;

        return -1;
    }


    while (true)
    {
        unsigned char ch = (unsigned char)getchar();
        if (ch == 'q')
        {
            break;
        }
    }

    // Unload library
    ::FreeLibrary(dllInstance);
    dllInstance = NULL;

    return 0;
}

