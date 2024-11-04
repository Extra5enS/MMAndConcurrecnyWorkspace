#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

// реализуйте потоко защищенную очередь, которая в Pop ожидала бы появления нового элемента, если очередь пусткая

#include<optional>

template<class T>
class ThreadSafeQueue {
public:

    void Push([[maybe_unused]] T val) {
        std::unique_lock lock(mutex_);
        queue_.push(val);
        waitFinishCond_.notify_one();
    }

    std::optional<T> Pop() {
        std::unique_lock lock(mutex_);

        waitFinishCond_.wait(lock, [&](){return !queue_.empty() || needRelease_;});

        if (needRelease_)
        {
            return std::nullopt;
        }

        T val = queue_.front();
        queue_.pop();

        return val;
    }

    bool IsEmpty() {
        std::unique_lock lock(mutex_);
        return queue_.empty();
    }

    void ReleaseConsumers() {
        needRelease_ = true;
        waitFinishCond_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable waitFinishCond_;
    
    bool needRelease_ = false;
};

#endif