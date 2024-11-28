#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H

// реализуйте потоко защищенную очередь, которая в Pop ожидала бы появления нового элемента, если очередь пусткая

#include<optional>

template<class T>
class ThreadSafeQueue {
public:

    void Push([[maybe_unused]] T val) {
        // ... implement this
    }

    std::optional<T> Pop() {
        // ... implement this
        return T{}; // NOLINT(clang-analyzer-core.StackAddressEscape)
    }

    bool IsEmpty() {
        // ... implement this
        return true;
    }

    void ReleaseConsumers() {
        // ... this method should release all threads that wait in Pop() method for new elems
    }

private:
};

#endif
