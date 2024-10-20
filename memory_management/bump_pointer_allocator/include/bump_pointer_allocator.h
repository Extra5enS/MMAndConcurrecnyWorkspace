#ifndef MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H
#define MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H

#include <cstddef>  // is used for size_t
#include <new>
#include <unistd.h>
#include <sys/mman.h>
#include "base/macros.h"

template <size_t MEMORY_POOL_SIZE>
class BumpPointerAllocator {
public:
    BumpPointerAllocator()
    {
        if (base_ == nullptr) {
            throw std::bad_alloc();
        }
    }

    ~BumpPointerAllocator() noexcept
    {
        munmap(base_, MEMORY_POOL_SIZE);
    }

    NO_COPY_SEMANTIC(BumpPointerAllocator);
    NO_MOVE_SEMANTIC(BumpPointerAllocator);

    template <class T = char>
    T *Allocate(size_t count) noexcept
    {
        if (count == 0) {
            return nullptr;
        }

        size_t size = count * sizeof(T);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (cur_ + size > GetUpperBound()) {
            return nullptr;
        }

        T *allocated = reinterpret_cast<T *>(cur_);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        cur_ += size;

        return allocated;
    }

    void Free() noexcept
    {
        cur_ = base_;
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr(void *ptr) noexcept
    {
        return base_ <= ptr && ptr < GetUpperBound();
    }

private:
    char *base_ = reinterpret_cast<char *>(
        // NOLINTNEXTLINE(modernize-use-nullptr)
        mmap(NULL, MEMORY_POOL_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));

    char *cur_ = base_;

    char *GetUpperBound() const noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return base_ + MEMORY_POOL_SIZE;
    }
};

#endif  // MEMORY_MANAGEMENT_BUMP_POINTER_ALLOCATOR_INCLUDE_BUMP_POINTER_ALLOCATOR_H
