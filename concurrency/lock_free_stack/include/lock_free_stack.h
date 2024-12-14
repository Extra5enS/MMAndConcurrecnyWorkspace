#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <optional>
#include <atomic>
#include "base/macros.h"

static constexpr size_t MAX_THREAD_COUNT = 16;
static constexpr size_t MAX_POINTERS_COUNT = 128;

template<class T>
class LockFreeStack {
    struct Node {
        explicit Node(const T& val, Node *next = nullptr) : val(val), next(next) {}
        T val;              // NOLINT(misc-non-private-member-variables-in-classes)
        Node *next;         // NOLINT(misc-non-private-member-variables-in-classes)
    };

    struct HazardPtr {
        std::atomic<Node *> ptr;
    };

    class RetiredPtrs {
    public:
        RetiredPtrs() = default;
        ~RetiredPtrs() {
            for (size_t i = 0; i < count_ && i <= MAX_POINTERS_COUNT; i++) {
                delete list_[i];
            }
        }

        NO_COPY_SEMANTIC(RetiredPtrs);
        NO_MOVE_SEMANTIC(RetiredPtrs);

        void SetPtr(Node *ptr, std::array<HazardPtr, MAX_THREAD_COUNT> &hazardPtrsArray) {
            assert (count_ < MAX_POINTERS_COUNT);
            list_[count_] = ptr;
            count_++;

            if (count_ >= MAX_POINTERS_COUNT) {
                CleanPtrs(hazardPtrsArray);
            }
        }

    private:
        bool IsInHazardPtrs(Node *ptr, std::array<HazardPtr, MAX_THREAD_COUNT> &hazardPtrsArray) {
            for (auto &hazardPtr : hazardPtrsArray) {
                if (ptr == hazardPtr.ptr.load()) {
                    return true;
                }
            }

            return false;
        }

        void CleanPtrs(std::array<HazardPtr, MAX_THREAD_COUNT> &hazardPtrsArray) {
            std::array<Node *, MAX_POINTERS_COUNT> cleanedList{};
            size_t newCount = 0;

            for (size_t i = 0; i < count_ && i <= MAX_POINTERS_COUNT; i++) {
                if (IsInHazardPtrs(list_[i], hazardPtrsArray)) {
                    cleanedList[newCount] = list_[i];
                    newCount++;
                }
                else {
                    delete list_[i];
                }
            }

            count_ = newCount;
            list_ = cleanedList;
        }

        size_t count_ = 0;
        std::array<Node *, MAX_POINTERS_COUNT> list_{};
    };

    class MemoryManager {
    public:
        MemoryManager() = default;
        ~MemoryManager() = default;
        
        NO_COPY_SEMANTIC(MemoryManager);
        NO_MOVE_SEMANTIC(MemoryManager);

        void SetHazardPtr(Node *ptr) {
            hazardPtrsArray_[GetThreadIndex()].ptr.store(ptr);
        }

        void SetRetiredPtr(Node *ptr) {
            retiredPtrsArray_[GetThreadIndex()].SetPtr(ptr, hazardPtrsArray_);
        }

        int GetThreadIndex() {
            if (threadIndex_ == -1) {
                size_t numThreads = numThreads_.load();
                while (!numThreads_.compare_exchange_weak(numThreads, numThreads + 1U)) { ; }
                threadIndex_ = numThreads;
            }

            return threadIndex_;
        }

    private:
        std::atomic<size_t> numThreads_ = 0;

        std::array<HazardPtr, MAX_THREAD_COUNT> hazardPtrsArray_{};
        std::array<RetiredPtrs, MAX_THREAD_COUNT> retiredPtrsArray_{};
        thread_local static int threadIndex_;
    }; // class MemoryManager
public:
    LockFreeStack() = default;
    ~LockFreeStack() = default;

    NO_COPY_SEMANTIC(LockFreeStack);
    NO_MOVE_SEMANTIC(LockFreeStack);

    void Push(T val) {
        auto newNode = new Node{val, head_.load()};
        while (!head_.compare_exchange_weak(newNode->next, newNode)) {;}
    }

    std::optional<T> Pop() {
        while (true) {
            Node *curHead = nullptr;

            do {
                curHead = head_.load();
                manager_.SetHazardPtr(curHead);
            } while (curHead != head_.load());

            if (curHead == nullptr) {
                return std::nullopt;
            }
            
            manager_.SetHazardPtr(curHead);
            if (head_.compare_exchange_strong(curHead, curHead->next)) {
                T val = curHead->val;
                manager_.SetHazardPtr(nullptr);
                manager_.SetRetiredPtr(curHead);
                return val;
            }
        }
    }

    bool IsEmpty() {
        return (head_.load() == nullptr);
    }

private:
    std::atomic<Node*> head_{nullptr};
    MemoryManager manager_;
};

template <typename T>
thread_local int LockFreeStack<T>::MemoryManager::threadIndex_ = -1;

#endif