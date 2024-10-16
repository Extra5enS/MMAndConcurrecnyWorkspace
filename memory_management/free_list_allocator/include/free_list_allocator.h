#ifndef MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
#define MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include "base/macros.h"

template <size_t ONE_MEM_POOL_SIZE>
class FreeListAllocator {
    // here we recommend you to use class MemoryPool. Use new to allocate them from heap.
    // remember, you can not use any containers with heap allocations
    template <size_t MEM_POOL_SIZE>
    class FreeListMemoryPool;

public:
    FreeListAllocator() = default;
    ~FreeListAllocator() = default;
    NO_MOVE_SEMANTIC(FreeListAllocator);
    NO_COPY_SEMANTIC(FreeListAllocator);

    template <class T = uint8_t>
    T *Allocate([[maybe_unused]]size_t count)
    {
        // TODO(you): Add your implementation here...
        return nullptr;
    }

    void Free([[maybe_unused]]void *ptr)
    {
        // TODO(you): Add your implementation here...
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr([[maybe_unused]] void *ptr)  // TODO(you): remove [[maybe_unused]]
    {
        // TODO(you): Add your implementation here...
        return false;
    }

private:
    // TODO(you): Add your fields and methods here...
};

#endif  // MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H