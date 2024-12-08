#ifndef CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H
#define CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H

#include "base/macros.h"
#include "concurrency/thread_safe_containers/include/thread_safe_queue.h"

#include <cstddef>
#include <thread>
#include <queue>
#include <future>

class ThreadPool {
    using TaskType = std::function<void()>;

public:
    explicit ThreadPool([[maybe_unused]] size_t countOfThread) : threadCount_(countOfThread)
    {
        threads_.reserve(threadCount_);

        auto threadWork = [this]() -> void
        {
            while (true)
            {
                {std::unique_lock lock(mutex_);
                cv_.wait(lock, [this]()->bool { return !tasks_.IsEmpty() || end_.load(); });}

                if (!tasks_.IsEmpty())
                {
                    auto task = std::move(tasks_.Pop());
                    
                    if (task.has_value())
                    {
                        (*task)();
                    }
                }
                else if (end_.load())
                {
                    tasks_.ReleaseConsumers();
                    return;
                }
            }
        };

        for (size_t i = 0; i < threadCount_; i++)
        {
            threads_.emplace_back(threadWork);
        }
    }

    ~ThreadPool()
    {
        for (auto &thread : threads_) {
            thread.join();
        }
    }

    NO_COPY_SEMANTIC(ThreadPool);
    NO_MOVE_SEMANTIC(ThreadPool);

    template<class Task, class... Args>
    void PostTask([[maybe_unused]] Task task, [[maybe_unused]] Args... args) {
        auto func = [callback = std::forward<Task>(task), 
                     args = std::tuple<Args...>(args...)] {
                        std::apply(callback, args);
                     };
        tasks_.Push(func);

        {std::unique_lock lock(mutex_); cv_.notify_one();}
    }
    
    void WaitForAllTasks() {
        end_.store(true);

        {std::unique_lock lock(mutex_); cv_.notify_all();}
    }

private:

    const size_t threadCount_ = 0;
    
    std::vector<std::thread> threads_ = {};
    ThreadSafeQueue<TaskType> tasks_ = {}; 

    std::condition_variable cv_;

    std::atomic<bool> end_{false};
    std::mutex mutex_;
};

#endif