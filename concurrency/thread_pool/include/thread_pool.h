#ifndef CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H
#define CONCURRENCY_THREAD_POOL_INCLUDE_THREAD_POOL_H

#include "base/macros.h"
#include <cstddef>
#include <thread>
#include <queue>

class ThreadPool {
    using TaskType = std::function<void()>;
public:
    explicit ThreadPool([[maybe_unused]] size_t countOfTask) : stop_(false), threadCount_(countOfTask) {
        // workers_.reserve(threadCount_);

        // auto callable = [this]() {
        //     std::function<void()> task;
        //     while (true) {
        //         std::unique_lock<std::mutex> lock(this->mutex_);

        //         std::cout << "AA" << std::endl;
        //         cv_.wait(lock, [this]() { return !this->tasks_.empty() || this->stop_; });
            
        //         // while (tasks_.empty() && !stop_)
        //         //     cv_.wait(lock);

        //         if (this->stop_) {
        //             return;
        //         }

        //         task = std::move(this->tasks_.front());
        //         this->tasks_.pop();
        //         this->mutex_.unlock();

        //         task();
        //     }
        // };

        for (size_t i = 0; i < threadCount_; i++)
        {
            workers_.emplace_back([this]() {
            std::function<void()> task;
            while (true) {
                std::unique_lock<std::mutex> lock(this->mutex_);

                std::cout << "AA" << std::endl;
                cv_.wait(lock, [this]() { return !this->tasks_.empty() || this->stop_; });
            
                // while (tasks_.empty() && !stop_)
                //     cv_.wait(lock);

                if (this->stop_) {
                    return;
                }

                task = std::move(this->tasks_.front());
                this->tasks_.pop();
                this->mutex_.unlock();

                // task();
            }
        });
        }
    }

    ~ThreadPool() = default;
    NO_COPY_SEMANTIC(ThreadPool);
    NO_MOVE_SEMANTIC(ThreadPool);

    template<class Task, class... Args>
    void PostTask([[maybe_unused]] Task task, [[maybe_unused]] Args... args) {
        auto postTask = [callable = std::forward<Task>(task)] { return callable(); };
        
        std::unique_lock<std::mutex> lock(mutex_);
        tasks_.emplace(std::move(postTask));
        allTaskCount_++;
        cv_.notify_one();
    }
    
    void WaitForAllTasks() {
        std::unique_lock<std::mutex> lock(mutex_);
        stop_ = true;
        cv_.notify_all();
        mutex_.unlock();

        for (auto &worker : workers_) {
            worker.join();
        }
        std::cout << allTaskCount_ << std::endl;
    }

private:
    std::vector<std::thread> workers_ = {};
    std::queue<TaskType> tasks_ = {};
    std::mutex mutex_ = {};
    std::condition_variable cv_;

    bool stop_;
    size_t allTaskCount_ = {};
    size_t completedTaskCount_ = {};
    const size_t threadCount_;
};

#endif