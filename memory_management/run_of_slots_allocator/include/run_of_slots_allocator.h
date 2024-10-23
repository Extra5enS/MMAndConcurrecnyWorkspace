#ifndef MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
#define MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include <bitset>
#include "base/macros.h"


template <size_t ONE_MEM_POOL_SIZE, size_t... SLOTS_SIZES>
class RunOfSlotsAllocator {
    static_assert(sizeof...(SLOTS_SIZES) != 0, "you should set slots sizes");

    // NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
    class RunOfSlotsMemoryPoolInterface
    {
    public:
        virtual ~RunOfSlotsMemoryPoolInterface() = default;
        
        virtual void *FindMem() = 0;
        virtual bool FreeAddress(void *ptr) = 0;
        virtual bool VerifyPtr(void *ptr) = 0;
    };

    template <size_t MEM_POOL_SIZE, size_t SLOT_SIZE>
    class RunOfSlotsMemoryPool : public RunOfSlotsMemoryPoolInterface
    {
    public:
        RunOfSlotsMemoryPool() : numSlots_(MEM_POOL_SIZE / SLOT_SIZE) {}
        ~RunOfSlotsMemoryPool() override = default;

        NO_MOVE_SEMANTIC(RunOfSlotsMemoryPool);
        NO_COPY_SEMANTIC(RunOfSlotsMemoryPool);

        size_t GetSlotSize() const
        {
            return SLOT_SIZE;
        }
        
        void *FindMem() final
        {
            size_t freeSlotNum = 0;
            for (; freeSlotNum < numSlots_; freeSlotNum++)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (bitmap_[freeSlotNum] == 0b0)
                {
                    break;
                }
            }

            if (freeSlotNum == numSlots_)
            {
                return nullptr;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            bitmap_[freeSlotNum] = 0b1;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return reinterpret_cast<void*>(&memPoolAddress_[freeSlotNum * SLOT_SIZE]);
        }

        bool FreeAddress(void *ptr) final
        {
            if (!VerifyPtr(ptr))
            {
                return false;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            size_t removableSlot = (reinterpret_cast<char*>(ptr) - &memPoolAddress_[0]) / SLOT_SIZE;
            bitmap_[removableSlot] = 0b0;

            return true;
        }

        bool VerifyPtr(void *ptr) final
        {
            auto tempPtr = reinterpret_cast<char*>(ptr);

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return ((&memPoolAddress_[0] <= tempPtr) && (tempPtr <= &memPoolAddress_[MEM_POOL_SIZE - 1]));
        }
        
    private:

        size_t numSlots_;
        std::bitset<MEM_POOL_SIZE / SLOT_SIZE>bitmap_  = {};
        std::array<char, MEM_POOL_SIZE>memPoolAddress_ = {};
    };

public:
    RunOfSlotsAllocator()
    {
        ([&]{
            memoryPools_.push_back(new RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE, SLOTS_SIZES>{});
        }(), ...);
    }

    ~RunOfSlotsAllocator()
    {
        for (auto& currentMemoryPool : memoryPools_)
        {
            delete currentMemoryPool;
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

        size_t memoryPoolNum = GetSuitablePoolNum(sizeof(T));

        if (memoryPoolNum >= memoryPools_.size())
        {
            return nullptr;
        }

        return reinterpret_cast<T*>(memoryPools_[memoryPoolNum]->FindMem());
    }

    void Free([[maybe_unused]] void *ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

        for (auto& currentMemoryPool : memoryPools_)
        {
            if (currentMemoryPool->FreeAddress(ptr))
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

        for (auto& currentMemoryPool : memoryPools_)
        {
            if (currentMemoryPool->VerifyPtr(ptr))
            {
                return true;
            }
        }

        return false;
    }

private:

    size_t GetSuitablePoolNum(size_t size)
    {
        size_t memoryPoolNum = 0;
        while (size != 0)
        {
            size /= 2;
            memoryPoolNum++;
        }

        return memoryPoolNum - 1;
    }

    std::vector<RunOfSlotsMemoryPoolInterface*> memoryPools_;
};

#endif  // MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H