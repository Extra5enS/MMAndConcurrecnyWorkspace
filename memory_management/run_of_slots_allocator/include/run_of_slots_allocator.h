#ifndef MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
#define MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H

#include <cstdint>
#include <cstddef>
#include "base/macros.h"
#include "bitarray.h"

template <size_t ONE_MEM_POOL_SIZE, size_t... SLOTS_SIZES>
class RunOfSlotsAllocator {
    static_assert(sizeof...(SLOTS_SIZES) != 0, "you should set slots sizes");

    // here we recommend you to use class MemoryPool to create RunOfSlots for 1 size. Use new to allocate them from
    // heap. remember, you can not use any containers with heap allocations
    template <size_t MEM_POOL_SIZE>
    class RunOfSlotsMemoryPool {
    public:
        explicit RunOfSlotsMemoryPool(size_t slotSize) noexcept
            : slotSize_(slotSize),
              maxSlot_(MEM_POOL_SIZE / slotSize_ - 1),
              // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
              data_(reinterpret_cast<char *>(&slots_ + 1)),
              // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
              dataEnd_(data_ + MEM_POOL_SIZE)
        {
        }

        NO_MOVE_SEMANTIC(RunOfSlotsMemoryPool);
        NO_COPY_SEMANTIC(RunOfSlotsMemoryPool);

        ~RunOfSlotsMemoryPool() = default;

        char *Allocate() noexcept
        {
            size_t freeSlot = slots_.FindBit(FREE);
            if (freeSlot == NO_BIT || freeSlot > maxSlot_) {
                return nullptr;
            }

            slots_.SetBit(freeSlot);

            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return data_ + freeSlot * slotSize_;
        }

        void Free(const void *ptr) noexcept
        {
            if (ptr == nullptr) {
                return;
            }

            if (!VerifyPtr(ptr)) {
                return;
            }

            size_t slot = GetSlotNumber(ptr);

            slots_.ClearBit(slot);
        }

        bool VerifyPtr(const void *ptr) const noexcept
        {
            auto p = uintptr_t(ptr);

            bool inBounds = data_ <= ptr && ptr < dataEnd_ && (p % slotSize_ == 0);
            if (!inBounds) {
                return false;
            }

            size_t slot = GetSlotNumber(ptr);

            bool allocated = slot <= maxSlot_ && slots_.ReadBit(slot);

            return allocated;
        }

        size_t GetSlotSize() const noexcept
        {
            return slotSize_;
        }

    private:
        static constexpr unsigned char FREE = 0;
        size_t slotSize_ = 0;
        size_t maxSlot_ = 0;
        char *data_ = nullptr;
        char *dataEnd_ = nullptr;

        BitArray<MEM_POOL_SIZE> slots_ {};

        size_t GetSlotNumber(const void *ptr) const noexcept
        {
            return (ptrdiff_t(ptr) - ptrdiff_t(data_)) / slotSize_;
        }
    };

public:
    RunOfSlotsAllocator()
    {
        for (size_t i = 0; i < NUM_OF_SLOTS; i++) {
            size_t poolSize = sizeof(Pool) + ONE_MEM_POOL_SIZE;
            auto pool = new char[poolSize];

            new (pool) Pool(SLOT_SIZES[i]);

            pools_[i] = reinterpret_cast<Pool *>(pool);
        }
    }

    ~RunOfSlotsAllocator()
    {
        for (auto pool : pools_) {
            pool->~Pool();
            delete[] reinterpret_cast<char *>(pool);
        }
    }

    NO_MOVE_SEMANTIC(RunOfSlotsAllocator);
    NO_COPY_SEMANTIC(RunOfSlotsAllocator);

    template <class T = uint8_t>
    T *Allocate()
    {
        Pool *pool = FindCorrectPool(sizeof(T));

        if (pool == nullptr) {
            return nullptr;
        }

        char *allocated = pool->Allocate();

        return reinterpret_cast<T *>(allocated);
    }

    void Free(void *ptr)
    {
        for (auto pool : pools_) {
            pool->Free(ptr);
        }
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr(void *ptr)
    {
        for (auto pool : pools_) {
            if (pool->VerifyPtr(ptr)) {
                return true;
            }
        }
        return false;
    }

private:
    using Pool = RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE>;

    static constexpr size_t NUM_OF_SLOTS = sizeof...(SLOTS_SIZES);
    static constexpr std::array<size_t, NUM_OF_SLOTS> SLOT_SIZES {SLOTS_SIZES...};

    std::array<Pool *, NUM_OF_SLOTS> pools_ {};

    Pool *FindCorrectPool(size_t slotSize) noexcept
    {
        for (size_t i = 0; i < NUM_OF_SLOTS; i++) {
            if (SLOT_SIZES[i] == slotSize) {
                return pools_[i];
            }
        }

        return nullptr;
    }
};

#endif  // MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
