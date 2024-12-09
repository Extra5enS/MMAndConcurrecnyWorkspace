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

template <class PtrT>
class HazardPtrManager
{
private:

    struct HazardPtr
    {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::atomic<PtrT> ptr = nullptr;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::atomic<std::thread::id> id;
    }; // class HazardPtr

    struct HazardPtrWrapper
    {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        HazardPtr* hazardPtr = nullptr;

        explicit HazardPtrWrapper(std::array<HazardPtr, MAX_THREAD_NUM>& hazardPtrs)
        {
            for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
            {
                std::thread::id id;
                if (hazardPtrs[i].id.compare_exchange_strong(id, std::this_thread::get_id(), std::memory_order_acquire))
                {
                    hazardPtr = &hazardPtrs[i];
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
            hazardPtr->ptr.store(nullptr, std::memory_order_release);
        }
        
    }; // class HazardPtrWrapper

    struct DelList
    {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::array<PtrT, 2 * MAX_THREAD_NUM + 1> delList{};
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        size_t delCount = 0;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::atomic<std::thread::id> id;

        void DelNode(PtrT node, std::array<HazardPtr, MAX_THREAD_NUM>& hazardPtrs)
        {
            delList[delCount++] = node;
            if (delCount >= 2 * MAX_THREAD_NUM)
            {
                Scan(hazardPtrs);
            }
        }

    private:
        void Scan(std::array<HazardPtr, MAX_THREAD_NUM>& hazardPtrs)
        {
            std::array<PtrT, 2 * MAX_THREAD_NUM + 1> newDelList{};
            size_t newDelCount = 0;
            for (size_t i = 0; i < delCount; i++)
            {
                if (IsHazardPtr(delList[i], hazardPtrs))
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

        bool IsHazardPtr(PtrT ptr, std::array<HazardPtr, MAX_THREAD_NUM>& hazardPtrs)
        {
            for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
            {
                if (hazardPtrs[i].ptr.load(std::memory_order_acquire) == ptr)
                {
                    return true;
                }
            }

            return false;
        }

    }; // class DelList

    struct DelListWrapper
    {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        DelList* delList;

        explicit DelListWrapper(std::array<DelList, MAX_THREAD_NUM>& delLists)
        {
            for (size_t i = 0; i < MAX_THREAD_NUM; ++i)
            {
                std::thread::id id;
                if (delLists[i].id.compare_exchange_strong(id, std::this_thread::get_id(), std::memory_order_acquire))
                {
                    delList = &delLists[i];
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

public:
    std::atomic<PtrT>& GetHazardPtr()
    {
        thread_local static HazardPtrWrapper wrapper(hazardPtrs_);
        return wrapper.hazardPtr->ptr;
    }

    void DelNode(PtrT ptr)
    {
        thread_local static DelListWrapper wrapper(delLists_);
        wrapper.delList->DelNode(ptr, hazardPtrs_);
    }

private:

    std::array<HazardPtr, MAX_THREAD_NUM> hazardPtrs_;
    std::array<DelList, MAX_THREAD_NUM> delLists_;

}; // class HazardPointerManager

template <class T>
class LockFreeStack {

private:

    struct Node
    {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        T key;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        Node *next;

        explicit Node(const T& key, Node *next = nullptr) : key(key), next(next) {}
    }; // class Node
    
public:

    void Push(T val)
    {
        auto *newNode = new Node{val, head_.load(std::memory_order_acquire)};
        while (!head_.compare_exchange_weak(newNode->next, newNode, std::memory_order_release))
        {
            ;
        }
    }

    std::optional<T> Pop()
    {
        Node *curHead = nullptr;
        std::atomic<Node*>& hazardPtr = hpManager_.GetHazardPtr();

        do
        {
            curHead = head_.load(std::memory_order_acquire);
            if (curHead == nullptr)
            {
                return std::nullopt;
            }

            Node *tmp = nullptr;
            do {
                tmp = curHead;
                SetHazard(hazardPtr, curHead);
                curHead = head_.load(std::memory_order_acquire);
                if (curHead == nullptr)
                {
                    return std::nullopt;
                }
            } while (curHead != tmp);

        } while (!head_.compare_exchange_weak(curHead, curHead->next, std::memory_order_acquire));
        
        T tmp = curHead->key;
        SetHazard(hazardPtr, nullptr);
        hpManager_.DelNode(curHead);
        
        return tmp;
    }

    bool IsEmpty()
    {
        return head_.load() == nullptr;
    }

private:

    void SetHazard(std::atomic<Node*>& hazardPtr, Node* val)
    {
        hazardPtr.store(val, std::memory_order_release);
    }

    std::atomic<Node*> head_{nullptr};
    HazardPtrManager<Node*> hpManager_;
}; // class LockFreeStack

#endif