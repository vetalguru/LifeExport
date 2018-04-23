#include <Windows.h>
#include <iostream>
#include <fltUser.h>

#include "Communication.h"
#include "FilterCommunicationPort.h"


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

	FilterCommunicationPort portCreate;
	HRESULT res = portCreate.connect(std::wstring(AA_CREATE_PORT_NAME), 0);
	if (FAILED(res))
	{
		std::wcout << L"Connect error. Error code " << res << std::endl;
	}

	if (!portCreate.isConnected())
	{
		std::wcout << L"port is not connected" << std::endl;
	}


	portCreate.disconnect();

    // Unload library
    ::FreeLibrary(dllInstance);
    dllInstance = NULL;

    return 0;
}
