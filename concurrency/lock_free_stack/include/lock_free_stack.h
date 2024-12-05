#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_LOCK_FREE_STACK_H

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <optional>
#include <thread>
#include "base/macros.h"

#define NDEBUG

constexpr size_t MAX_THREADS = 10;

template <typename T>
class HazardPointerManager {
public:
    HazardPointerManager() = default;
    ~HazardPointerManager()
    {
        for (auto &domain : domains_) {
            delete domain.load();
        }
    }

    NO_COPY_SEMANTIC(HazardPointerManager);
    NO_MOVE_SEMANTIC(HazardPointerManager);

    T Protect(std::atomic<T> &hazardPtr, const std::atomic<T> &p)
    {
        T ptr = nullptr;

        do {
            ptr = p.load();
            hazardPtr.store(ptr);
        } while (ptr != p.load());

        return ptr;
    }

    std::atomic<T> &GetPtr()
    {
        thread_local static std::atomic<HazardPointerDomain *> &domain = GetDomain(std::this_thread::get_id());
        return domain.load()->GetPtr();
    }

    void Retire(std::atomic<T> &ptr)
    {
        thread_local static std::atomic<HazardPointerDomain *> &domain = GetDomain(std::this_thread::get_id());
        return domain.load()->Retire(ptr);
    }

private:
    struct HazardPointerDomain {
        HazardPointerDomain(const std::thread::id &threadId, const HazardPointerManager *mng) :
            id(threadId), manager(mng) {}

        NO_COPY_SEMANTIC(HazardPointerDomain);
        NO_MOVE_SEMANTIC(HazardPointerDomain);

        ~HazardPointerDomain()
        {
            for (size_t i = 0; i < retireListSize; ++i) {
                delete retireList[i];
            }

            for (size_t i = 0; i < pointersSize.load(); ++i) {
                T ptr = pointers[i].load();
                delete ptr;
            }
        }

        std::atomic<T> &GetPtr()
        {
            std::atomic<T> &ptr = pointers[pointersSize.load()];
            pointersSize.fetch_add(1);
            return ptr;
        }

        void Retire(std::atomic<T> &p)
        {
            T ptr = p.load();
            p.store(nullptr);
            pointersSize.fetch_sub(1);
            retireList[retireListSize++] = ptr;
            if (retireListSize >= MAX_POINTERS - 1) {
                FreeRetireList();
            }
        }

        void FreeRetireList()
        {
            std::array<T, MAX_POINTERS> newRetireList {nullptr};
            size_t newRetireListSize = 0;
            for (size_t i = 0; i < retireListSize; ++i) {
                if (manager->FindDomain(retireList[i])) {
                    newRetireList[newRetireListSize++] = retireList[i];
                } else {
                    delete retireList[i];
                }
            }

            retireList = newRetireList;
            retireListSize = newRetireListSize;
        }

        bool Find(const T ptr) const
        {
            for (size_t i = 0; i < pointersSize.load() && i < MAX_POINTERS; ++i) {
                if (pointers[i].load() == ptr) {
                    return true;
                }
            }

            return false;
        }

        static constexpr size_t MAX_POINTERS = 128;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::thread::id id;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::array<std::atomic<T>, MAX_POINTERS> pointers {T {nullptr}};
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::atomic<size_t> pointersSize{0};
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        std::array<T, MAX_POINTERS> retireList {nullptr};
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        size_t retireListSize = 0;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        const HazardPointerManager *manager;
    };

    std::atomic<HazardPointerDomain *> &GetDomain(const std::thread::id &id)
    {
        auto *newDomain = new HazardPointerDomain {id, this};
        for (auto &domain : domains_) {
            if (domain.load() == nullptr) {
                HazardPointerDomain *nullDomain = nullptr;
                if (domain.compare_exchange_strong(nullDomain, newDomain)) {
                    return domain;
                }
            }
            if (domain.load()->id == id) {
                delete newDomain;
                return domain;
            }
        }

        delete newDomain;
        assert(0 && "Not enough domains!");
    }

    bool FindDomain(const T ptr) const
    {
        for (auto &domain : domains_) {
            if (domain.load() == nullptr) {
                return false;
            }
            if (domain.load()->Find(ptr)) {
                return true;
            }
        }

        return false;
    }

    std::array<std::atomic<HazardPointerDomain *>, MAX_THREADS> domains_ {nullptr};
};

template <class T>
class LockFreeStack {
public:
    void Push(const T val)
    {
        Node *newNode = new Node {val};
        Node *curHead = head_.load();
        newNode->next = curHead;

        while (!head_.compare_exchange_weak(newNode->next, newNode)) {
        }
    }

    std::optional<T> Pop()
    {
        std::atomic<Node *> &hazardPtr = hazardManager_.GetPtr();
        Node *curHead = nullptr;

        while (true) {
            curHead = hazardManager_.Protect(hazardPtr, head_);
            if (curHead == nullptr) {
                return std::nullopt;
            }

            if (head_.compare_exchange_strong(curHead, curHead->next)) {
                T val = curHead->val;
                hazardManager_.Retire(hazardPtr);
                return val;
            }
        }
    }

    bool IsEmpty()
    {
        return head_.load() == nullptr;
    }

private:
    struct Node {
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        T val;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        Node *next;
        explicit Node(T value) : val(value) {}
    };

    std::atomic<Node *> head_ {nullptr};
    HazardPointerManager<Node *> hazardManager_;
};

#endif
