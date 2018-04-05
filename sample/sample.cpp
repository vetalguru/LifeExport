#include <Windows.h>
#include <iostream>


#define LIFE_EXPORT_LIBRARY_NAME "LifeExportLibrary.dll"


int wmain(int argc, wchar_t* argv[])
{
    // Load library
    HINSTANCE dllInstance = ::LoadLibrary(LIFE_EXPORT_LIBRARY_NAME);
    if (dllInstance == NULL)
    {
        std::cerr << "Unable to load library" << std::endl;
        return - 1;
    }
 
    std::wcout << "Library was loaded" << std::endl;

    // Unload library
    ::FreeLibrary(dllInstance);
    dllInstance = NULL;

    return 0;
}
