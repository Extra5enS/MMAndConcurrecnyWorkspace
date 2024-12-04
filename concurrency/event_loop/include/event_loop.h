#ifndef CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H
#define CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H

#include <queue>
#include <any>
#include <utility>

#include "base/macros.h"

using TaskType = std::function<void()>;

class EventLoop {
public:
    EventLoop() = default;
    ~EventLoop() {
        while (!tasks_.empty()) {
            auto task = std::move(tasks_.front());
            tasks_.pop();
            task();
        }
    }

    NO_MOVE_SEMANTIC(EventLoop);
    NO_COPY_SEMANTIC(EventLoop);    

    template<class Callback, class... Args>
    void AddCallback([[maybe_unused]] Callback callback, [[maybe_unused]] Args... args) {
        auto task = [callable = std::forward<Callback>(callback)] { return callable(); };
        tasks_.emplace(std::move(task));
    }
private:
    std::queue<TaskType> tasks_;
};

class EventLoopScope {
public:
    EventLoopScope() : oldTaskCount_(taskCount_) { taskCount_ = 0; }
    ~EventLoopScope() {
        size_t taskCount = taskCount_;
        taskCount_ = oldTaskCount_;

        std::vector<TaskType> temp = {};

        for (size_t i = 0; i < taskCount; i++)
        {
            temp.push_back(std::move(tasks_.back()));
            tasks_.pop_back();
        }

        for (size_t i = 0; i < taskCount; i++)
        {
            auto task = std::move(temp[taskCount - i -1]);
            task();
        }
    }

    NO_COPY_SEMANTIC(EventLoopScope);
    NO_MOVE_SEMANTIC(EventLoopScope);

    template<class Callback, class... Args>
    static void AddCallback([[maybe_unused]] Callback callback, [[maybe_unused]] Args... args) {
        taskCount_++;
        auto task = [callable = std::forward<Callback>(callback)] { return callable(); };
        tasks_.emplace_back(std::move(task));
    }
private:
    static std::vector<TaskType> tasks_;
    static size_t taskCount_;
    const size_t oldTaskCount_;
};

size_t EventLoopScope::taskCount_ = 0;              // NOLINT(misc-definitions-in-headers)
std::vector<TaskType> EventLoopScope::tasks_ = {};  // NOLINT(misc-definitions-in-headers, fuchsia-statically-constructed-objects)
#endif