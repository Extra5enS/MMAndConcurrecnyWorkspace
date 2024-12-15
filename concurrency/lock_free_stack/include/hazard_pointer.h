#ifndef CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_HAZARD_POINTER_H
#define CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_HAZARD_POINTER_H

#include <array>
#include <atomic>
#include <stdexcept>
#include <thread>

#include "base/macros.h"

struct HazardPointer {
    std::atomic<void *> ptr {nullptr};
    std::atomic<std::thread::id> id = {};
};

static constexpr size_t MAX_THREADS = 32;

inline std::array<HazardPointer, MAX_THREADS> g_hazardPointers = {};

class HazardPointerThreadOwner {
public:
    NO_COPY_SEMANTIC(HazardPointerThreadOwner);
    NO_MOVE_SEMANTIC(HazardPointerThreadOwner);

    HazardPointerThreadOwner()
    {
        for (size_t i = 0; i < MAX_THREADS; i++) {
            auto thisId = std::this_thread::get_id();
            std::thread::id oldId;

            if (g_hazardPointers[i].id.compare_exchange_strong(oldId, thisId, std::memory_order_release,
                                                               std::memory_order_relaxed)) {
                hazardPointer_ = &g_hazardPointers[i];
                return;
            }
        }

        throw std::runtime_error("Too many threads!!!");
    }

    ~HazardPointerThreadOwner()
    {
        hazardPointer_->id.store(std::thread::id(), std::memory_order_release);
        hazardPointer_->ptr.store(nullptr, std::memory_order_release);
    }

    std::atomic<void *> &GetHazardPointer() noexcept
    {
        return hazardPointer_->ptr;
    }

private:
    HazardPointer *hazardPointer_ = nullptr;
};

inline std::atomic<void *> &GetLocalHazardPointer()
{
    thread_local static HazardPointerThreadOwner hpOwner;

    return hpOwner.GetHazardPointer();
}

inline bool IsHazardPointer(void *ptr) noexcept
{
    for (size_t i = 0; i < MAX_THREADS; i++) {
        if (ptr == g_hazardPointers[i].ptr.load(std::memory_order_acquire)) {
            return true;
        }
    }
    return false;
}

#endif  // CONCURRENCY_LOCK_FREE_STACK_CONTAINERS_INCLUDE_HAZARD_POINTER_H
