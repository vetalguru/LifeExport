#ifndef _LIFE_EXPORT_FILTER_DRIVER_DRIVER_TESTS_H_
#define _LIFE_EXPORT_FILTER_DRIVER_DRIVER_TESTS_H_


#include "gtest\gtest.h"
#include "common.h"


const std::wstring TOOLS_PATH(L"C:\\Program Files (x86)\\Windows Kits\\10\\Tools\\x64\\");
const std::wstring INF_FILE_NAME(L"LifeEXportFilterDriver.inf");



TEST(LifeExportFilterDriver_INF_file, Check_infVerif)
{
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/devtest/infverif
    std::wstring command = TOOLS_PATH;
    command += std::wstring(L"infVerif.exe");
    command += L" ";
    command += INF_FILE_NAME;
    std::wstring result;

    DWORD errCode = execCommand(command, &result);

    ASSERT_TRUE(errCode == ERROR_SUCCESS);
    ASSERT_TRUE(result.empty());

    return SUCCEED();
}


#endif _LIFE_EXPORT_FILTER_DRIVER_DRIVER_TESTS_H_
