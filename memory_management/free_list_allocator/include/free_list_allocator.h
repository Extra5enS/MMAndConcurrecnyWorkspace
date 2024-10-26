#ifndef MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
#define MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <iostream>
#include "base/macros.h"

template <size_t ONE_MEM_POOL_SIZE>
class FreeListAllocator {
    // here we recommend you to use class MemoryPool. Use new to allocate them from heap.
    // remember, you can not use any containers with heap allocations
    class MemoryBlockHeader
    {
public:
        MemoryBlockHeader(size_t size, MemoryBlockHeader *nextFree) :
                          size_(size), nextFreeBlock_(nextFree) {}

        void SetSize(size_t size)
        {
            size_ = size;
        }

        size_t GetSize() const
        {
            return size_;
        }

        void SetNextFreeBlock(MemoryBlockHeader *nextFree)
        {
            nextFreeBlock_ = nextFree;
        }

        MemoryBlockHeader *GetNextFreeBlock() const
        {
            return nextFreeBlock_;
        }
private:
        size_t            size_           = {};
        MemoryBlockHeader *nextFreeBlock_ = nullptr;
    };

    template <size_t MEM_POOL_SIZE>
    class FreeListMemoryPool
    {
public:
        FreeListMemoryPool()
        {
            data_ = new uint8_t[MEM_POOL_SIZE];

            firstFreeBlock_ = reinterpret_cast<MemoryBlockHeader*>(data_);
            if (MEM_POOL_SIZE <= sizeof(MemoryBlockHeader))
            {
                firstFreeBlock_->SetSize(0);
            }
            else
            {
                firstFreeBlock_->SetSize(MEM_POOL_SIZE - sizeof(MemoryBlockHeader));
            }
    
            firstFreeBlock_->SetNextFreeBlock(nullptr);
        }

        ~FreeListMemoryPool()
        {
            delete [] data_;
        }

        NO_MOVE_SEMANTIC(FreeListMemoryPool);
        NO_COPY_SEMANTIC(FreeListMemoryPool);

        void *Allocate(size_t size)
        {
            MemoryBlockHeader *currentBlock = firstFreeBlock_;
            MemoryBlockHeader *previousBlock = nullptr;

            while (currentBlock != nullptr && currentBlock->GetSize() < size)
            {
                previousBlock = currentBlock;
                currentBlock = currentBlock->GetNextFreeBlock();
            }

            if (currentBlock == nullptr)
            {
                return nullptr;
            }

            MemoryBlockHeader *newFree = nullptr;
            if (currentBlock->GetSize() - sizeof(MemoryBlockHeader) > size)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newFree = reinterpret_cast<MemoryBlockHeader*>(reinterpret_cast<uint8_t*>(currentBlock) + sizeof(MemoryBlockHeader) + size);
                newFree->SetSize(currentBlock->GetSize() - sizeof(MemoryBlockHeader) - size);
                newFree->SetNextFreeBlock(currentBlock->GetNextFreeBlock());
            }
            else
            {
                newFree = currentBlock->GetNextFreeBlock();
            }

            if (firstFreeBlock_ == currentBlock)
            {
                firstFreeBlock_ = newFree;
            }

            if (previousBlock != nullptr)
            {
                previousBlock->SetNextFreeBlock(newFree);
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(currentBlock + sizeof(MemoryBlockHeader)));
        }

        void Free(void *ptr)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto *correspondBlock = reinterpret_cast<MemoryBlockHeader*>(reinterpret_cast<uint8_t*>(ptr) - sizeof(MemoryBlockHeader));
            correspondBlock = firstFreeBlock_;
            firstFreeBlock_ = correspondBlock;
        }

        bool VerifyPtr(void *ptr)
        {
            auto *ptrCast = reinterpret_cast<uint8_t*>(ptr);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return data_ <= ptrCast && ptrCast < (data_ + MEM_POOL_SIZE);
        }


private:
    MemoryBlockHeader   *firstFreeBlock_    = nullptr;
    size_t              freeMemSize_        = MEM_POOL_SIZE;
    uint8_t             *data_              = nullptr;
    };

public:
    FreeListAllocator()
    {
        pools_.push_back(new FreeListMemoryPool<ONE_MEM_POOL_SIZE> {});
    }

    ~FreeListAllocator()
    {
        for (auto &pool : pools_)
        {
            delete pool;
        }
    }

    NO_MOVE_SEMANTIC(FreeListAllocator);
    NO_COPY_SEMANTIC(FreeListAllocator);

    template <class T = uint8_t>
    T *Allocate(size_t count)
    {
        if (count == 0 || count * sizeof(T) > ONE_MEM_POOL_SIZE)
        {
            return nullptr;
        }

        for (auto &pool : pools_)
        {
            void* addr = pool->Allocate(count);
            if (addr != nullptr)
            {
                return reinterpret_cast<T*>(addr);
            }
        }

        pools_.push_back(new FreeListMemoryPool<ONE_MEM_POOL_SIZE> {});
        return reinterpret_cast<T*>(pools_.back()->Allocate(count));
    }

    void Free([[maybe_unused]] void *ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

        for (auto &pool: pools_)
        {
            if (pool->VerifyPtr(ptr))
            {
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
        for (auto &pool : pools_)
        {
            if (pool->VerifyPtr(ptr))
            {
                return true;
            }
        }

        return false;
    }

private:
    std::vector<FreeListMemoryPool<ONE_MEM_POOL_SIZE>*> pools_;
};

#endif  // MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H