#ifndef MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H
#define MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H

#include <cstddef>  // is used for size_t
#include <cstdint>
#include "base/macros.h"

template <size_t MEMORY_POOL_SIZE>
class BumpPointerAllocator {
public:
    BumpPointerAllocator()
    {
        start_ = new uint8_t[MEMORY_POOL_SIZE];

        free_ = start_;
    }

    ~BumpPointerAllocator()
    {
        delete []start_;

        start_ = free_ = nullptr;
    }

    NO_COPY_SEMANTIC(BumpPointerAllocator);
    NO_MOVE_SEMANTIC(BumpPointerAllocator);

    template <class T = uint8_t>
    T *Allocate(size_t count)
    {
        if (count == 0)
        {
            return nullptr;
        }
// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (free_ + count * sizeof(T) < start_ + MEMORY_POOL_SIZE)
        {
            uint8_t *newPtr = free_;
// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            free_ += count * sizeof(T);

            return (T *) newPtr;
        }

        return nullptr;
    }

    void Free()
    {
        free_ = start_;
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr([[maybe_unused]] void *ptr)
    {
// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return (ptr >= start_) && (ptr < (start_ + MEMORY_POOL_SIZE)) && (ptr < free_);
    }

private:
    uint8_t *start_;
    uint8_t *free_;
};

#endif  // MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H
