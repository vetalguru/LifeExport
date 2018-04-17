#ifndef _LIFE_EXPORT_LIBRARY_SIMPLE_TESTS_H_
#define _LIFE_EXPORT_LIBRARY_SIMPLE_TESTS_H_

#include "gtest\gtest.h"

#include <Windows.h>
#include <string.h>

#include "version.h"
#include "common.h"

/*
* GLOBAL VARIABLES
*/
const std::wstring LIFE_EXPORT_LIBRARY_NAME(L"LifeExportLibrary.dll");
const std::wstring BUILD_VERSION_FILE_NAME (L"BUILD");



TEST(LifeExportLibrary_Simple, TryLoadLibrary)
{
    // Load library
    HINSTANCE dllInstance = ::LoadLibraryW(LIFE_EXPORT_LIBRARY_NAME.c_str());
    ASSERT_FALSE(dllInstance == NULL);
    
    if (dllInstance != NULL)
    {
        // Unload library
        ::FreeLibrary(dllInstance);
        dllInstance = NULL;
    }

    return SUCCEED();
}


TEST(LifeExportLibrary_Simple, CheckLibraryVersion)
{
    //
    // Get expected values
    //
    DWORD expectedMajorVersion    = VERSION_MAJOR;
    DWORD expectedMinorVersion    = VERSION_MINOR;
    DWORD expectedBuildVersion    = 0; // this can be changed
    DWORD expectedRevisionVersion = 0; // this can be changed

    // Read build version
    HANDLE buildFile = ::CreateFileW(
        BUILD_VERSION_FILE_NAME.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (buildFile == INVALID_HANDLE_VALUE)
    {
        FAIL();
    }

    DWORD readed = 0;
    const unsigned BUILD_NUM_MAX_BYTE_SIZE = 20;
    std::vector<unsigned char> buildNumBuffer(BUILD_NUM_MAX_BYTE_SIZE, 0);
    BOOL res = ::ReadFile(buildFile, buildNumBuffer.data(), (DWORD)buildNumBuffer.size(), &readed, NULL);
    if (!res)
    {
        FAIL();
    }

    // It is not so good algorithm to convert vector of unsigned chars to DWORD but it has no problem with symbol '\0'
    std::string buildNumStr(buildNumBuffer.begin(), buildNumBuffer.end());
    buildNumStr = buildNumStr.substr(2); // remove first 2 chars
    buildNumStr.erase(std::remove(buildNumStr.begin(), buildNumStr.end(), '\0'), buildNumStr.end());
    
    expectedBuildVersion = std::stoi(buildNumStr);

    ::CloseHandle(buildFile);

    // get revision version
    std::wstring revision;
    DWORD errCode = execCommand(L"git rev-list --count HEAD", &revision, 5);
    if (errCode != ERROR_SUCCESS)
    {
        FAIL();
    }

    expectedRevisionVersion = std::stoi(revision);


    //
    // Get resul values
    //
    DWORD resultMajorVersion    = 0;
    DWORD resultMinorVersion    = 0;
    DWORD resultBuildVersion    = 0;
    DWORD resultRevisionVersion = 0;

    DWORD versionHandle = 0;
    DWORD versionDataSize = ::GetFileVersionInfoSizeW(LIFE_EXPORT_LIBRARY_NAME.c_str(), &versionHandle);
    ASSERT_FALSE(versionDataSize == 0);

    char* versionData = new (std::nothrow) char[versionDataSize];
    ASSERT_FALSE(versionData == NULL);

    res = ::GetFileVersionInfoW(LIFE_EXPORT_LIBRARY_NAME.c_str(), versionHandle, versionDataSize, versionData);
    if (!res)
    {
        delete [] versionData;
        versionData = NULL;

        FAIL();
    }

    LPBYTE buffer = NULL;
    UINT   bufferSize = 0;
    res = ::VerQueryValueW(versionData, L"\\", (VOID FAR* FAR*)&buffer, &bufferSize);
    if (!res || !bufferSize)
    {
        delete[] versionData;
        versionData = NULL;

        FAIL();
    }

    VS_FIXEDFILEINFO *fileInfo = (VS_FIXEDFILEINFO *)buffer;
    resultMajorVersion    = (fileInfo->dwFileVersionMS >> 16) & 0xffff;
    resultMinorVersion    = (fileInfo->dwFileVersionMS >>  0) & 0xffff;
    resultBuildVersion    = (fileInfo->dwFileVersionLS >> 16) & 0xffff;
    resultRevisionVersion = (fileInfo->dwFileVersionLS >>  0) & 0xffff;

    delete[] versionData;
    versionData = NULL;

    // Compare values
    ASSERT_EQ(expectedMajorVersion, resultMajorVersion);
    ASSERT_EQ(expectedMinorVersion, resultMinorVersion);

    ASSERT_TRUE(expectedBuildVersion != 0);
    ASSERT_TRUE(resultBuildVersion != 0);
    ASSERT_EQ(expectedBuildVersion, resultBuildVersion);

    ASSERT_TRUE(expectedRevisionVersion != 0);
    ASSERT_TRUE(resultRevisionVersion != 0);
    ASSERT_EQ(expectedRevisionVersion, resultRevisionVersion);

    return SUCCEED();
}


#endif _LIFE_EXPORT_LIBRARY_SIMPLE_TESTS_H_
