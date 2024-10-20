#ifndef CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H
#define CONCURRENCY_EVENT_LOOP_INCLUDE_EVENT_LOOP_H

#include "base/macros.h"

// event loop это механизм, завязанный на событиях и их асинхронной работе. Вы делаете post колбета, и он когда-нибудь исполнится
// В нашем случае будет использоваться EventLoopScope, и все колбеки должны исполниться при разрушении EventLoopScope.
// Для взаимодействия 

class EventLoop {
public:
    EventLoop() = default;
    ~EventLoop() = default;

    NO_MOVE_SEMANTIC(EventLoop);
    NO_COPY_SEMANTIC(EventLoop);    
    
    template<class Callback, class... Args>
    void AddCallback(Callback callback, Args... args) {
        // impl
    }
private:
    // add your fields
};

class EventLoopScope {

public:
    EventLoopScope() = default;
    ~EventLoopScope() = default;

    NO_COPY_SEMANTIC(EventLoopScope);
    NO_MOVE_SEMANTIC(EventLoopScope);

    template<class Callback, class... Args>
    static void AddCallback(Callback callback, Args... args) {
        // impl
    }
private:
    // add your fields
};


#endif