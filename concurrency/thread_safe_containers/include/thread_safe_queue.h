#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

template<class T>
class ThreadSafeQueue {
public:
    void Push(T val) {
        std::lock_guard<std::mutex> lock(lock_);
        queue_.push(val);
        readyToPop_.notify_one();
    }

    std::optional<T> Pop() {
        std::unique_lock<std::mutex> lock(lock_);
        readyToPop_.wait(lock, [this]() { return !queue_.empty() || threadsReleased_; });

        if (queue_.empty()) {
            return std::nullopt;
        }

        T val = queue_.front();
        queue_.pop();

        return val;
    }

    bool IsEmpty() {
        std::lock_guard<std::mutex> lock(lock_);
        return queue_.empty();
    }

    void ReleaseConsumers() {
        std::lock_guard<std::mutex> lock(lock_);
        readyToPop_.notify_all();
        threadsReleased_ = true;
    }

private:
    std::queue<T> queue_;
    std::condition_variable readyToPop_;
    std::mutex lock_;
    bool threadsReleased_ = false;
};

#endif
