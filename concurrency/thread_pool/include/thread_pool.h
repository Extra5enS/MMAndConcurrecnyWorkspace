#ifndef CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H
#define CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H

#include "base/macros.h"
#include <cstddef>
#include <thread>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool {
public:
    explicit ThreadPool([[maybe_unused]] size_t countOfTask) : freeThreadsCount_(countOfTask) {
        for (int i = 0; i < countOfTask; i++) {
            threads_.emplace_back([this] {
                return Idle();
            });
        }
    }

    ~ThreadPool() {
        int threadsCount = threads_.size();

        for (int i = 0; i < threadsCount; i++) {
            threads_ [i].join();
        }
    }

    NO_COPY_SEMANTIC(ThreadPool);
    NO_MOVE_SEMANTIC(ThreadPool);

    template<class Task, class... Args>
    void PostTask([[maybe_unused]] Task task, [[maybe_unused]] Args... args) {
        auto taskToExecute = [func = task, args...] {
            return func(args...);
        };

        std::unique_lock lock(mutex_);

        taskQueue_.push(taskToExecute);

        if (freeThreadsCount_ > 0) {
            waitForTask_.notify_one();
        }
    }

    void WaitForAllTasks() {
        std::unique_lock lock(mutex_);
        isWait_ = true;

        while (!taskQueue_.empty()) {
            waitForTasksCompleted_.wait(lock);
        }

        waitForTask_.notify_all();
    }

private:
    void Idle() {
        std::function<void()> taskToExecute = {};

        while (true) {
            {
                std::unique_lock lock(mutex_);

                if (taskQueue_.empty() && isWait_) {
                    waitForTasksCompleted_.notify_one();
                }

                ++freeThreadsCount_;

                while (taskQueue_.empty()) {
                    if (isWait_) {
                        return;
                    }

                    waitForTask_.wait(lock);
                }

                --freeThreadsCount_;

                taskToExecute = taskQueue_.back();
                taskQueue_.pop();
            }

            taskToExecute();
        }
    }

    int freeThreadsCount_ = 0;

    bool isWait_ = false;

    std::queue<std::function<void()>> taskQueue_;

    std::vector<std::thread> threads_;

    std::condition_variable waitForTasksCompleted_;
    std::condition_variable waitForTask_;
    std::mutex mutex_;
};

#endif
