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
            FreeMemoryBlock(size_t size, char *ptr, FreeMemoryBlock *nextBlock, FreeMemoryBlock *prevBlock) : 
                            size_(size), 
                            ptrToMem_(ptr), 
                            nextBlock_(nextBlock),
                            prevBlock_(prevBlock) {}

            char *GetPtr()
            {
                return ptrToMem_;
            }

            void SetPtr(char *ptr)
            {
                ptrToMem_ = ptr;
            }

            FreeMemoryBlock *GetNextBlock()
            {
                return nextBlock_;
            }

            void SetNextBlock(FreeMemoryBlock *nextBlock)
            {
                nextBlock_ = nextBlock;
            }

            FreeMemoryBlock *GetPrevBlock()
            {
                return prevBlock_;
            }

            void SetPrevBlock(FreeMemoryBlock *prevBlock)
            {
                prevBlock_ = prevBlock;
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
            size_t size_ = 0;
            char *ptrToMem_ = nullptr;
            FreeMemoryBlock *nextBlock_ = nullptr;
            FreeMemoryBlock *prevBlock_ = nullptr;
        };

    public: // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        FreeListMemoryPool() : beginMemPool_(&memPool_[0]), endMemPool_(&memPool_[0] + MEM_POOL_SIZE)
        {    
            rootBlock_ = new FreeMemoryBlock{MEM_POOL_SIZE, beginMemPool_, nullptr, nullptr};
        }
        
        ~FreeListMemoryPool() 
        {
            FreeMemoryBlock *curBlock = rootBlock_;
            FreeMemoryBlock *nextBlock = nullptr;
            
            do
            {
                nextBlock = curBlock->GetNextBlock();
                delete curBlock;
                curBlock = nextBlock;
            } while (curBlock != nullptr);
        }

        NO_MOVE_SEMANTIC(FreeListMemoryPool);
        NO_COPY_SEMANTIC(FreeListMemoryPool);

        template <class T = uint8_t>
        T *Allocate(size_t count)
        {
            size_t requiredMem = count * sizeof(T) + sizeof(size_t);

            auto freeBlock = FindFreeBlock(requiredMem);

            if (freeBlock == nullptr)
            {
                return nullptr;
            }

            auto header = reinterpret_cast<size_t*>(freeBlock->GetPtr());
            *header = requiredMem - sizeof(size_t);

            UpdateBlocksOnAllocation(requiredMem, freeBlock);

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return reinterpret_cast<T*>(header + 1);
        }

        void Free(void *ptr)
        {   // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto ptrToHeader = reinterpret_cast<char*>(ptr) - sizeof(size_t);

            auto curBlock = FindBlockAfterPtr(ptrToHeader);

            if (curBlock == nullptr)
            {
                return;
            }

            UpdateBlocksOnRelease(ptrToHeader, curBlock);
        }
        
        bool VerifyPtr(void *ptr)
        {
            auto charPtr = reinterpret_cast<char*>(ptr);
            return (beginMemPool_ <= charPtr && charPtr < endMemPool_);
        }

    private:
        inline FreeMemoryBlock *FindFreeBlock(size_t requiredMem)
        {
            auto curBlock = rootBlock_;
            
            while (curBlock->GetSize() < requiredMem)
            {
                curBlock = curBlock->GetNextBlock();
                
                if (curBlock == nullptr)
                {
                    return nullptr;
                }
            }

            return curBlock;
        }

        // NOLINTNEXTLINE(readability-non-const-parameter)
        inline FreeMemoryBlock *FindBlockAfterPtr(char* const ptrToHeader)
        {
            auto curBlock = rootBlock_;
            size_t size = *reinterpret_cast<size_t*>(ptrToHeader);

            while (ptrToHeader > curBlock->GetPtr())
            {
                auto nextBlock = curBlock->GetNextBlock();
                
                if (nextBlock == nullptr)
                {   // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    if (curBlock->GetPtr() + curBlock->GetSize() != ptrToHeader)
                    {
                        assert(0);
                    }

                    curBlock->SetSize(curBlock->GetSize() + size);
                    
                    return nullptr;
                }
                
                curBlock = nextBlock;
            }

            return curBlock;
        }

        inline void UpdateBlocksOnAllocation(size_t requiredMem, FreeMemoryBlock *freeBlock)
        {
            if (freeBlock->GetSize() == requiredMem)
            {
                if (rootBlock_ == freeBlock)
                {
                    rootBlock_ = freeBlock->GetPrevBlock();
                    rootBlock_->SetPrevBlock(nullptr);
                }

                auto prevBlock = freeBlock->GetPrevBlock();
                auto nextBlock = freeBlock->GetNextBlock();

                prevBlock->SetNextBlock(nextBlock);
                
                if (nextBlock)
                {
                    nextBlock->SetPrevBlock(prevBlock);
                }
            }
            else
            {   // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                freeBlock->SetPtr(freeBlock->GetPtr() + requiredMem);
                freeBlock->SetSize(freeBlock->GetSize() - requiredMem);
            }
        }

        inline void UpdateBlocksOnRelease(char* const ptrToHeader, FreeMemoryBlock *curBlock)
        {
            size_t size = *reinterpret_cast<size_t*>(ptrToHeader);

            auto prevBlock = curBlock->GetPrevBlock();

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (ptrToHeader + size == curBlock->GetPtr())
            {
                curBlock->SetPtr(ptrToHeader);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                curBlock->SetSize(curBlock->GetSize() + size);

                if (prevBlock == nullptr)
                {
                    return;
                }

                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (prevBlock->GetPtr() + prevBlock->GetSize() == ptrToHeader)
                {   // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    prevBlock->SetSize(prevBlock->GetSize() + curBlock->GetSize());

                    auto nextBlock = curBlock->GetNextBlock();
                    prevBlock->SetNextBlock(nextBlock);
                    nextBlock->SetPrevBlock(prevBlock);

                    delete curBlock;
                    return;
                }
            }
            else
            {
                if (prevBlock == nullptr)
                {
                    auto newBlock = new FreeMemoryBlock{size, ptrToHeader, curBlock, nullptr};
                    rootBlock_ = newBlock;
                    curBlock->SetPrevBlock(newBlock);

                    return;
                }

                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (prevBlock->GetPtr() + prevBlock->GetSize() == ptrToHeader)
                {
                    prevBlock->SetSize(prevBlock->GetSize() + size);
                    return;
                }
            }
        }

        std::array<char, MEM_POOL_SIZE> memPool_ = {};
        char *beginMemPool_ = nullptr;
        char *endMemPool_ = nullptr;

        FreeMemoryBlock *rootBlock_;
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