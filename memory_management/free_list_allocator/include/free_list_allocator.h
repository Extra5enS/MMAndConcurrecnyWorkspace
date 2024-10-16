#ifndef MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
#define MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H

#include <algorithm>
#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>
#include "base/macros.h"

template <size_t ONE_MEM_POOL_SIZE>
class FreeListAllocator {
    // here we recommend you to use class MemoryPool. Use new to allocate them from heap.
    // remember, you can not use any containers with heap allocations
    template <size_t MEM_POOL_SIZE>
    class FreeListMemoryPool {
        struct Block {
            size_t size;
            Block *next;
        };

        FreeListMemoryPool()
        {
            if (listHead_ == nullptr) {
                throw std::bad_alloc();
            }
        }

        char *Allocate(size_t size) {}

    private:
        size_t capacity_ = CalculateCapacity();
        Block *listHead_ = reinterpret_cast<Block *>(
            // NOLINTNEXTLINE(modernize-use-nullptr)
            mmap(NULL, capacity_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

        size_t CalculateCapacity() const noexcept
        {
            size_t psize = getpagesize();
            size_t cap = psize * (1 + ONE_MEM_POOL_SIZE / psize);
            if (ONE_MEM_POOL_SIZE % psize == 0) {
                cap -= psize;
            }
            return cap;
        }
    };

public:
    FreeListAllocator() = default;
    ~FreeListAllocator() = default;
    NO_MOVE_SEMANTIC(FreeListAllocator);
    NO_COPY_SEMANTIC(FreeListAllocator);

    template <class T = char>
    T *Allocate(size_t count)
    {
        return nullptr;
    }

    void Free(void *ptr) {}

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr(void *ptr)
    {
        return false;
    }

private:
};

#endif  // MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
