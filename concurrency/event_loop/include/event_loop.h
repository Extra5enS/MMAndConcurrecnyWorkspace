#ifndef CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H
#define CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H

#include "base/macros.h"
#include <vector>
#include <thread>
#include <functional>
#include <iostream>
#include <unordered_map>
// event loop это механизм, завязанный на событиях и их асинхронной работе. Вы делаете post колбета, и он когда-нибудь исполнится
// В нашем случае будет использоваться EventLoopScope, и все колбеки должны исполниться при разрушении EventLoopScope.
// Для взаимодействия

class EventLoop {
public:

    EventLoop() = default;
    ~EventLoop()
    {
        for (const auto &funcToCall : callBacks_) {
            funcToCall();
        }
    }

    NO_MOVE_SEMANTIC(EventLoop);
    NO_COPY_SEMANTIC(EventLoop);

    template<class Callback, class... Args>
    void AddCallback([[maybe_unused]] Callback callback, [[maybe_unused]] Args... args) {
        auto funcToCall = [func = callback, args...] {
            return func(args...);
        };

        callBacks_.push_back(funcToCall);
    }
private:
    std::vector<std::function<void()>> callBacks_;
};



class EventLoopScope {
public:
    EventLoopScope() {
        scopeCallBacks_.push_back(new EventLoop {});
    }

    ~EventLoopScope() {
        if (scopeCallBacks_.empty()) {
            return;
        }

        delete scopeCallBacks_.back();
        scopeCallBacks_.pop_back();
    }

    NO_COPY_SEMANTIC(EventLoopScope);
    NO_MOVE_SEMANTIC(EventLoopScope);

    template<class Callback, class... Args>
    static void AddCallback(Callback callback, Args... args) {
        scopeCallBacks_.back()->AddCallback(callback, args...);
    }

    private:
        static std::vector<EventLoop *> scopeCallBacks_;
};

std::vector<EventLoop *> EventLoopScope::scopeCallBacks_; // NOLINT(misc-definitions-in-headers, fuchsia-statically-constructed-objects)

#endif
