#include "gtest\gtest.h"

// LifeExportLibrary tests
#include "LifeExportLibrary\Simple_tests.h"


// LifeExportFilterDriver tests
#include "LifeExportFilterDriver\Test_inf_file.h"



int wmain(int argc, wchar_t **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return  RUN_ALL_TESTS();
}
