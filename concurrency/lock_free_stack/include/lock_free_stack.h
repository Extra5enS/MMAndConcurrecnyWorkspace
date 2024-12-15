#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <optional>
#include <atomic>
#include <array>

#include "base/macros.h"

const size_t MAX_HAZARDS_COUNT = 20;

template<class T>
struct StackNode {
    T data;          //NOLINT(misc-non-private-member-variables-in-classes)
    StackNode *prev; //NOLINT(misc-non-private-member-variables-in-classes)

    StackNode(T val, StackNode *newPrev) : data(val), prev(newPrev) {}
};

template<class T>
class LockFreeStack {
// =================== LOCK FREE STACK METHODS ===================
public:
    LockFreeStack() = default;

    NO_COPY_SEMANTIC(LockFreeStack);
    NO_MOVE_SEMANTIC(LockFreeStack);

    void Push([[maybe_unused]] T val) {
        auto *newNode = new StackNode<T> {val, nullptr};

        while (true) {
            newNode->prev = head_.load(std::memory_order_acquire);

            if (head_.compare_exchange_weak(newNode->prev, newNode, std::memory_order_release,
                                                                    std::memory_order_acquire)) {
                break;
            }
        }
    }

    std::optional<T> Pop() {
        std::atomic<StackNode<T> *> &hazPtr = hazardPointers_.GetHazardPointerForCurThread();

        StackNode<T> *oldHead = nullptr;

        T valueToReturn;

        StackNode<T> *prevHead = nullptr;

        do {
            oldHead = head_.load(std::memory_order_acquire);

            if (oldHead == nullptr) {
                return std::nullopt;
            }

            StackNode<T> *tmp = oldHead;
            hazPtr.store(oldHead, std::memory_order_release);
            oldHead = head_.load(std::memory_order_acquire);

            if (tmp != oldHead) {
                hazPtr.store(nullptr, std::memory_order_release);
                continue;
            }

            if (oldHead == nullptr) {
                hazPtr.store(nullptr);
                return std::nullopt;
            }

            if (head_.compare_exchange_weak(oldHead, oldHead->prev, std::memory_order_release,
                                                                    std::memory_order_acquire)) {
                break;
            }
        } while (true);

        valueToReturn = oldHead->data;

        hazPtr.store(nullptr, std::memory_order_release);

        if (oldHead) {
            if (hazardPointers_.IsHazard(oldHead)) {
                DeleteLater(oldHead);
            }
            else {
                delete oldHead;
            }

            DeleteRetired();
        }

        return valueToReturn;
    }

    bool IsEmpty() {
        return head_ == nullptr;
    }

    ~LockFreeStack() {
        RetiredPointer *curRetiredListHead = retiredListHead_.load();

        while (curRetiredListHead) {
            RetiredPointer *prevHead = curRetiredListHead->prev;

            delete curRetiredListHead;

            curRetiredListHead = prevHead;
        }
    }

// =================== HAZARD POINTERS STRUCTS AND METHODS ===================

    struct HazardPtr {
        std::atomic<std::thread::id> ownerId = {};
        std::atomic<StackNode<T> *> ptr = nullptr;
    };

    class HazardPointers {
    public:
        std::atomic<StackNode<T> *> &GetHazardPointerForCurThread() {
            thread_local static HazardPointerOwner hPtrOwner;

            return hPtrOwner.GetHazardPtr();
        }

        bool IsHazard(StackNode<T> *ptr) {
            for (int i = 0; i < MAX_HAZARDS_COUNT; i++) {
                if (hazardPointersArray_[i].ptr.load() == ptr) {
                    return true;
                }
            }
            return false;
        }

    private:
        class HazardPointerOwner {
        public:
            HazardPointerOwner() : hPtr_(nullptr) {
                for (int i = 0; i < MAX_HAZARDS_COUNT; i++) {
                    std::thread::id oldId;

                    if (hazardPointersArray_[i].ownerId.compare_exchange_strong(oldId, std::this_thread::get_id(), std::memory_order_release,
                                                                                                                   std::memory_order_acquire)) {
                        hPtr_ = &hazardPointersArray_[i];
                        break;
                    }
                }
            }

            NO_COPY_SEMANTIC(HazardPointerOwner);
            NO_MOVE_SEMANTIC(HazardPointerOwner);

            std::atomic<StackNode<T> *> &GetHazardPtr() {
                return hPtr_->ptr;
            }

            ~HazardPointerOwner() {
                hPtr_->ptr.store(nullptr);
                hPtr_->ownerId.store(std::thread::id());
            }

        private:
            HazardPtr *hPtr_;
        };

        static std::array<HazardPtr, MAX_HAZARDS_COUNT> hazardPointersArray_;
    };
private:

// =================== RETIRED POINTERS STRUCTS AND METHODS ===================

    struct RetiredPointer {
        StackNode<T>   *ptr  = nullptr; //NOLINT(misc-non-private-member-variables-in-classes)
        RetiredPointer *prev = nullptr; //NOLINT(misc-non-private-member-variables-in-classes)

        explicit RetiredPointer(StackNode<T> *ptrToDeleteLater) : ptr(ptrToDeleteLater), prev(nullptr) {}

        NO_COPY_SEMANTIC(RetiredPointer);
        NO_MOVE_SEMANTIC(RetiredPointer);

        ~RetiredPointer() {
            delete ptr;
        }
    };

    void AddToRetiredList(RetiredPointer *newRetired) {
        while (true) {
            newRetired->prev = retiredListHead_.load(std::memory_order_acquire);

            if (retiredListHead_.compare_exchange_weak(newRetired->prev, newRetired, std::memory_order_release,
                                                                                     std::memory_order_acquire)) {
                break;
            }
        }
    }

    void DeleteLater(StackNode<T> *ptrToDelete) {
        AddToRetiredList(new RetiredPointer {ptrToDelete});
    }

    void DeleteRetired() {
        RetiredPointer *curRetiredListHead = retiredListHead_.exchange(nullptr, std::memory_order_acquire);

        while (curRetiredListHead) {
            RetiredPointer *prevHead = curRetiredListHead->prev;

            if (hazardPointers_.IsHazard(curRetiredListHead->ptr)) {
                AddToRetiredList(curRetiredListHead);
            }
            else {
                delete curRetiredListHead;
            }

            curRetiredListHead = prevHead;
        }
    }

// =================== LOCK FREE STACK FIELDS ===================
    HazardPointers hazardPointers_;

    std::atomic<RetiredPointer *> retiredListHead_ = nullptr;

    std::atomic<StackNode<T> *> head_ = nullptr;
};

template<class T> std::array<typename LockFreeStack<T>::HazardPtr, MAX_HAZARDS_COUNT> LockFreeStack<T>::HazardPointers::hazardPointersArray_; //NOLINT(fuchsia-statically-constructed-objects)

#endif
