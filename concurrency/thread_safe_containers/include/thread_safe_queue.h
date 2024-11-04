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
        std::cerr << "PUSH " << pushn << std::endl;
        pushn++;
        waitFinishCond_.notify_one();
    }

    std::optional<T> Pop() {
        std::unique_lock lock(mutex_);
        waitCount_++;

        waitFinishCond_.wait(lock, [&](){return !queue_.empty();});

        std::cerr << "POP " << popn << std::endl;
        popn++;

        T val = queue_.front();
        queue_.pop();

        if (--waitCount_ == 0)
        {
            waitFinishCond_.notify_one();
        }
        return val;
    }

    bool IsEmpty() {
        std::unique_lock lock(mutex_);
        return queue_.empty();
    }

    void ReleaseConsumers() {
        // ... this method should release all threads that wait in Pop() method for new elems
    }

    std::atomic<size_t> waitCount_ = 0;
    std::atomic<size_t> pushn = 0;
    std::atomic<size_t> popn = 0;

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable waitFinishCond_;
    std::condition_variable waitCond1_;
    
};

#endif