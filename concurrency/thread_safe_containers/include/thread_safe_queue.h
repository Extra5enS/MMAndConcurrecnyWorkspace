#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>

template<class T>
class ThreadSafeQueue {
public:
    void Push(const T& val) {
        std::unique_lock<std::mutex> lock(mutex_);
        queue_.push(val);
        condVar_.notify_one();
    }

    T Pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condVar_.wait(lock, [this] { return !queue_.empty(); });
        
        T value = queue_.front();
        queue_.pop();
        return value;
    }

    bool IsEmpty() {
        std::unique_lock lock(mutex_);
        return queue_.empty();
    }

private:
    std::mutex mutex_;
    std::condition_variable condVar_;
    std::queue<T> queue_;
};

#endif