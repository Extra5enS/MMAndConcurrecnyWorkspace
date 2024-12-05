#ifndef MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H
#define MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include "base/macros.h"

template <size_t ONE_MEM_POOL_SIZE>
class FreeListAllocator {

    template <size_t MEM_POOL_SIZE>
    class FreeListMemoryPool {
        class FreeMemoryBlock
        {
        public:
            FreeMemoryBlock(FreeMemoryBlock *nextBlock, size_t size) : 
                            nextBlock_(nextBlock), 
                            size_(size) {}

            FreeMemoryBlock *GetNextBlock()
            {
                return nextBlock_;
            }

            void SetNextBlock(FreeMemoryBlock *nextBlock)
            {
                nextBlock_ = nextBlock;
            }

            size_t GetSize() const
            {
                return size_;
            }

            void SetSize(size_t size)
            {
                size_ = size;
            }

        private:
            FreeMemoryBlock *nextBlock_ = nullptr;
            size_t size_ = 0;
        };

    public: // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        FreeListMemoryPool()
        {
            if (MEM_POOL_SIZE <= sizeof(FreeMemoryBlock))
            {
                return;
            }

            rootBlock_ = reinterpret_cast<FreeMemoryBlock*>(&memPool_[0]);
            rootBlock_->SetSize(MEM_POOL_SIZE - sizeof(FreeMemoryBlock));
            rootBlock_->SetNextBlock(nullptr);
        }
        
        ~FreeListMemoryPool() = default;

        NO_MOVE_SEMANTIC(FreeListMemoryPool);
        NO_COPY_SEMANTIC(FreeListMemoryPool);

        template <class T = uint8_t>
        T *Allocate(size_t count)
        {
            size_t requiredMem = count * sizeof(T) + sizeof(size_t);

            FreeMemoryBlock *prevBlock = nullptr;
            FreeMemoryBlock *curBlock = rootBlock_;

            while (curBlock->GetSize() < requiredMem)
            {
                prevBlock = curBlock;
                curBlock = curBlock->GetNextBlock();
                
                if (curBlock == nullptr)
                {
                    return nullptr;
                }
            }

            UpdateBlockOnAllocation(curBlock, prevBlock, requiredMem);

            auto header = reinterpret_cast<size_t*>(curBlock);
            *header = requiredMem;
            
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return reinterpret_cast<T*>(header + 1);
        }

        void Free(void *ptr)
        {   // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto ptrToHeader = reinterpret_cast<char*>(ptr) - sizeof(size_t);
            size_t size = *reinterpret_cast<size_t*>(ptrToHeader);

            auto newBlock = reinterpret_cast<FreeMemoryBlock*>(ptrToHeader);
            newBlock->SetNextBlock(rootBlock_);
            newBlock->SetSize(size);
            
            rootBlock_ = newBlock;
        }
        
        bool VerifyPtr(void *ptr)
        {
            auto charPtr = reinterpret_cast<char*>(ptr);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return (&memPool_[0] <= charPtr && charPtr < &memPool_[0] + MEM_POOL_SIZE);
        }

    private:

        void UpdateBlockOnAllocation(FreeMemoryBlock *curBlock, FreeMemoryBlock *prevBlock, size_t requiredMem)
        {
            if (curBlock->GetSize() == requiredMem)
            {
                if (prevBlock)
                {
                    prevBlock->SetNextBlock(curBlock->GetNextBlock());
                }
                else
                {
                    rootBlock_ = curBlock->GetNextBlock();
                }
            }
            else
            {
                auto ptr = reinterpret_cast<char*>(curBlock);

                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                auto changeCurBlock = reinterpret_cast<FreeMemoryBlock*>(ptr + requiredMem);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                changeCurBlock->SetSize(curBlock->GetSize() - requiredMem);
                changeCurBlock->SetNextBlock(curBlock->GetNextBlock());
                
                if (prevBlock)
                {
                    prevBlock->SetNextBlock(changeCurBlock);
                }
                else
                {
                    rootBlock_ = changeCurBlock;
                }
            }
        }

        std::array<char, MEM_POOL_SIZE> memPool_ = {};
        FreeMemoryBlock *rootBlock_ = nullptr;
    }; // class FreeListMemoryPool

public:
    FreeListAllocator() : memPools_(1, new FreeListMemoryPool<ONE_MEM_POOL_SIZE>()) {}
    ~FreeListAllocator()
    {
        for (auto currentMemPool : memPools_)
        {
            delete currentMemPool;
        }
    }

    NO_MOVE_SEMANTIC(FreeListAllocator);
    NO_COPY_SEMANTIC(FreeListAllocator);

    template <class T = uint8_t>
    T *Allocate([[maybe_unused]]size_t count)
    {
        if (count == 0 || count * sizeof(T) + sizeof(size_t) > ONE_MEM_POOL_SIZE)
        {
            return nullptr;
        }

        for (auto currentMemPool : memPools_)
        {
            T *ptr = currentMemPool->template Allocate<T>(count);

            if (ptr != nullptr)
            {
                return ptr;
            }
        }

        auto nextMemPool = new FreeListMemoryPool<ONE_MEM_POOL_SIZE>{};
        memPools_.push_back(nextMemPool);

        return nextMemPool->template Allocate<T>(count);
    }

    void Free([[maybe_unused]]void *ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

        for (auto currentMemPool : memPools_)
        {
            if (currentMemPool->VerifyPtr(ptr))
            {
                currentMemPool->Free(ptr);
                return;
            }
        }
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
      */
    bool VerifyPtr(void *ptr)
    {
        if (ptr == nullptr)
        {
            return false;
        }

        for (auto currentMemPool : memPools_)
        {
            if (currentMemPool->VerifyPtr(ptr))
            {
                return true;
            }
        }

        return false;
    }

private:
    std::vector<FreeListMemoryPool<ONE_MEM_POOL_SIZE>*> memPools_;
};

#endif  // MEMORY_MANAGEMENT_FREE_LIST_ALLOCATOR_INCLUDE_FREE_LIST_ALLOCATOR_H