#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <optional>

template<class T>
class LockFreeStack {
public:

    void Push([[maybe_unused]] T val) {
        // ... implement this
    }

    std::optional<T> Pop() {
        // ... implement this
        return T{};
    }

    bool IsEmpty() {
        // ... implement this
        return true;
    }

private:
};

#endif