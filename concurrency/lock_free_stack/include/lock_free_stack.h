#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <optional>
#include <atomic>
#include <vector>
#include <thread>
#include <algorithm>
#include <cstring>
#include <array>
#include "base/macros.h"

const size_t MAX_THREAD_NUM = 100;

template <class T>
struct Node
{
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    T key;
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    Node<T> *next;

    explicit Node(const T& key, Node<T> *next = nullptr) : key(key), next(next) {}
}; // class Node

template <class T>
struct HazardPtr
{
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::atomic<Node<T>*> node = nullptr;
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::atomic<std::thread::id> id;
}; // class HazardPtr

template <class T>
std::array<HazardPtr<T>, MAX_THREAD_NUM> g_hazardPtrs;

template <class T>
struct DelList
{
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::array<Node<T>*, 2 * MAX_THREAD_NUM + 1> delList{};
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    size_t delCount = 0;
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    std::atomic<std::thread::id> id;

    void DelNode(Node<T>* node)
    {
        delList[delCount++] = node;
        if (delCount >= 2 * MAX_THREAD_NUM)
        {
            Scan();
        }
    }

private:
    void Scan()
    {
        std::array<Node<T>*, 2 * MAX_THREAD_NUM + 1> newDelList{};
        size_t newDelCount = 0;
        for (size_t i = 0; i < delCount; i++)
        {
            if (IsHazardPtr(delList[i]))
            {
                newDelList[newDelCount++] = delList[i];
            }
            else
            {
                delete delList[i];
            }
        }
        
        delList = newDelList;
        delCount = newDelCount;
    }

    bool IsHazardPtr(Node<T>* node)
    {
        for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
        {
            if (g_hazardPtrs<T>[i].node.load(std::memory_order_acquire) == node)
            {
                return true;
            }
        }

        return false;
    }

}; // class DelList

template <class T>
std::array<DelList<T>, MAX_THREAD_NUM> g_delLists;

template <class T>
class LockFreeStack {

    struct HazardPtrWrapper
    {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        HazardPtr<T>* hazardPtr = nullptr;

        HazardPtrWrapper()
        {
            for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
            {
                std::thread::id id;
                if (g_hazardPtrs<T>[i].id.compare_exchange_strong(id, std::this_thread::get_id(), std::memory_order_acquire))
                {
                    hazardPtr = &g_hazardPtrs<T>[i];
                    break;
                }
            }

            assert(hazardPtr != nullptr && "There are more threads than the maximum number");
        }

        NO_MOVE_SEMANTIC(HazardPtrWrapper);
        NO_COPY_SEMANTIC(HazardPtrWrapper);

        ~HazardPtrWrapper()
        {
            hazardPtr->id.store(std::thread::id{}, std::memory_order_release);
            hazardPtr->node.store(nullptr, std::memory_order_release);
        }
        
    }; // class HazardPtrWrapper

    std::atomic<Node<T>*>& GetHazardPtr()
    {
        thread_local static HazardPtrWrapper wrapper;
        return wrapper.hazardPtr->node;
    }

    struct DelListWrapper
    {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        DelList<T>* delList;

        DelListWrapper()
        {
            for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
            {
                std::thread::id id;
                if (g_delLists<T>[i].id.compare_exchange_strong(id, std::this_thread::get_id(), std::memory_order_acquire))
                {
                    delList = &g_delLists<T>[i];
                    break;
                }
            }

            assert(delList != nullptr && "There are more threads than the maximum number");
        }

        NO_MOVE_SEMANTIC(DelListWrapper);
        NO_COPY_SEMANTIC(DelListWrapper);

        ~DelListWrapper()
        {
            delList->id.store(std::thread::id{}, std::memory_order_release);
        }
    };

    DelList<T>* GetDelList()
    {
        thread_local static DelListWrapper wrapper;
        return wrapper.delList;
    }

public:

    void Push(T val)
    {
        auto *newNode = new Node<T>{val, head_.load(std::memory_order_acquire)};
        while (!head_.compare_exchange_weak(newNode->next, newNode, std::memory_order_release))
        {
            ;
        }
    }

    std::optional<T> Pop()
    {
        Node<T> *curHead = nullptr;
        std::atomic<Node<T>*>& hazardPtr = GetHazardPtr();
        DelList<T>* delList = GetDelList();

        do
        {
            curHead = head_.load(std::memory_order_acquire);
            if (curHead == nullptr)
            {
                return std::nullopt;
            }

            Node<T> *tmp = nullptr;
            do {
                tmp = curHead;
                hazardPtr.store(curHead, std::memory_order_release);
                curHead = head_.load(std::memory_order_acquire);
                if (curHead == nullptr)
                {
                    return std::nullopt;
                }
            } while (curHead != tmp);

        } while (!head_.compare_exchange_weak(curHead, curHead->next, std::memory_order_acquire));
        
        T tmp = curHead->key;
        hazardPtr.store(nullptr, std::memory_order_release);
        delList->DelNode(curHead);
        
        return tmp;
    }

    bool IsEmpty()
    {
        return head_.load() == nullptr;
    }

private:

    std::atomic<Node<T>*> head_{nullptr};
}; // class LockFreeStack

#endif