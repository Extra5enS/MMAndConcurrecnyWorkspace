#ifndef CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H
#define CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H

#include "base/macros.h"

#include <functional>
#include <queue>
#include <stack>
#include <cassert>

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
        auto task = [task = std::forward<Callback>(callback), 
                     args = std::tuple<Args...>(args...)] {
                        std::apply(task, args);
                     };
        tasks_.push(task);
    }

private:
    std::queue<std::function<void()>> tasks_;
};

class EventLoopScope {

public:
    EventLoopScope()
    {
        eventLoops_.push(new EventLoop());
    }

    ~EventLoopScope()
    {
        assert(!eventLoops_.empty());

        delete eventLoops_.top();
        eventLoops_.pop();
    }

    NO_COPY_SEMANTIC(EventLoopScope);
    NO_MOVE_SEMANTIC(EventLoopScope);

    template<class Callback, class... Args>
    static void AddCallback(Callback callback, Args... args)
    {
        if (!eventLoops_.empty())
        {
            eventLoops_.top()->AddCallback(callback, args...);
        }
    }

private:
    static std::stack<EventLoop*> eventLoops_;
};

// NOLINTNEXTLINE(cert-err58-cpp, fuchsia-statically-constructed-objects, misc-definitions-in-headers)
std::stack<EventLoop*> EventLoopScope::eventLoops_;


#endif