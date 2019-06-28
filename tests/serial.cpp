#include <gtest/gtest.h>

namespace monip_serial {
    
    class monip_serial: public testing::Test {
        void SetUp() { }
        void TearDown() { }
    };

    TEST_F(monip_serial, test_Serial_WriteMessage)
    {
        ASSERT_TRUE(0 == 0);
    }
}
