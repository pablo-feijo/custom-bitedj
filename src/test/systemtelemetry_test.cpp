#include <gtest/gtest.h>
#include "util/systemtelemetry.h"

using namespace mixxx::systemtelemetry;

TEST(SystemTelemetryTest, CpuSkipsGuestDoubleCountingAndIncludesIoWaitInIdle) {
    const auto ticks = parseCpuTicks("cpu 100 20 30 400 50 6 7 8 90 10\n");
    ASSERT_TRUE(ticks);
    EXPECT_EQ(ticks->total, 621);
    EXPECT_EQ(ticks->idle, 450);
    EXPECT_DOUBLE_EQ(*cpuUsage({100, 60}, {200, 110}), 50.0);
    EXPECT_DOUBLE_EQ(*cpuUsage({100, 60}, {200, 160}), 0.0);
}

TEST(SystemTelemetryTest, InvalidOrResetCountersAreUnavailable) {
    EXPECT_FALSE(parseCpuTicks("cpu0 1 2 3 4"));
    EXPECT_FALSE(parseCpuTicks("cpu 1 bad 3 4"));
    EXPECT_FALSE(parseCpuTicks(""));
    EXPECT_FALSE(cpuUsage({100, 60}, {100, 60}));
    EXPECT_FALSE(cpuUsage({100, 60}, {50, 30}));
    EXPECT_FALSE(cpuUsage({100, 60}, {110, 90}));
}

TEST(SystemTelemetryTest, TemperatureUsesMillidegreesAndRejectsMissingSensors) {
    ASSERT_TRUE(parseTemperature("57000\n"));
    EXPECT_DOUBLE_EQ(*parseTemperature("57000\n"), 57.0);
    EXPECT_FALSE(parseTemperature(""));
    EXPECT_FALSE(parseTemperature("nan"));
    EXPECT_FALSE(parseTemperature("999999"));
}
