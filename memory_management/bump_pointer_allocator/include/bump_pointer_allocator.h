#ifndef MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H
#define MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H

#include <sys/mman.h>
#include <cstddef>  // is used for size_t
#include <cstdint>
#include "base/macros.h"

template <size_t MEMORY_POOL_SIZE>
class BumpPointerAllocator {
public:
    BumpPointerAllocator() : blockStart_(mmap(nullptr,
                                              MEMORY_POOL_SIZE,
                                              PROT_READ   | PROT_WRITE,
                                              MAP_PRIVATE | MAP_ANONYMOUS,
                                              -1,
                                               0)),
                             // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                             blockEnd_(reinterpret_cast<char*>(blockStart_) + MEMORY_POOL_SIZE),
                             current_(blockStart_)
                             {}

    ~BumpPointerAllocator()
    {
        munmap(blockStart_, MEMORY_POOL_SIZE);
    }

    NO_COPY_SEMANTIC(BumpPointerAllocator);
    NO_MOVE_SEMANTIC(BumpPointerAllocator);

    /**
     * @brief Method allocates @param count objects of type @param T
     * @returns nullptr if the block does not have enough capacity for the requested memory size or @param count == 0,
                otherwise it returns pointer to allocated memory
     */
    template <class T = uint8_t>
    T *Allocate(size_t count)
    {
        if (count == 0)
        {
            return nullptr;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        T* next = reinterpret_cast<T*>(current_) + count;
        if (next >= blockEnd_)
        {
            return nullptr;
        }

        T* object = reinterpret_cast<T*>(current_);
        current_ = next;

        return object;
    }

    void Free()
    {
        current_ = blockStart_;
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr(void *ptr)
    {
        return ptr >= blockStart_ && ptr < blockEnd_;
    }

private:
    void *blockStart_;
    void *blockEnd_;
    void *current_;
};

#endif  // MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H
