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
        RunOfSlotsMemoryPool(size_t slotSize, RunOfSlotsMemoryPool *nextSlot) : slotSize_(slotSize), memPoolSize_(MEM_POOL_SIZE), nextSlot_(nextSlot)
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

        RunOfSlotsMemoryPool *GetNextSlot() const
        {
            return nextSlot_;
        }

        template <class T = uint8_t>
        T *FindMem()
        {
            if (sizeof(T) <= slotSize_)
            {
                T *findMemInThisSlot = FindMemInThisSlot<T>();

                if (findMemInThisSlot != nullptr)
                {
                    return findMemInThisSlot;
                }

                if (nextSlot_ == nullptr)
                {
                    return nullptr;
                }

                return nextSlot_->template FindMem<T>();
            }
            
            if (nextSlot_ == nullptr)
            {
                return nullptr;
            }

            return nextSlot_->template FindMem<T>();
        }

        void FreeAddress(void *ptr)
        {
            bool isClean = false;

            for (size_t i = 0; i < numCells_; i++)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (usedCells_[i] == ptr)
                {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    usedCells_[i] = nullptr;
                    isClean = true;
                }
            }

            if (isClean || nextSlot_ == nullptr)
            {
                return;
            }

            nextSlot_->FreeAddress(ptr);
        }

        bool VerifyPtr(void *ptr)
        {
            auto tempPtr = reinterpret_cast<char*>(ptr);

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if ((&memPoolAddress_[0] <= tempPtr) && (tempPtr <= &memPoolAddress_[memPoolSize_ - 1]))
            {
                return true;
            }

            if (nextSlot_ == nullptr)
            {
                return false;
            }
            
            return nextSlot_->VerifyPtr(ptr);
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

        template <class T = uint8_t>
        T *FindMemInThisSlot()
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

        size_t slotSize_;
        size_t memPoolSize_;
        size_t numCells_;
        
        std::array<char, MEM_POOL_SIZE>memPoolAddress_ = {};
        void** usedCells_;
        
        RunOfSlotsMemoryPool *nextSlot_;
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
        rootSlot_ = ParseSlots(SLOTS_SIZES ...);
    }

    ~RunOfSlotsAllocator()
    {
        auto *curentSlot = rootSlot_;

        while (curentSlot != nullptr)
        {
            auto *nextSlot = curentSlot->GetNextSlot();
            delete curentSlot;
            curentSlot = nextSlot;
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

        return rootSlot_->template FindMem<T>();
    }

    void Free(void *ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

        return rootSlot_->FreeAddress(ptr);
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

        return rootSlot_->VerifyPtr(ptr);
    }

private:

    RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *rootSlot_;
};

#endif  // MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H