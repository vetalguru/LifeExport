#include "gtest\gtest.h"

TEST(First_test_sample, simpletest)
{
    EXPECT_EQ(1, 1);
}


int wmain(int argc, wchar_t **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return  RUN_ALL_TESTS();
}
