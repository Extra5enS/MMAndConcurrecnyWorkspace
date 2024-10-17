#ifndef MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
#define MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include "base/macros.h"


template <size_t ONE_MEM_POOL_SIZE, size_t... SLOTS_SIZES>
class RunOfSlotsAllocator {
    static_assert(sizeof...(SLOTS_SIZES) != 0, "you should set slots sizes");

    template <size_t MEM_POOL_SIZE>
    class RunOfSlotsMemoryPool;

    template <size_t MEM_POOL_SIZE>
    class RunOfSlotsMemoryPool
    {
    public:
        RunOfSlotsMemoryPool(size_t slotSize, RunOfSlotsMemoryPool *nextMemoryPool) : slotSize_(slotSize), memPoolSize_(MEM_POOL_SIZE), nextMemoryPool_(nextMemoryPool)
        {
            numCells_ = memPoolSize_ / slotSize_;
            usedCells_ = new void*[numCells_] {};
        }

        ~RunOfSlotsMemoryPool()
        {
            delete[] usedCells_;
        }

        NO_MOVE_SEMANTIC(RunOfSlotsMemoryPool);
        NO_COPY_SEMANTIC(RunOfSlotsMemoryPool);

        RunOfSlotsMemoryPool *GetNextMemoryPool() const
        {
            return nextMemoryPool_;
        }

        size_t GetSlotSize() const
        {
            return slotSize_;
        }

        template <class T = uint8_t>
        T *FindMem()
        {
            size_t numRequiredCells = GetNumRequiredCells<T>();            
            size_t freeCellNum = GetFreeCellIndex(numRequiredCells);

            if (freeCellNum == numCells_)
            {
                return nullptr;
            }

            void *ptr = reinterpret_cast<void*>(&memPoolAddress_[freeCellNum * slotSize_]);
            for (size_t i = freeCellNum; i < freeCellNum + numRequiredCells; i++)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                usedCells_[i] = ptr;
            }

            return reinterpret_cast<T*>(ptr);
        }

        bool FreeAddress(void *ptr)
        {
            for (size_t i = 0; i < numCells_; i++)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (usedCells_[i] == ptr)
                {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    usedCells_[i] = nullptr;
                    return true;
                }
            }

            return false;
        }

        bool VerifyPtr(void *ptr)
        {
            auto tempPtr = reinterpret_cast<char*>(ptr);

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return ((&memPoolAddress_[0] <= tempPtr) && (tempPtr <= &memPoolAddress_[memPoolSize_ - 1]));
        }
        
    private:

        template <class T = uint8_t>
        size_t GetNumRequiredCells()
        {
            size_t numRequiredCells = sizeof(T) / slotSize_;

            if (numRequiredCells * slotSize_ < sizeof(T))
            {
                numRequiredCells++;
            }

            return numRequiredCells;
        }

        // if found return index of cell, if not found - end of array
        size_t GetFreeCellIndex(const size_t numRequiredCells)
        {
            size_t freeChain = 0;
            size_t freeCellNum = 0;

            for (; freeCellNum < numCells_; freeCellNum++)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (usedCells_[freeCellNum] != nullptr)
                {
                    freeChain = 0;
                    continue;
                }
                
                freeChain++;

                if (numRequiredCells == freeChain)
                {   
                    freeCellNum -= (numRequiredCells - 1);
                    break;
                }
            }

            return freeCellNum;
        }

        size_t slotSize_;
        size_t memPoolSize_;
        size_t numCells_;
        
        std::array<char, MEM_POOL_SIZE>memPoolAddress_ = {};
        void** usedCells_;
        
        RunOfSlotsMemoryPool *nextMemoryPool_;
    };

    template <typename SlotSize>
    static RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *ParseSlots(SlotSize slotSize)
    {
        auto last = new RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE>(slotSize, nullptr);
        return last;
    }

    template <typename SlotSize, typename... SlotsSizes>
    static RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *ParseSlots(SlotSize slotSize, SlotsSizes ... slotsSizes)
    {
        auto children = ParseSlots(slotsSizes ...);
        auto parent = new RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE>(slotSize, children);
        
        return parent;
    }

public:
    RunOfSlotsAllocator()
    {
        rootMemoryPool_ = ParseSlots(SLOTS_SIZES ...);
    }

    ~RunOfSlotsAllocator()
    {
        auto *curentMemoryPool = rootMemoryPool_;

        while (curentMemoryPool != nullptr)
        {
            RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *nextMemoryPool = curentMemoryPool->GetNextMemoryPool();
            delete curentMemoryPool;
            curentMemoryPool = nextMemoryPool;
        }
    }

    NO_MOVE_SEMANTIC(RunOfSlotsAllocator);
    NO_COPY_SEMANTIC(RunOfSlotsAllocator);

    template <class T = uint8_t>
    T *Allocate()
    {
        if (sizeof(T) == 0 || sizeof(T) > ONE_MEM_POOL_SIZE)
        {
            return nullptr;
        }

        RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *currentMemortPool = rootMemoryPool_;

        while (currentMemortPool)
        {
            if (sizeof(T) <= currentMemortPool->GetSlotSize())
            {
                return currentMemortPool->template FindMem<T>();
            }
            
            currentMemortPool = currentMemortPool->GetNextMemoryPool();
        }
        
        return static_cast<T*>(nullptr);
    }

    void Free(void *ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

        RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *currentMemoryPool = rootMemoryPool_;

        while (!(currentMemoryPool->FreeAddress(ptr)))
        {
            currentMemoryPool = currentMemoryPool->GetNextMemoryPool();
            
            if (currentMemoryPool == nullptr)
            {
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

        RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *currentMemoryPool = rootMemoryPool_;
        bool isInCurrentMemoryPool = currentMemoryPool->VerifyPtr(ptr);

        while (!isInCurrentMemoryPool)
        {
            currentMemoryPool = currentMemoryPool->GetNextMemoryPool();

            if (currentMemoryPool == nullptr)
            {
                return false;
            }

            isInCurrentMemoryPool = currentMemoryPool->VerifyPtr(ptr);
        }

        return isInCurrentMemoryPool;
    }

private:

    RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *rootMemoryPool_;
};

#endif  // MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H