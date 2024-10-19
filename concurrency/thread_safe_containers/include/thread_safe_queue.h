#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_THREAD_SAFE_QUEUE_H

// реализуйте потоко защищенную очередь, которая в Pop ожидала бы появления нового элемента, если очередь пусткая

template<class T>
class ThreadSafeQueue {
public:

    void Push([[maybe_unused]] T val) {
        // ... implement this
    }

    T Pop() {
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