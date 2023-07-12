//! @file
//! @brief  bmtest main function
//! @author Martin Cejp

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
