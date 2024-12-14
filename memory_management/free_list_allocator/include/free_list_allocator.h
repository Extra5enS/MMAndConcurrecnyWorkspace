#ifndef MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
#define MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H

#include <cstddef>
#include <sys/mman.h>
#include <new>
#include <cstdint>
#include "base/macros.h"

template <size_t ONE_MEM_POOL_SIZE>
class FreeListAllocator {
    struct FreeListMemoryPool {
        struct FreeBlockHeader {
            size_t size;
            FreeBlockHeader *next;
        };

        struct OccupiedBlockHeader {
            size_t size;
        };

        static constexpr FreeBlockHeader *OCCUPIED_BLOCK = nullptr;
        static constexpr size_t TOO_LARGE = static_cast<size_t>(-1);

        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        FreeListMemoryPool *nextPool = nullptr;
        // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
        FreeBlockHeader *freeListHead =
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            reinterpret_cast<FreeBlockHeader *>(reinterpret_cast<FreeListMemoryPool *>(this + 1));


        FreeListMemoryPool() noexcept
        {
            freeListHead->size = ONE_MEM_POOL_SIZE;
            freeListHead->next = nullptr;
        }

        char *Allocate(size_t size) noexcept
        {
            if (size == 0) {
                return nullptr;
            }

            if (size + sizeof(OccupiedBlockHeader) > ONE_MEM_POOL_SIZE) {
                return reinterpret_cast<char *>(TOO_LARGE);
            }

            FreeBlockHeader *prevBlock = nullptr;
            FreeBlockHeader *block = freeListHead;

            while (block && block->size < size) {
                prevBlock = block;
                block = block->next;
            }

            if (block == nullptr) {
                return nullptr;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto allocated = reinterpret_cast<char *>(block) + sizeof(OccupiedBlockHeader);

            FreeBlockHeader *nextFreeBlock = block->next;

            if (block->size >= size + sizeof(FreeBlockHeader)) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                ptrdiff_t shift = GetBlockPtrAlignmentShift(allocated + size);

                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                nextFreeBlock = reinterpret_cast<FreeBlockHeader *>(allocated + size + shift);
                nextFreeBlock->size = block->size - size - shift;
                nextFreeBlock->next = block->next;
            }

            if (prevBlock) {
                prevBlock->next = nextFreeBlock;
            }

            if (freeListHead == block) {
                freeListHead = nextFreeBlock;
            }

            reinterpret_cast<OccupiedBlockHeader *>(block)->size = size;

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

            auto block = reinterpret_cast<FreeBlockHeader *>(ptr);

            block->next = freeListHead;
            freeListHead = block;
        }

        bool VerifyPtr(const void *ptr) const noexcept
        {
            if (ptr == nullptr) {
                return false;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto base = reinterpret_cast<const char *>(this) + sizeof(FreeListAllocator);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return base + sizeof(OccupiedBlockHeader) <= ptr &&
                   // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                   reinterpret_cast<const char *>(ptr) < base + ONE_MEM_POOL_SIZE;
        }

        static ptrdiff_t GetBlockPtrAlignmentShift(const char *ptr) noexcept

        {
            auto szptr = reinterpret_cast<uintptr_t >(ptr);
            uintptr_t mod = szptr % alignof(FreeBlockHeader);

            if (mod == 0) {
                return 0;
            }

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
        MemPool *pool = firstPool_;
        while (pool) {
            MemPool *old = pool;
            pool = pool->nextPool;
            munmap(old, ONE_MEM_POOL_SIZE);
        }
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
        if (allocated == reinterpret_cast<char *>(MemPool::TOO_LARGE)) {
            return nullptr;
        }

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

        while (pool) {
            if (pool->VerifyPtr(ptr)) {
                pool->Free(ptr);
                break;
            }
            pool = pool->nextPool;
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
            if (pool->VerifyPtr(ptr)) {
                return true;
            }
            pool = pool->nextPool;
        }
        return false;
    }

private:
    using MemPool = FreeListMemoryPool;
    MemPool *firstPool_ = CreateNewPool();

    static MemPool *CreateNewPool() noexcept
    {
        auto newPool = reinterpret_cast<MemPool *>(mmap(
            // NOLINTNEXTLINE(modernize-use-nullptr)
            NULL, ONE_MEM_POOL_SIZE + sizeof(MemPool), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        new (newPool) MemPool();
        return newPool;
    }
};

#endif  // MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
