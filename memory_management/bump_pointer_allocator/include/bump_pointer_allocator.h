#ifndef MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H
#define MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H

#include <cstddef>  // is used for size_t
#include <cstdint>
#include "base/macros.h"

template <size_t MEMORY_POOL_SIZE>
class BumpPointerAllocator {
public:
    BumpPointerAllocator() : startPointer_(new char[MEMORY_POOL_SIZE]) 
    {
        freePointer_ = startPointer_;
    }

    ~BumpPointerAllocator()
    {
        delete[] startPointer_;
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

        size_t requiredMemSize = count * sizeof(T);

        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (freePointer_ - startPointer_ + requiredMemSize > MEMORY_POOL_SIZE)
        {
            return nullptr;
        }

        T *allocatedMemoryAddress = reinterpret_cast<T*>(freePointer_);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        freePointer_ += requiredMemSize;

        return allocatedMemoryAddress;
    }

    void Free()
    {
        freePointer_ = startPointer_;
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr(void *ptr)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return (startPointer_ <= static_cast<char*>(ptr) && static_cast<char*>(ptr) < startPointer_ + MEMORY_POOL_SIZE);
    }

private:
    char *freePointer_ = nullptr;
    char *startPointer_ = nullptr;
};

#endif  // MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H