#include <gtest/gtest.h>

#include <thread>

#include "util/rtscheduling.h"

#ifdef __LINUX__
#include <pthread.h>
#include <sched.h>

TEST(ThreadSchedulingTest, GuiChildrenInheritNormalPolicy) {
    // Run on a disposable thread so no test runner scheduling is changed.
    std::thread parent([] {
        ASSERT_TRUE(mixxx::restoreCurrentThreadNormalScheduling("test GUI"));
        std::thread child([] {
            int policy = -1;
            sched_param param{};
            ASSERT_EQ(0, pthread_getschedparam(pthread_self(), &policy, &param));
            EXPECT_EQ(SCHED_OTHER, policy);
            EXPECT_EQ(0, param.sched_priority);
        });
        child.join();
    });
    parent.join();
}
#endif
