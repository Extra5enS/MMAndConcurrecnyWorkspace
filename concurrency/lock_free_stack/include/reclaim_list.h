#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_RECLAIM_LIST_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_RECLAIM_LIST_H

#include <atomic>

#include "base/macros.h"

template <class Deleter>
class ReclaimListNode {
public:
    NO_COPY_SEMANTIC(ReclaimListNode);
    NO_MOVE_SEMANTIC(ReclaimListNode);

    explicit ReclaimListNode(void *ptr) : data_(ptr) {}

    ~ReclaimListNode()
    {
        deleter_(data_);
    }

    void *Data() noexcept
    {
        return data_;
    }

    ReclaimListNode *&Next() noexcept
    {
        return next_;
    }

private:
    void *data_ = nullptr;
    ReclaimListNode *next_ = nullptr;
    Deleter deleter_ {};
};

template <class Deleter>
inline void AddToReclaimList(std::atomic<ReclaimListNode<Deleter> *> &list, ReclaimListNode<Deleter> *node)
{
    node->Next() = list.load(std::memory_order_relaxed);

    while (!list.compare_exchange_weak(node->Next(), node, std::memory_order_release, std::memory_order_relaxed)) {
    }
}

#endif  // CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_RECLAIM_LIST_H
