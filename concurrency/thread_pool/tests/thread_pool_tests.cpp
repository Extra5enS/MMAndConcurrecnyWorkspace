#include <gtest/gtest.h>
#include <algorithm>

#include "concurrency/thread_pool/include/thread_pool.h"
#include <atomic>
#include <cstring>

TEST(ThreadPoolTests, DefaultThreadPoolTests) {
    static constexpr size_t THREAD_COUNT = 5;
    static constexpr size_t COUNT = 100'000;
    std::atomic_size_t count = 0;
    ThreadPool threadPool(THREAD_COUNT);
    for(size_t i = 0; i < COUNT; i++) {
        threadPool.PostTask([&count] {
            count++;
        });
    }

    threadPool.WaitForAllTasks();
    ASSERT_EQ(count, COUNT);
}
