#ifndef MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
#define MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H

#include <cstddef>
#include <new>
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
        static constexpr Block *OCCUPIED_BLOCK = reinterpret_cast<Block *>(-1);

    public:
        FreeListMemoryPool()
        {
            if (freeListHead_ == nullptr) {
                throw std::bad_alloc();
            }
            freeListHead_->size = CalculateCapacity();
            freeListHead_->next = freeListHead_ + 1;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        }

        char *Allocate(size_t size)
        {
            Block *prevBlock = nullptr;
            Block *block = freeListHead_;

            while (block && block->size + sizeof(Block) < size) {
                prevBlock = block;
                block = block->next;
            }

            if (block == nullptr) {
                return nullptr;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto allocated = reinterpret_cast<char *>(block) + sizeof(Block);

            Block *nextFreeBlock = block->next;

            if (block->size < size + sizeof(Block)) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                nextFreeBlock = reinterpret_cast<Block *>(allocated + size);
                nextFreeBlock->size = block->size - size - sizeof(Block);
                nextFreeBlock->next = block->next;
            }

            if (prevBlock) {
                prevBlock->next = nextFreeBlock;
            }

            if (freeListHead_ == block) {
                freeListHead_ = nextFreeBlock;
            }

            block->next = OCCUPIED_BLOCK;
            block->size = size + sizeof(Block);

            return allocated;
        }

        void Free(void *ptr) noexcept
        {
            if (ptr == nullptr) {
                return;
            }

            if (!VerifyPtr(ptr)) {
                return;
            }

            Block *block = reinterpret_cast<Block *>(ptr);

            block->next = freeListHead_;
            freeListHead_ = block;
        }

        bool VerifyPtr(const void *ptr) const noexcept
        {
            if (ptr == nullptr) {
                return false;
            }

            auto block = reinterpret_cast<Block *>(ptr);

            if (block->next != OCCUPIED_BLOCK) {
                return false;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return base_ <= ptr && reinterpret_cast<const char *>(ptr) + sizeof(Block) < upperBound_;
        }

    private:
        // NOLINTNEXTLINE(modernize-use-nullptr)
        void *base_ = mmap(NULL, CalculateCapacity(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        const void *const upperBound_ = base_ + CalculateCapacity();
        Block *freeListHead_ = reinterpret_cast<Block *>(base_);

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
