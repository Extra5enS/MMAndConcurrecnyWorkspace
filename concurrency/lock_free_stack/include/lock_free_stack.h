#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <optional>
#include <atomic>
#include <vector>
#include <thread>
#include <algorithm>
#include <cstring>
#include "base/macros.h"

template <class T>
class HazardPtr;

template <class T>
class LockFreeStack;

const size_t MAX_THREAD_NUM = 100;

template <class T>
class Node
{
    friend class LockFreeStack<T>;
public:
    explicit Node(const T& key, Node<T> *next = nullptr) : key_(key), next_(next) {}
    
private:
    T key_;
    Node<T> *next_;
}; // class Node

template <class T>
class HazardPtr
{
public:
    NO_MOVE_SEMANTIC(HazardPtr);
    NO_COPY_SEMANTIC(HazardPtr);

    HazardPtr() = default;
    ~HazardPtr() = default;

public:
    std::atomic<Node<T>*> node_ = nullptr;
    std::atomic<std::thread::id> id_;
}; // class HazardPtr

template <class T>
HazardPtr<T> hazardPtrs[MAX_THREAD_NUM];

template <class T>
class HazardPtrWrapper
{
public:
    HazardPtrWrapper()
    {
        for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
        {
            std::thread::id id;
            if (hazardPtrs<T>[i].id_.compare_exchange_strong(id, std::this_thread::get_id()))
            {
                hazardPtr_ = &hazardPtrs<T>[i];
                break;
            }
        }

        assert(hazardPtr_ != nullptr && "There are more threads than the maximum number");
    }

    NO_MOVE_SEMANTIC(HazardPtrWrapper);
    NO_COPY_SEMANTIC(HazardPtrWrapper);

    ~HazardPtrWrapper()
    {
        hazardPtr_->id_.store(std::thread::id{});
        hazardPtr_->node_.store(nullptr);
    }

public:
    HazardPtr<T>* hazardPtr_ = nullptr;
}; // class HazardPtrWrapper

template <class T>
std::atomic<Node<T>*>& getHazardPtr()
{
    thread_local static HazardPtrWrapper<T> wrapper;
    return wrapper.hazardPtr_->node_;
}

template <class T>
bool isHazardPtr(Node<T>* node)
{
    for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
    {
        if (hazardPtrs<T>[i].node_.load() == node)
        {
            return true;
        }
    }

    return false;
}

template <class T>
class DelList
{
public:
    void delNode(Node<T>* node)
    {
        delList_[delCount_++] = node;
        if (delCount_ >= 2 * MAX_THREAD_NUM)
        {
            scan();
        }
    }

public:
    void scan()
    {
        Node<T>* newDelList[2 * MAX_THREAD_NUM + 1] = {};
        size_t newDelCount = 0;
        for (size_t i = 0; i < delCount_; i++)
        {
            if (isHazardPtr(delList_[i]))
            {
                newDelList[newDelCount++] = delList_[i];
            }
            else
            {
                delete delList_[i];
            }
        }
        
        std::memcpy(delList_, newDelList, 2 * MAX_THREAD_NUM * sizeof(Node<T>*));
        delCount_ = newDelCount;
    }

    Node<T>* delList_[2 * MAX_THREAD_NUM + 1] = {};
    size_t   delCount_ = 0;
    std::atomic<std::thread::id> id_;

}; // class DelList

template <class T>
DelList<T> delLists[MAX_THREAD_NUM];

template <class T>
class DelListWrapper
{
public:
    DelListWrapper()
    {
        for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
        {
            std::thread::id id;
            if (delLists<T>[i].id_.compare_exchange_strong(id, std::this_thread::get_id()))
            {
                delList_ = &delLists<T>[i];
                break;
            }
        }

        assert(delList_ != nullptr && "There are more threads than the maximum number");
    }

    NO_MOVE_SEMANTIC(DelListWrapper);
    NO_COPY_SEMANTIC(DelListWrapper);

    ~DelListWrapper()
    {
        delList_->id_.store(std::thread::id{});
    }

public:
    DelList<T>* delList_;
};

template <class T>
DelList<T>* getDelList()
{
    thread_local static DelListWrapper<T> wrapper;
    return wrapper.delList_;
}

template <class T>
class LockFreeStack {

public:
    LockFreeStack() = default;

    NO_MOVE_SEMANTIC(LockFreeStack);
    NO_COPY_SEMANTIC(LockFreeStack);

    ~LockFreeStack() = default;

    void Push(T val)
    {
        auto *newNode = new Node<T>{val, head_.load(std::memory_order_acquire)};
        while (!head_.compare_exchange_weak(newNode->next_, newNode, std::memory_order_release))
        {
            ;
        }
    }

    std::optional<T> Pop()
    {
        Node<T> *curHead = nullptr;
        std::atomic<Node<T>*>& hazardPtr = getHazardPtr<T>();
        DelList<T>* delList = getDelList<T>();

        do
        {
            curHead = head_.load(std::memory_order_acquire);
            if (curHead == nullptr)
            {
                return std::nullopt;
            }

            hazardPtr.store(curHead);
            if (curHead != head_.load(std::memory_order_acquire))
            {
                continue;
            }

        } while (!head_.compare_exchange_weak(curHead, curHead->next_, std::memory_order_acquire));
        
        T tmp = curHead->key_;
        hazardPtr.store(nullptr);
        delList->delNode(curHead);
        
        return tmp;
    }

    bool IsEmpty()
    {
        return head_.load(std::memory_order_acquire) == nullptr;
    }

private:

    std::atomic<Node<T>*> head_{nullptr};
}; // class LockFreeStack

#endif