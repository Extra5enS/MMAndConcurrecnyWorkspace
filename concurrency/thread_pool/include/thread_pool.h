#ifndef CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H
#define CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H

#include "base/macros.h"
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>
#include "../../thread_safe_containers/include/thread_safe_queue.h"

class ThreadPool {
public:
    explicit ThreadPool(size_t threadsCount) {
        threads_.reserve(threadsCount);
        for (size_t i = 0; i < threadsCount; ++i) {
            threads_.emplace_back(&ThreadPool::Run, this);
        }
    }

    ~ThreadPool() {
        releaseThreads_.store(true);
        {
            std::lock_guard lock(mtx_);
            readyToExecute_.notify_all();
        }

        for (auto &thread : threads_) {
            thread.join();
        }
    }

    NO_COPY_SEMANTIC(ThreadPool);
    NO_MOVE_SEMANTIC(ThreadPool);

    template<class Task, class... Args>
    void PostTask(Task task, Args... args) {
        // NOLINTNEXTLINE[modernize-avoid-bind,-warnings-as-errors]
        auto newTask = std::bind(std::forward<Task>(task), std::forward<Args>(args)...);
        tasks_.Push(std::move(newTask));
        {
            std::lock_guard lock(mtx_);
            ++tasksNum_;
            readyToExecute_.notify_one();
        }
    }

    void WaitForAllTasks() {
        std::unique_lock lock(mtx_);
        tasksCompleted_.wait(lock, [this]() {
            return tasks_.IsEmpty() && completedTasksNum_.load() == tasksNum_;
        });
        tasksNum_ = 0;
        completedTasksNum_.store(0);
    }

private:
    void Run() {
        while (true) {
            {
                std::unique_lock lock(mtx_);
                readyToExecute_.wait(lock, [this]() {
                    return !tasks_.IsEmpty() || releaseThreads_.load();
                });
            }

            if (!tasks_.IsEmpty()) {
                auto task = tasks_.Pop();
                if (task.has_value()) {
                    (*task)();
                }
                completedTasksNum_.fetch_add(1);
                tasksCompleted_.notify_all();
            }

            if (releaseThreads_.load()) {
                tasks_.ReleaseConsumers();
                return;
            }
        }
    }

    std::vector<std::thread> threads_;
    ThreadSafeQueue<std::function<void()>> tasks_;
    std::atomic<bool> releaseThreads_ {false};
    std::mutex mtx_;
    std::condition_variable readyToExecute_;
    std::condition_variable tasksCompleted_;
    size_t tasksNum_ = 0;
    std::atomic<size_t> completedTasksNum_ {0};
};

#endif

