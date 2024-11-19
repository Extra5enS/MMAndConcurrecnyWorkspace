#ifndef MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
#define MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include "base/macros.h"
#include <vector>
#include <algorithm>
#include <cmath>

template <size_t ONE_MEM_POOL_SIZE, size_t... SLOTS_SIZES>
class RunOfSlotsAllocator {
    static_assert(sizeof...(SLOTS_SIZES) != 0, "you should set slots sizes");

    // here we recommend you to use class MemoryPool to create RunOfSlots for 1 size. Use new to allocate them from
    // heap. remember, you can not use any containers with heap allocations

    template <size_t MEM_POOL_SIZE>
    class RunOfSlotsMemoryPool
    {
        uint8_t GetBit(uint8_t num, size_t bitPos)
        {
            return num & (1U << bitPos);
        }

        void EraseBit(uint8_t *num, size_t bitPos)
        {
            (*num) &= (~1U) << bitPos;
        }

        void SetMask(uint8_t *num, uint8_t mask)
        {
            (*num) |= mask;
        }

    public:

        RunOfSlotsMemoryPool() = default;

        explicit RunOfSlotsMemoryPool(size_t slotSize) : slotSize_(slotSize), slotsCount_(MEM_POOL_SIZE / slotSize)
        {
            bitsetSize_ = slotsCount_ / 8 + (((slotsCount_ % 8) == 0) ? 0 : 1);

            data_ = new uint8_t[MEM_POOL_SIZE + bitsetSize_] {};
        }

        NO_MOVE_SEMANTIC(RunOfSlotsMemoryPool);
        NO_COPY_SEMANTIC(RunOfSlotsMemoryPool);

        template<class T = uint8_t>
        T *Allocate()
        {
            int freeSlot = GetFreeSlot();

            if (freeSlot < 0)
            {
                return nullptr;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return (T *) (data_ + bitsetSize_ + freeSlot * slotSize_);
        }

        bool Verify([[maybe_unused]] void *ptr)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, modernize-use-auto)
            uint8_t *ptrToVerify = (uint8_t *) ptr;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (ptrToVerify < data_ + bitsetSize_ || ptrToVerify >= data_ + bitsetSize_ + MEM_POOL_SIZE)
            {
                return false;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            int slotPos = (ptrToVerify - (data_ + bitsetSize_)) / slotSize_;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (((ptrToVerify - (data_ + bitsetSize_)) % slotSize_) != 0)
            {
                return false;
            }

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (GetBit(data_[slotPos / 8], slotPos % 8) == 0)
            {
                return false;
            }

            return true;
        }

        void Free([[maybe_unused]] void *ptr)
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast, modernize-use-auto)
            uint8_t *ptrToFree = (uint8_t *) ptr;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            size_t slotPos = (ptrToFree - (data_ + bitsetSize_)) / slotSize_;

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            EraseBit(&data_[slotPos / 8], slotPos % 8);
        }

        int GetFreeSlot()
        {
            uint8_t *bitSet = data_;

            int freeSlot = 0;

            for (int i = 0; (i < bitsetSize_); i++)
            {
                for (int j = 0; j < slotsCount_ && j < 8; j++)
                {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    if (GetBit(bitSet[i], j) == 0)
                    {
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                        SetMask(&bitSet[i], 1U << (unsigned int) j);

                        return freeSlot;
                    }

                    ++freeSlot;
                }
            }

            return -1;
        }

        size_t GetEntrySize()
        {
            return slotSize_;
        }

        ~RunOfSlotsMemoryPool()
        {
            delete[] data_;
        }

    private:
        uint8_t *data_ = nullptr;

        size_t slotSize_   = 0;
        size_t slotsCount_ = 0;
        size_t bitsetSize_ = 0;
    };

public:
    RunOfSlotsAllocator()
    {
        std::vector<size_t> slotsSizes {SLOTS_SIZES...};

        std::sort(slotsSizes.begin(), slotsSizes.end());

        size_t slotsCount = std::log2(slotsSizes.back()) + 1;

        for (int i = 0; i < slotsCount; i++)
        {
            // NOLINTNEXTLINE(hicpp-signed-bitwise)
            if (std::find(slotsSizes.begin(), slotsSizes.end(), 1U << i) != slotsSizes.end())
            {
                // NOLINTNEXTLINE(hicpp-signed-bitwise)
                pools_.push_back(new RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> {1U << i});
            }
            else
            {
                pools_.push_back(nullptr);
            }
        }
    }

    ~RunOfSlotsAllocator()
    {
        for (RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *pool : pools_)
        {
            delete pool;
        }
    }

    NO_MOVE_SEMANTIC(RunOfSlotsAllocator);
    NO_COPY_SEMANTIC(RunOfSlotsAllocator);

    template <class T = uint8_t>
    T *Allocate()
    {

        size_t slotPos = std::log2(sizeof(T));

        if (slotPos >= pools_.size())
        {
            return nullptr;
        }

        return pools_[slotPos]->template Allocate<T>();
    }

    void Free([[maybe_unused]] void *ptr)
    {
        for (RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *pool : pools_)
        {
            if (pool->Verify(ptr))
            {
                pool->Free(ptr);
            }
        }
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr([[maybe_unused]] void *ptr)  // TODO(you): remove [[maybe_unused]]
    {
        for (RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *pool : pools_)
        {
            if (pool->Verify(ptr))
            {
                return true;
            }
        }

        return false;
    }

private:
    std::vector<RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE> *> pools_;
};

#endif  // MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
