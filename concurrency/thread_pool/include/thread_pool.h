#ifndef CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H
#define CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H

#include "base/macros.h"
#include <cstddef>

class ThreadPool {
public:
    explicit ThreadPool([[maybe_unused]] size_t countOfTask) {
        // impl
    }
    ~ThreadPool() = default;
    NO_COPY_SEMANTIC(ThreadPool);
    NO_MOVE_SEMANTIC(ThreadPool);

    template<class Task, class... Args>
    void PostTask(Task task, Args... args) {
        // impl
    }

    void WaitForAllTasks() {
        // impl
    }

private:
};

#endif