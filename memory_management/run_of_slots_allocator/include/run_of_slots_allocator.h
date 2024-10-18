#ifndef MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
#define MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H

#include <cmath>
#include <cstdint>
#include <cstddef>
#include <tuple>
#include "base/macros.h"

static constexpr size_t BYTE_SIZE = 8;

template <size_t ONE_MEM_POOL_SIZE, size_t... SLOTS_SIZES>
class RunOfSlotsAllocator {
    static_assert(sizeof...(SLOTS_SIZES) != 0, "you should set slots sizes");

    template <size_t MEM_POOL_SIZE, size_t SLOT_SIZE>
    class RunOfSlotsMemoryPool {
    private:
        static constexpr size_t SLOTS_NUMBER = MEM_POOL_SIZE / SLOT_SIZE;
        static constexpr size_t SLOTS_MEMORY_SIZE = std::ceil(static_cast<double>(SLOTS_NUMBER) / BYTE_SIZE);
        void *base_;
        uint8_t *slots_;

        int FindEmptySlot() const {
            size_t curBit = 0;

            for (size_t byte = 0; byte < SLOTS_MEMORY_SIZE; ++byte) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                uint8_t curSlot = slots_[byte];
                for (size_t bit = 0; bit < BYTE_SIZE && curBit < SLOTS_NUMBER; ++bit, ++curBit) {
                    uint8_t bitmask = 1U << bit;
                    if ((curSlot & bitmask) == 0) {
                        return BYTE_SIZE * byte + bit;
                    }
                }
            }

            return -1;
        }

        void SetSlot(size_t slotNumber, bool value = true) {
            size_t byteSlot = slotNumber / BYTE_SIZE;
            uint8_t bit = 1U << (slotNumber % BYTE_SIZE);
            if (!value) {
                bit = ~bit;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                slots_[byteSlot] &= bit;
            } else {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                slots_[byteSlot] |= bit;
            }
        }

    public:
        RunOfSlotsMemoryPool()
        {
            slots_ = new uint8_t[MEM_POOL_SIZE + SLOTS_MEMORY_SIZE]{0};
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            base_ = slots_ + SLOTS_MEMORY_SIZE;
        }

        NO_MOVE_SEMANTIC(RunOfSlotsMemoryPool);
        NO_COPY_SEMANTIC(RunOfSlotsMemoryPool);

        ~RunOfSlotsMemoryPool()
        {
            delete [] slots_;
        }

        void *Allocate()
        {
            int emptySlotNumber = FindEmptySlot();
            if (emptySlotNumber < 0) {
                return nullptr;
            }
            SetSlot(emptySlotNumber);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return static_cast<char*>(base_) + emptySlotNumber * SLOT_SIZE;
        }

        bool VerifyPtr(void* ptr) const
        {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return ptr >= base_ && ptr < static_cast<char*>(base_) + MEM_POOL_SIZE;
        }

        void Free(void* ptr)
        {
            size_t slotNumber = (static_cast<char*>(ptr) - static_cast<char*>(base_)) / SLOT_SIZE;
            SetSlot(slotNumber, false);
        }

    };

public:
    RunOfSlotsAllocator() : memPools_(std::make_tuple(new RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE, SLOTS_SIZES>()...))
                            {}
    ~RunOfSlotsAllocator()
    {
        std::apply([](auto*... pools) {
            (delete pools, ...);
        }, memPools_);
    }

    NO_MOVE_SEMANTIC(RunOfSlotsAllocator);
    NO_COPY_SEMANTIC(RunOfSlotsAllocator);

    template <class T = uint8_t>
    T *Allocate()
    {
        auto *memPool = GetPool<sizeof(T)>();
        return reinterpret_cast<T*>(memPool->Allocate());
    }

    void Free(void *ptr)
    {
        std::apply([this, ptr](auto*... pools) {
            // NOLINTNEXTLINE(clang-diagnostic-unused-value,-warnings-as-errors) this warning happens because false in next line looks like unused expression, but it is neccessary here to continue loop
            ((pools->VerifyPtr(ptr) ? (pools->Free(ptr), true) : false) || ...);
        }, memPools_);
    }

    /**
     * @brief Method should check in @param ptr is pointer to mem from this allocator
     * @returns true if ptr is from this allocator
     */
    bool VerifyPtr(void *ptr) const
    {
        return std::apply([this, ptr](const auto*... pools) {
            return (pools->VerifyPtr(ptr) || ...);
        }, memPools_);
    }

private:
    std::tuple<RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE, SLOTS_SIZES>*...> memPools_;

    template<size_t SLOT_SIZE>
    RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE, SLOT_SIZE>* GetPool()
    {
        return std::get<RunOfSlotsMemoryPool<ONE_MEM_POOL_SIZE, SLOT_SIZE>*>(memPools_);
    }
};

#endif  // MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
