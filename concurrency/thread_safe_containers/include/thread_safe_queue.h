#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H

// реализуйте потоко защищенную очередь, которая в Pop ожидала бы появления нового элемента, если очередь пусткая

#include <optional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

template<class T>
class ThreadSafeQueue {
public:

    void Push([[maybe_unused]] T val) {
        std::unique_lock lock(mutex_);

        queue_.push(val);

        if (waitersCount_ > 0) {
            waitForSingleElemCondVar_.notify_one();
        }
    }

    std::optional<T> Pop() {
        std::unique_lock lock(mutex_);

        while (queue_.empty()) {
            if (isReleased_) {
                return std::nullopt;
            }

            ++waitersCount_;

            waitForSingleElemCondVar_.wait(lock);

            --waitersCount_;
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
        std::unique_lock lock(mutex_);

        isReleased_ = true;

        waitForSingleElemCondVar_.notify_all();
    }

private:
    std::queue<T> queue_;

    mutable std::mutex mutex_;

    std::condition_variable waitForSingleElemCondVar_;

    size_t waitersCount_ = 0;

    bool isReleased_ = false;
};

#endif
