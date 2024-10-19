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
        startPtr_ = new char[MEMORY_POOL_SIZE];
        freePtr_ = startPtr_;
    }

    ~BumpPointerAllocator()
    {
        delete [] startPtr_;
    }

    NO_COPY_SEMANTIC(BumpPointerAllocator);
    NO_MOVE_SEMANTIC(BumpPointerAllocator);

    template <class T = uint8_t>
    T *Allocate(size_t count)
    {
        size_t size = count * sizeof(T);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (size == 0 || freePtr_ + size > startPtr_ + MEMORY_POOL_SIZE)
        {
            return nullptr;
        }

        T *allocAddr = reinterpret_cast<T*>(freePtr_);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        freePtr_ += size;
        return allocAddr;
    }

    void Free()
    {
        freePtr_ = startPtr_;
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr(void *ptr)
    {
        char *addr = static_cast<char*>(ptr);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return startPtr_ <= addr && addr < startPtr_ + MEMORY_POOL_SIZE;
    }

private:
    char *startPtr_ = nullptr;
    char *freePtr_  = nullptr;
};

#endif  // MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H