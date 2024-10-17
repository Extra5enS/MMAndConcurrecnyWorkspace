#ifndef MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
#define MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H

#include <cstddef>
#include <new>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#include "base/macros.h"

#include "Error.hpp"

template <size_t ONE_MEM_POOL_SIZE>
class FreeListAllocator {
    // here we recommend you to use class MemoryPool. Use new to allocate them from heap.
    // remember, you can not use any containers with heap allocations
    class FreeListMemoryPool {
        friend class FreeListAllocator;

        struct FreeBlockHeader {
            size_t size;
            FreeBlockHeader *next;
        };

        struct OccupiedBlockHeader {
            size_t size;
        };

        static constexpr FreeBlockHeader *OCCUPIED_BLOCK = nullptr;

    public:
        FreeListMemoryPool() noexcept
        {
            freeListHead_->size = CalculateCapacity() - sizeof(FreeListMemoryPool);
            freeListHead_->next = nullptr;
        }

        char *Allocate(size_t size) noexcept
        {
            FreeBlockHeader *prevBlock = nullptr;
            FreeBlockHeader *block = freeListHead_;

            LOG("----------------------------------\n");
            LOG("Start allocation of %zu bytes.\n", size);

            while (block && block->size < size) {
                prevBlock = block;
                block = block->next;
                LOG("block = %p\n", block);
            }

            LOG("Working with block[%p]\n"
                "size = %zu\n"
                "next = %p\n",
                block, block->size, block->next);

            LOG("\n");

            if (block == nullptr) {
                return nullptr;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto allocated = reinterpret_cast<char *>(block) + sizeof(OccupiedBlockHeader);

            LOG("Allocated %p\n", allocated);

            FreeBlockHeader *nextFreeBlock = block->next;

            if (block->size >= size + sizeof(FreeBlockHeader)) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                LOG("This block has spare space. Making a new block here.\n");
                ptrdiff_t shift = GetBlockPtrAlignmentShift(allocated + size);

                nextFreeBlock = reinterpret_cast<FreeBlockHeader *>(allocated + size + shift);
                nextFreeBlock->size = block->size - size - shift;
                nextFreeBlock->next = block->next;
            }

            LOG("nextFreeBlock %p\n", nextFreeBlock);

            if (prevBlock) {
                prevBlock->next = nextFreeBlock;
            }

            if (freeListHead_ == block) {
                freeListHead_ = nextFreeBlock;
                LOG("New freeListHead_ = %p\n", freeListHead_);
            }

            reinterpret_cast<OccupiedBlockHeader *>(block)->size = size;

            LOG("Allocation successfull!\n");
            LOG("----------------------------------\n");

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

            LOG("----------------------------------\n");
            LOG("Free ptr[%p]\n", ptr);

            FreeBlockHeader *block = reinterpret_cast<FreeBlockHeader *>(ptr);

            block->next = freeListHead_;
            freeListHead_ = block;

            LOG("Working with block[%p]\n"
                "size = %zu\n"
                "next = %p\n",
                block, block->size, block->next);

            LOG("freeListHead_[%p]\n"
                "size = %zu\n"
                "next = %p\n",
                freeListHead_, freeListHead_->size, freeListHead_->next);

            LOG("----------------------------------\n");
        }

        bool VerifyPtr(const void *ptr) const noexcept
        {
            if (ptr == nullptr) {
                return false;
            }

            auto block = reinterpret_cast<const FreeBlockHeader *>(ptr);

            if (block->next != OCCUPIED_BLOCK) {
                return false;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto base = reinterpret_cast<const char *>(this);
            return this <= ptr &&
                   reinterpret_cast<const char *>(ptr) + sizeof(FreeBlockHeader) < base + CalculateCapacity();
        }

    private:
        // NOLINTNEXTLINE(modernize-use-nullptr)
        FreeListMemoryPool *nextPool = nullptr;
        FreeBlockHeader *freeListHead_ = reinterpret_cast<FreeBlockHeader *>(this + sizeof(FreeBlockHeader));

        static ptrdiff_t GetBlockPtrAlignmentShift(const char *ptr) noexcept
        {
            uintptr_t szptr = reinterpret_cast<size_t>(ptr);
            uintptr_t mod = szptr % alignof(FreeBlockHeader);

            if (mod == 0)
                return 0;

            return alignof(FreeBlockHeader) - mod;
        }
    };

public:
    FreeListAllocator()
    {
        if (firstPool_ == nullptr) {
            throw std::bad_alloc();
        }
    }

    ~FreeListAllocator() noexcept
    {
        munmap(firstPool_, CalculateCapacity());
    }

    NO_MOVE_SEMANTIC(FreeListAllocator);
    NO_COPY_SEMANTIC(FreeListAllocator);

    template <class T = char>
    T *Allocate(size_t count) noexcept
    {
        if (count == 0) {
            return nullptr;
        }

        MemPool *pool = firstPool_;

        size_t size = count * sizeof(T);

        char *allocated = pool->Allocate(size);

        while (allocated == nullptr) {
            MemPool *prevPool = pool;
            pool = pool->nextPool;

            if (pool == nullptr) {
                pool = CreateNewPool();
                prevPool->nextPool = pool;
            }

            allocated = pool->Allocate(size);
        }

        return reinterpret_cast<T *>(allocated);
    }

    void Free(void *ptr) noexcept
    {
        MemPool *pool = firstPool_;

        LOG("BIG FREE\n");

        while (pool) {
            pool = pool->nextPool;
            if (pool->VerifyPtr(ptr)) {
                pool->Free(ptr);
                break;
            }
        }
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr(void *ptr)
    {
        if (ptr == nullptr) {
            return false;
        }

        MemPool *pool = firstPool_;

        while (pool) {
            if (pool->VerifyPtr(ptr))
                return true;
            pool = pool->nextPool;
        }
        return false;
    }

private:
    using MemPool = FreeListMemoryPool;
    MemPool *firstPool_ = CreateNewPool();

    static MemPool *CreateNewPool() noexcept
    {
        MemPool *newPool = reinterpret_cast<MemPool *>(
            mmap(NULL, CalculateCapacity(), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        new (newPool) MemPool();
        return newPool;
    }

    static size_t CalculateCapacity() noexcept
    {
        size_t psize = getpagesize();
        size_t cap = psize * (1 + ONE_MEM_POOL_SIZE / psize);
        if (ONE_MEM_POOL_SIZE % psize == 0) {
            cap -= psize;
        }
        return cap;
    }
};

#endif  // MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
