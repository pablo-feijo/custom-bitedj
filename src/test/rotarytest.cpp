#include <gtest/gtest.h>

#include "util/rotary.h"

TEST(RotaryTest, JogFilterSupportsFullRangeAndConservesImpulse) {
    for (const int length : {1, 6, 64}) {
        Rotary rotary;
        rotary.setFilterLength(length);
        ASSERT_EQ(length, rotary.getFilterLength());
        double sum = rotary.filter(1.0);
        EXPECT_DOUBLE_EQ(1.0 / length, sum);
        for (int i = 1; i < length; ++i) {
            sum += rotary.filter(0.0);
        }
        EXPECT_NEAR(1.0, sum, 1e-12);
        EXPECT_DOUBLE_EQ(0.0, rotary.filter(0.0));
    }
}

TEST(RotaryTest, ChangingLengthClearsSamplesAndResetsCursor) {
    Rotary rotary;
    rotary.setFilterLength(64);
    for (int i = 0; i < 63; ++i) {
        rotary.filter(1.0);
    }
    rotary.setFilterLength(1);
    EXPECT_DOUBLE_EQ(0.0, rotary.filter(0.0));
    EXPECT_DOUBLE_EQ(2.0, rotary.filter(2.0));
    rotary.setFilterLength(64);
    EXPECT_DOUBLE_EQ(0.0, rotary.filter(0.0));
    rotary.setFilterLength(0);
    EXPECT_EQ(1, rotary.getFilterLength());
    rotary.setFilterLength(100);
    EXPECT_EQ(64, rotary.getFilterLength());
}
