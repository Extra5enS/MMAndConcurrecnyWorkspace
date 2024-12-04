#ifndef CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H
#define CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H

#include <queue>
// #include <utility>
#include <stack>

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
    void AddCallback(Callback callback, Args... args) {
        auto task = [callable = std::forward<Callback>(callback),
                     args = std::tuple<Args...>(args...)] { std::apply(callable, args); };
        tasks_.emplace(std::move(task));
    }
private:
    std::queue<TaskType> tasks_;
};

class EventLoopScope {
public:
    EventLoopScope() {
        eventLoops_.push(new EventLoop);
    }
    ~EventLoopScope() {
        assert(!eventLoops_.empty());
        delete eventLoops_.top();
        eventLoops_.pop();
    }

    NO_COPY_SEMANTIC(EventLoopScope);
    NO_MOVE_SEMANTIC(EventLoopScope);

    template<class Callback, class... Args>
    static void AddCallback(Callback callback, Args... args) {
        if (eventLoops_.empty()) {
            return;
        }
        auto *top = eventLoops_.top();
        top->AddCallback(callback, args...);
    }
private:
    static std::stack<EventLoop*> eventLoops_; 
};

std::stack<EventLoop*> EventLoopScope::eventLoops_{}; // NOLINT(misc-definitions-in-headers, fuchsia-statically-constructed-objects, cert-err58-cpp)
#endif