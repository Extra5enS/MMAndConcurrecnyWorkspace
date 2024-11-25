#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <optional>

#include "hazard_pointer.h"
#include "reclaim_list.h"

template <class T>
class LockFreeStack {
    struct Node final {
        T val;
        Node *next;
    };

public:
    NO_COPY_SEMANTIC(LockFreeStack);
    NO_MOVE_SEMANTIC(LockFreeStack);

    LockFreeStack() = default;

    ~LockFreeStack() noexcept
    {
        DeleteSafeNodes();
    }

    void Push(T val)
    {
        Node *newNode = new Node {val, nullptr};

        newNode->next = head_.load(std::memory_order_relaxed);

        while (!head_.compare_exchange_weak(newNode->next, newNode, std::memory_order_release,
                                            std::memory_order_relaxed)) {
        }
    }

    std::optional<T> Pop()
    {
        std::atomic<void *> &hp = GetLocalHazardPointer();

        Node *oldHead = head_.load(std::memory_order_relaxed);

        do {
            Node *temp = nullptr;
            do {
                temp = oldHead;
                hp.store(oldHead, std::memory_order_release);
                oldHead = head_.load(std::memory_order_acquire);
            } while (oldHead != temp);
        } while (oldHead && !head_.compare_exchange_strong(oldHead, oldHead->next, std::memory_order_acquire,
                                                           std::memory_order_relaxed));

        if (!oldHead) {
            return std::nullopt;
        }

        T val = oldHead->val;

        hp.store(nullptr, std::memory_order_release);

        if (IsHazardPointer(oldHead)) {
            ReclaimLater(oldHead);
        } else {
            delete oldHead;
        }

        DeleteSafeNodes();

        return val;
    }

    bool IsEmpty() noexcept
    {
        return head_.load(std::memory_order_relaxed) == nullptr;
    }

private:
    void ReclaimLater(Node *node)
    {
        AddToReclaimList(reclaimList_, new ReclaimListNode<Deleter>(node));
    }

    void DeleteSafeNodes() noexcept
    {
        ReclaimListNode<Deleter> *current = reclaimList_.exchange(nullptr, std::memory_order_acquire);

        while (current) {
            ReclaimListNode<Deleter> *next = current->Next();

            if (!IsHazardPointer(current->Data())) {
                delete current;
            } else {
                AddToReclaimList(reclaimList_, current);
            }
            current = next;
        }
    }

private:
    struct Deleter {
        void operator()(void *p)
        {
            delete static_cast<Node *>(p);
        }
    };

    std::atomic<Node *> head_ {nullptr};
    std::atomic<ReclaimListNode<Deleter> *> reclaimList_ {nullptr};
};

#endif
