#ifndef MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
#define MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <iostream>
#include "base/macros.h"

template <size_t ONE_MEM_POOL_SIZE, size_t... SLOTS_SIZES>
class RunOfSlotsAllocator {
    static_assert(sizeof...(SLOTS_SIZES) != 0, "you should set slots sizes");

    template <size_t MEM_POOL_SIZE>
    class RunOfSlotsMemoryPool
    {
    public:
        explicit RunOfSlotsMemoryPool(size_t slotSize)
        {
            size_t offset = MEM_POOL_SIZE / slotSize / 8;
            if (offset == 0)
            {
                offset = 8;
            }
            pool_ = new uint8_t[offset + MEM_POOL_SIZE]{};
            slotSize_ = slotSize;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            data_ = pool_ + offset;
            bitmap_ = pool_;
        }

        ~RunOfSlotsMemoryPool()
        {
            delete [] pool_;
        }

        NO_MOVE_SEMANTIC(RunOfSlotsMemoryPool);
        NO_COPY_SEMANTIC(RunOfSlotsMemoryPool);

        void *Allocate()
        {
            size_t numSlots = MEM_POOL_SIZE / slotSize_;
            for (size_t numSlot = 0; numSlot < numSlots; numSlot++)
            {
                if (GetBit(numSlot) == 0b0)
                {
                    SetBit(numSlot, 0b1);
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    return reinterpret_cast<void*>(data_ + numSlot * slotSize_);
                }
            }

            return nullptr;
        }

        bool Free([[maybe_unused]] void *ptr)
        {
            if (!VerifyPtr(ptr))
            {
                return false;
            }

            size_t numSlot = (reinterpret_cast<uint8_t*>(ptr) - data_) / slotSize_;
            SetBit(numSlot, 0b0);
            return true;
        }

        bool VerifyPtr(void *ptr)
        {
            auto *ptrCast = reinterpret_cast<uint8_t*>(ptr);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return data_ <= ptrCast && ptrCast < (data_ + MEM_POOL_SIZE);
        }

    private:
        size_t slotSize_ = 0;
        uint8_t *pool_ = nullptr;
        uint8_t *bitmap_ = nullptr;
        uint8_t *data_ = nullptr;

        void SetBit(size_t numSlot, uint8_t val)
        {
            if (val == 0)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                bitmap_[numSlot / 8] &= ~(1U << (numSlot % 8));
            }
            else
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                bitmap_[numSlot / 8] |= (1U << (numSlot % 8));
            }
        }

        uint8_t GetBit(size_t numSlot)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic, hicpp-signed-bitwise)
            return (bitmap_[numSlot / 8] >> (numSlot % 8)) & 1U;
        }
    };

public:
    RunOfSlotsAllocator()
    {
        for (size_t slotSize : slotSizes_)
        {
            pools_.push_back(new RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> {slotSize});
        }
    }

    ~RunOfSlotsAllocator()
    {
        for (auto pool : pools_)
        {
            delete pool;
        }
    }

    NO_MOVE_SEMANTIC(RunOfSlotsAllocator);
    NO_COPY_SEMANTIC(RunOfSlotsAllocator);

    template <class T = uint8_t>
    T *Allocate()
    {
        if (sizeof(T) > ONE_MEM_POOL_SIZE || sizeof(T) == 0)
        {
            return nullptr;
        }

        size_t poolNum = SelectPoolToAllocate(sizeof(T));
        if (poolNum >= pools_.size())
        {
            return nullptr;
        }

        return reinterpret_cast<T*>(pools_[poolNum]->Allocate());
    }

    void Free([[maybe_unused]] void *ptr)
    {
        if (ptr == nullptr)
        {
            return;
        }

        for (auto pool : pools_)
        {
            if (pool->Free(ptr))
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

        for (auto pool : pools_)
        {
            if (pool->VerifyPtr(ptr))
            {
                return true;
            }
        }
        return false;
    }

private:
    std::vector<size_t> slotSizes_{SLOTS_SIZES...};
    std::vector<RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE>*> pools_;

    size_t SelectPoolToAllocate(size_t size)
    {
        size_t poolNum = -1;
        while (size > 0)
        {
            size /= 2;
            poolNum++;
        }

        return poolNum;
    }
    
};

#endif  // MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H