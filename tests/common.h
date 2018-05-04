#ifndef _LIFE_EXPORT_COMMON_TESTS_H_
#define _LIFE_EXPORT_COMMON_TESTS_H_

#include <Windows.h>
#include <string>


inline const DWORD execCommand(const std::wstring &aCmd, std::wstring *aResult, unsigned int aTimeSlice = 20 /* 20 ms */)
{
    if (aCmd.empty())
        return ERROR_BAD_COMMAND;

    if (aResult == NULL)
        return ERROR_INCORRECT_ADDRESS;

    aResult->clear();

    DWORD error = 0;

    SECURITY_ATTRIBUTES sAttr = { sizeof(SECURITY_ATTRIBUTES) };
    sAttr.bInheritHandle = TRUE;
    sAttr.lpSecurityDescriptor = NULL;

    // Create pipe to get result
    HANDLE hPipeRead, hPipeWrite = NULL;
    if (!::CreatePipe(&hPipeRead, &hPipeWrite, &sAttr, 0))
        return ::GetLastError();

    STARTUPINFOW startInfo = { sizeof(STARTUPINFOW) };
    startInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    startInfo.hStdOutput = hPipeWrite;
    startInfo.hStdError = hPipeWrite;
    startInfo.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION procInfo = { 0 };

    if (!::CreateProcessW(NULL, (LPWSTR)aCmd.c_str(), NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &startInfo, &procInfo))
    {
        error = ::GetLastError();
        ::CloseHandle(hPipeWrite);
        ::CloseHandle(hPipeRead);
        return error;
    }

    bool isProcerssEnded = false;
    std::string asciiResult;
    while (!isProcerssEnded)
    {
        isProcerssEnded = ::WaitForSingleObject(procInfo.hProcess, aTimeSlice) == WAIT_OBJECT_0;

        while (true)
        {
            // Check avail bytes
            DWORD availBytes = 0;
            if (!::PeekNamedPipe(hPipeRead, NULL, 0, NULL, &availBytes, NULL))
            {
                error = ::GetLastError();
                break;
            }

            if (!availBytes)
                break;

            char buffer[1024];
            ZeroMemory(buffer, 1024);
            DWORD readed = 0;
            if (!::ReadFile(hPipeRead, buffer, min(sizeof(buffer) - 1, availBytes), &readed, NULL))
            {
                error = ::GetLastError();
                break;
            }

            buffer[readed] = 0; // set end of string
            asciiResult += std::string(buffer);
        }
    }

    ::CloseHandle(hPipeWrite);
    ::CloseHandle(hPipeRead);
    ::CloseHandle(procInfo.hProcess);
    ::CloseHandle(procInfo.hThread);

    *aResult = std::wstring(asciiResult.begin(), asciiResult.end());

    return error;
}


#endif _LIFE_EXPORT_COMMON_TESTS_H_

