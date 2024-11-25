#include <gtest/gtest.h>
#include <thread>
#include <atomic>
#include <vector>
#include <mutex>
#include <algorithm>

#include "concurrency/thread_safe_containers/include/thread_safe_queue.h"
#include "concurrency/thread_safe_containers/include/fast_thread_safe_map.h"


TEST(ThreadSafeQueueTest, SingleThreadTest) {
    ThreadSafeQueue<size_t> queue;
    ASSERT_TRUE(queue.IsEmpty());

    static constexpr size_t MAX_VALUE_TO_PUSH = 10U;
    for(size_t i = 0; i < MAX_VALUE_TO_PUSH; i++) {
        queue.Push(i);
        ASSERT_FALSE(queue.IsEmpty());
    }

    for(size_t i = 0; i < MAX_VALUE_TO_PUSH; i++) {
        ASSERT_FALSE(queue.IsEmpty());
        auto val = queue.Pop();
        ASSERT_EQ(i, val);
    }

    ASSERT_TRUE(queue.IsEmpty());
}

TEST(ThreadSafeQueueTest, MultithreadingTest) {
    ThreadSafeQueue<size_t> queue;
    ASSERT_TRUE(queue.IsEmpty());
    std::atomic<size_t> pushCounter = 0;
    std::atomic<size_t> popCounter = 0;

    std::vector<size_t> container;
    std::mutex lock;

    static constexpr size_t THREAD_COUNT = 10U;
    static constexpr size_t PUSH_COUNT = 1000U;

    auto push = [&queue, &pushCounter](){
        for(size_t i = 0; i < PUSH_COUNT; i++) {
            size_t val = pushCounter++;
            queue.Push(val);
        }
    };

    auto pop = [&queue, &container, &lock, &popCounter]() {
        while(popCounter != THREAD_COUNT * PUSH_COUNT) {
            auto val = queue.Pop();
            if(!val.has_value()) {
                return;
            }
            {
                std::lock_guard lg(lock);
                container.push_back(val.value());
            }
            popCounter++;
        }
    };

    std::vector<std::thread> pushers;
    std::vector<std::thread> poppers;
    for(size_t i = 0; i < THREAD_COUNT; i++) {
        pushers.emplace_back(push);
        poppers.emplace_back(pop);
    }
    while(popCounter != THREAD_COUNT * PUSH_COUNT) {
        // wait here
    }
    queue.ReleaseConsumers();

    for(auto& pusher : pushers) {
        pusher.join();
    }
    for(auto& popper: poppers) {
        popper.join();
    }

    std::sort(container.begin(), container.end());
    for(size_t i = 0; i < THREAD_COUNT * PUSH_COUNT; i++) {
        ASSERT_EQ(container[i], i);
    }

    ASSERT_TRUE(queue.IsEmpty());
}

TEST(FastThreadSafeMap, SingleThreadTest) {
    ThreadSafeMap<size_t, size_t> map;

    static constexpr size_t MAX_VALUE_TO_PUSH = 10U;

    for(size_t i = 0; i < MAX_VALUE_TO_PUSH; i++) {
        ASSERT_FALSE(map.Test(i));
    }

    for(size_t i = 0; i < MAX_VALUE_TO_PUSH; i++) {
        map.Insert(i, i);
        ASSERT_TRUE(map.Test(i));
    }

    for(size_t i = 0; i < MAX_VALUE_TO_PUSH; i++) {
        map.Erase(i);
        ASSERT_FALSE(map.Test(i));
    }

}
