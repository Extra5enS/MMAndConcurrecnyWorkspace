#ifndef CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H
#define CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H

#include <functional>
#include <queue>
#include <utility>
#include <vector>
#include "base/macros.h"

// event loop это механизм, завязанный на событиях и их асинхронной работе. Вы делаете post колбета, и он когда-нибудь исполнится
// В нашем случае будет использоваться EventLoopScope, и все колбеки должны исполниться при разрушении EventLoopScope.
// Для взаимодействия

class EventLoop {
public:
    EventLoop() = default;
    ~EventLoop() {
        while (!events_.empty()) {
            auto func = events_.front();
            func();
            events_.pop();
        }
    }

    NO_COPY_SEMANTIC(EventLoop);
    NO_MOVE_SEMANTIC(EventLoop);

    template<class Callback, class... Args>
    void AddCallback(Callback callback, Args... args) {
        auto func = (
            [call = std::forward<Callback>(callback), &args...]
            { return call(std::forward<Args>(args)...); } );

        events_.push(std::move(func));
    }

private:
    std::queue<std::function<void()>> events_;
};

class EventLoopScope {
public:
    EventLoopScope() {
        loops_.emplace_back(new EventLoop {});
    }
    ~EventLoopScope() {
        EventLoop *loop = loops_.back();
        loops_.pop_back();
        delete loop;
    }

    NO_COPY_SEMANTIC(EventLoopScope);
    NO_MOVE_SEMANTIC(EventLoopScope);

    template<class Callback, class... Args>
    static void AddCallback(Callback callback, Args... args) {
        loops_.back()->AddCallback(std::move(callback), std::move(args)...);
    }

private:
    static std::vector<EventLoop*> loops_;
};

// NOLINTNEXTLINE(misc-definitions-in-headers, fuchsia-statically-constructed-objects,-warnings-as-errors)
std::vector<EventLoop*> EventLoopScope::loops_ {};

#endif
