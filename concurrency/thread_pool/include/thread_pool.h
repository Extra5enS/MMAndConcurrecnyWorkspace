#ifndef CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H
#define CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H

#include "base/macros.h"
#include "concurrency/thread_safe_containers/include/thread_safe_queue.h"

#include <cstddef>
#include <thread>
#include <queue>
#include <future>
#include <tuple> 

class ThreadPool {
    using TaskType = std::function<void()>;
public:
    explicit ThreadPool([[maybe_unused]] size_t countOfTask) : threadCount_(countOfTask) {
        workers_.reserve(threadCount_);

        auto callable = [this]() -> void {
            while (true) {
                if (!tasks_.IsEmpty())
                {
                    auto task = std::move(tasks_.Pop());
                    if (task.has_value())
                    {
                        (*task)();
                    }
                }
                else if (stop_.load())
                {
                    tasks_.ReleaseConsumers();
                    return;
                }
            }
        };

        for (size_t i = 0; i < threadCount_; i++) {
            workers_.emplace_back(callable);
        }
    }

    ~ThreadPool() {
        stop_.store(true);

        std::unique_lock lock(mutex_);
        cv_.notify_all();
        lock.unlock();

        for (auto &worker : workers_) {
            worker.join();
        }
    }

    NO_COPY_SEMANTIC(ThreadPool);
    NO_MOVE_SEMANTIC(ThreadPool);

    template<class Task, class... Args>
    void PostTask(Task task, Args... args) {
        auto postTask = [callable = std::forward<Task>(task), args = std::tuple<Args...>(args...)] { std::apply(callable, args); };
        tasks_.Push(postTask);
    }
    
    void WaitForAllTasks() {
        while (!tasks_.IsEmpty()) {
            cv_.notify_one();
        }
    }

private:
    const size_t threadCount_;
    
    std::vector<std::thread> workers_ = {};
    ThreadSafeQueue<TaskType> tasks_ = {}; 

    std::atomic<bool> stop_{false};
    std::mutex mutex_ = {};
    std::condition_variable cv_;
};

#endif
