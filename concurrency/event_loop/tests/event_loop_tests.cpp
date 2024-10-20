#include <gtest/gtest.h>
#include <algorithm>

#include "concurrency/event_loop/include/event_loop.h"


TEST(EventLoopTests, DISABLED_DefaultEventLoopTest) {
    size_t check = 0;
    EventLoop loop;

    loop.AddCallback([&check]() {
        check++;
    });
    ASSERT_EQ(check, 0);
    loop.~EventLoop();
    ASSERT_EQ(check, 1);

    std::string str;
    EventLoop loop1;
    loop1.AddCallback([&str] {
        str += "A";
    });
    loop1.AddCallback([&str] {
        str += "B";
    });
    ASSERT_EQ(str, "");
    loop1.~EventLoop();
    ASSERT_EQ(str, "AB");
}

TEST(EventLoopTests, DISABLED_EventLoopScopeTest) {
    std::string str;
    {
        EventLoopScope scope; // NOLINT(clang-diagnostic-unused-variable)
        EventLoopScope::AddCallback([&str]() {
            str += "A";
        });
        EventLoopScope::AddCallback([&str]() {
            str += "B";
        });
        {
            EventLoopScope scope; // NOLINT(clang-diagnostic-unused-variable)
            EventLoopScope::AddCallback([&str]() {
                str += "C";
            });
            EventLoopScope::AddCallback([&str]() {
                str += "D";
            });
            ASSERT_EQ(str, "");
        }
        ASSERT_EQ(str, "CD");
    }
    ASSERT_EQ(str, "CDAB");
}

