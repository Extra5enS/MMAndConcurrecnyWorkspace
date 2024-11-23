#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <optional>
#include <atomic>
#include <vector>
#include <thread>
#include <algorithm>
#include <map>
#include "base/macros.h"

template <class T>
class HazardPtr;

template <class T>
class HazardPtrs;

template <class T>
class LockFreeStack;

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
    friend class LockFreeStack<T>;
    friend class HazardPtrs<T>;
public:
    explicit HazardPtr(size_t threadNum) : threadNum_(threadNum)
    {
        delList_.reserve(2 * threadNum);
    }

    NO_MOVE_SEMANTIC(HazardPtr);
    NO_COPY_SEMANTIC(HazardPtr);

    HazardPtr() = default;
    ~HazardPtr() = default;

private:
    std::atomic<Node<T>*> node_ = nullptr;
    std::vector<Node<T>*> delList_;
    size_t delCount_ = 0;
    size_t threadNum_ = 0;
}; // class HazardPtr


template <class T>
class HazardPtrs
{
    friend class LockFreeStack<T>;
public:
    explicit HazardPtrs(size_t threadNum) : threadNum_(threadNum) {}

    NO_MOVE_SEMANTIC(HazardPtrs);
    NO_COPY_SEMANTIC(HazardPtrs);

    ~HazardPtrs() = default;

    void DelNode(Node<T>* node)
    {
        HazardPtr<T>* myHazardPtr = hazardPtrs_[std::this_thread::get_id()];
        myHazardPtr->delList_[myHazardPtr->delCount_++] = node;
        if (myHazardPtr->delCount_ >= threadNum_ * 2)
        {
            Scan(myHazardPtr->delList_, myHazardPtr->delCount_);
        }
    }

private:
    void Scan(std::vector<Node<T>*> &delList, size_t &delCount)
    {
        std::vector<Node<T>*> plist;
        for (auto& iter : hazardPtrs_)
        {
            Node<T> *hptr = iter.second->node_.load();
            if (hptr != nullptr)
            {
                plist.push_back(hptr);
            }
        }

        std::sort(plist.begin(), plist.end());

        std::vector<Node<T>*> newDelList;
        newDelList.reserve(2 * threadNum_);
        size_t newDelCount = 0;

        for (size_t i = 0; i < delCount; ++i)
        {
            if (std::binary_search(plist.begin(), plist.end(), delList[i]))
            {
                newDelList.push_back(delList[i]);
                newDelCount++;
            }
            else
            {
                delete delList[i];
            }
        }

        for (size_t i = 0; i < newDelCount; i++)
        {
            delList[i] = newDelList[i];
        }
        delCount = newDelCount;
    }

private:
    std::map<std::thread::id, HazardPtr<T>*> hazardPtrs_;
    size_t threadNum_ = 0;
    
}; // class HazardPtrs

template <class T>
class LockFreeStack {

public:
    explicit LockFreeStack(size_t threadNum)
    {
        hazardPtrs_ = new HazardPtrs<T>{threadNum};
    }

    NO_MOVE_SEMANTIC(LockFreeStack);
    NO_COPY_SEMANTIC(LockFreeStack);

    ~LockFreeStack()
    {
        for (auto& iter : hazardPtrs_->hazardPtrs_)
        {
            if (iter.second != nullptr)
            {
                delete iter.second;
            }
        }
        delete hazardPtrs_;
    }

    void Push(T val)
    {
        auto *newNode = new Node<T>{val, head_.load()};
        while (!head_.compare_exchange_weak(newNode->next_, newNode))
        {
            ;
        }
    }

    std::optional<T> Pop()
    {
        Node<T> *curHead = nullptr;
        
        auto iter = hazardPtrs_->hazardPtrs_.find(std::this_thread::get_id());
        HazardPtr<T>* myHazardPtr = nullptr;
        if (iter == hazardPtrs_->hazardPtrs_.end())
        {
            myHazardPtr = new HazardPtr<T>{hazardPtrs_->threadNum_};
            hazardPtrs_->hazardPtrs_.insert(std::make_pair(std::this_thread::get_id(), myHazardPtr));
        }
        else
        {
            myHazardPtr = iter->second;
        }

        do
        {
            curHead = head_.load();
            if (curHead == nullptr)
            {
                return std::nullopt;
            }

            myHazardPtr->node_.store(curHead);

            if (curHead != head_.load())
            {
                continue;
            }

        } while (!head_.compare_exchange_weak(curHead, curHead->next_));
        
        T tmp = curHead->key_;
        myHazardPtr->node_.store(nullptr);
        hazardPtrs_->DelNode(curHead);
        return tmp;
    }

    bool IsEmpty()
    {
        return head_.load() == nullptr;
    }

private:
    HazardPtrs<T>* hazardPtrs_ = nullptr;
    std::atomic<Node<T>*> head_{nullptr};
}; // class LockFreeStack

#endif