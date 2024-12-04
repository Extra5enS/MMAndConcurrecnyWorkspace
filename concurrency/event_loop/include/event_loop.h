#ifndef CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H
#define CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H

#include "base/macros.h"

#include <functional>
#include <queue>

// event loop это механизм, завязанный на событиях и их асинхронной работе. Вы делаете post колбета, и он когда-нибудь исполнится
// В нашем случае будет использоваться EventLoopScope, и все колбеки должны исполниться при разрушении EventLoopScope.
// Для взаимодействия 

class EventLoop {
public:
    EventLoop() = default;
    ~EventLoop()
    {
        while (!tasks_.empty())
        {
            auto task = tasks_.front();
            task();
            tasks_.pop();
        }
    }

    NO_MOVE_SEMANTIC(EventLoop);
    NO_COPY_SEMANTIC(EventLoop);
    
    template<class Callback, class... Args>
    void AddCallback([[maybe_unused]] Callback callback, [[maybe_unused]] Args... args) {
        std::function<void()> task = std::bind(std::forward<Callback>(callback),
                                               std::forward<Args>(args)...);
        tasks_.push(task);
    }

private:
    std::queue<std::function<void()>> tasks_;
};

class EventLoopScope {

public:
    EventLoopScope()
    {
        oldTasks_ = tasks_;
        tasks_ = {};
    }

    ~EventLoopScope()
    {
        while (!tasks_.empty())
        {
            auto task = tasks_.front();
            task();
            tasks_.pop();
        }
        tasks_ = oldTasks_;
    }

    NO_COPY_SEMANTIC(EventLoopScope);
    NO_MOVE_SEMANTIC(EventLoopScope);

    template<class Callback, class... Args>
    static void AddCallback(Callback callback, Args... args) {
        std::function<void()> task = std::bind(std::forward<Callback>(callback),
                                               std::forward<Args>(args)...);
        tasks_.push(task);
    }

private:
    static std::queue<std::function<void()>> tasks_;
    std::queue<std::function<void()>> oldTasks_;
};

std::queue<std::function<void()>> EventLoopScope::tasks_;


#endif