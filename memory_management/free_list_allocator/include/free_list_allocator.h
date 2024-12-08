#ifndef MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
#define MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include "base/macros.h"

template <size_t ONE_MEM_POOL_SIZE>
class FreeListAllocator {

    template <size_t MEM_POOL_SIZE>
    class FreeListMemoryPool;

    class ObjectHeader {
    public:
        void MakeObject(size_t newSize)
        {
            if (newSize == objectSize_) {
                return;
            }

            auto *prevFree = nextFree_;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            nextFree_ = reinterpret_cast<ObjectHeader*>(reinterpret_cast<char*>(this) + newSize);
            nextFree_->objectSize_ = objectSize_ - newSize;
            nextFree_->nextFree_ = prevFree;
            objectSize_ = newSize;
        }

        template <size_t SIZE>
        friend class FreeListMemoryPool;

    private:
        size_t objectSize_;
        ObjectHeader *nextFree_;
    };

    template <size_t MEM_POOL_SIZE>
    class FreeListMemoryPool {

    public:
        FreeListMemoryPool()
        {
            if (sizeof(ObjectHeader) >= MEM_POOL_SIZE) {
                free_ = nullptr;
                return;
            }
            free_ = static_cast<ObjectHeader*>(poolBase_);
            free_->nextFree_ = nullptr;
            free_->objectSize_ = MEM_POOL_SIZE;
        }

        NO_COPY_SEMANTIC(FreeListMemoryPool);
        NO_MOVE_SEMANTIC(FreeListMemoryPool);

        ~FreeListMemoryPool()
        {
            delete [] static_cast<char*>(poolBase_);
        }

        template <class T = uint8_t>
        T *Allocate(size_t count)
        {
            if (free_ == nullptr) {
                return nullptr;
            }

            size_t newSize = count + sizeof(ObjectHeader);
            if (newSize > freeSize_) {
                return nullptr;
            }

            ObjectHeader *current = free_;
            while (newSize > current->objectSize_) {
                current = current->nextFree_;
            }

            if (!current) {
                return nullptr;
            }

            current->MakeObject(newSize);
            free_ = current->nextFree_;
            freeSize_ -= newSize;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return reinterpret_cast<T*>(reinterpret_cast<char*>(current) + sizeof(ObjectHeader));
        }

        bool VerifyPtr(void *ptr)
        {
            return ptr >= poolBase_ && ptr < poolEnd_;
        }

        void Free(void *ptr)
        {
            auto *prevFree = free_;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            free_ = reinterpret_cast<ObjectHeader*>(static_cast<char*>(ptr) - sizeof(ObjectHeader));
            free_->nextFree_ = prevFree;
            freeSize_ += free_->objectSize_;
        }

    private:
        void *poolBase_  = new char[MEM_POOL_SIZE];
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        void *poolEnd_   = static_cast<char*>(poolBase_) + MEM_POOL_SIZE;
        size_t freeSize_ = MEM_POOL_SIZE;
        ObjectHeader *free_;
    };

public:
    FreeListAllocator() = default;
    ~FreeListAllocator()
    {
        for (auto *pool: pools_) {
            delete pool;
        }
    }

    NO_MOVE_SEMANTIC(FreeListAllocator);
    NO_COPY_SEMANTIC(FreeListAllocator);

    template <class T = uint8_t>
    T *Allocate(size_t count)
    {
        if (count >= ONE_MEM_POOL_SIZE) {
            return nullptr;
        }

        T *object = nullptr;
        for (auto *pool : pools_) {
            object = pool->template Allocate<T>(count);
            if (object) {
                return object;
            }
        }

        auto *newPool = new FreeListMemoryPool<ONE_MEM_POOL_SIZE>{};
        pools_.push_back(newPool);
        object = newPool->template Allocate<T>(count);

        return object;
    }

    void Free(void *ptr)
    {
        for (auto *pool: pools_) {
            if (pool->VerifyPtr(ptr)) {
                pool->Free(ptr);
            }
        }
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr(void *ptr)
    {
        for (auto *pool: pools_) {
            if (pool->VerifyPtr(ptr)) {
                return true;
            }
        }

        return false;
    }

private:
    std::vector<FreeListMemoryPool<ONE_MEM_POOL_SIZE>*> pools_;
};

#endif  // MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
