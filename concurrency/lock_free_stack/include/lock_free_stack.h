#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <optional>
#include <atomic>

template<class T>
class LockFreeStack {
    
    template <typename U = T>
    struct Node {
    public:
        explicit Node(const T& val, Node<U> *next = nullptr) : val(val), next(next) {}
        //NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        U val;
        //NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        Node<U> *next;
    };

public:

    LockFreeStack() = default;

    void Push(T val) {
        auto newNode = new Node<T>{val, head_};
        while (!head_.compare_exchange_weak(newNode->next, newNode, std::memory_order_release)) {;}
    }

    std::optional<T> Pop() {
        Node<T> *head = head_.load(std::memory_order_acquire);
        Node<T> *next = nullptr;
        
        do 
        {
            if (head == nullptr)
            {
                return std::nullopt;
            }

            next = head->next;
        } while (!head_.compare_exchange_weak(head, next, std::memory_order_acquire));
        
        T temp = head->val;
        delete head;

        return temp;
    }

    bool IsEmpty() {
        return (head_.load(std::memory_order_acquire) == nullptr);
    }

private:
    std::atomic<Node<T>*> head_{nullptr};
};

#endif