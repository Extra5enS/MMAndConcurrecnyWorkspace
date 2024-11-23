#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H

// реализуйте потоко защищенную очередь, которая в Pop ожидала бы появления нового элемента, если очередь пусткая

#include <optional>
#include <mutex>
#include <queue>
#include <condition_variable>

template <class T>
class ThreadSafeQueue {
public:
    void Push(T val)
    {
        std::unique_lock lock {mutex_};

        queue_.emplace(std::move(val));

        condPushedNewValue_.notify_one();
    }

    std::optional<T> Pop()
    {
        std::unique_lock lock {mutex_};

        while (queue_.empty() && !release_) {
            condPushedNewValue_.wait(lock);
        }

        if (release_) {
            return std::nullopt;
        }

        T ret = std::move(queue_.front());
        queue_.pop();

        return ret;
    }

    bool IsEmpty()
    {
        std::unique_lock lock {mutex_};

        return queue_.empty();
    }

    void ReleaseConsumers()
    {
        std::unique_lock lock {mutex_};

        release_ = true;

        condPushedNewValue_.notify_all();
    }

private:
    std::queue<T> queue_;
    std::mutex mutex_;
    std::condition_variable condPushedNewValue_;
    bool release_ = false;
};

#endif
