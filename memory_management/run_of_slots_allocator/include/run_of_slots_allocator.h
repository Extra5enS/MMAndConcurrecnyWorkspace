#ifndef MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H
#define MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_RUN_OF_SLOTS_ALLOCATOR_H

#include <bitset>
#include <cstdint>
#include <cstddef>
#include <tuple>
#include "base/macros.h"

template <size_t ONE_MEM_POOL_SIZE, size_t... SLOTS_SIZES>
class RunOfSlotsAllocator {
    static_assert(sizeof...(SLOTS_SIZES) != 0, "you should set slots sizes");

    template <size_t MEM_POOL_SIZE, size_t SLOT_SIZE>
    class RunOfSlotsMemoryPool {
    private:
        static constexpr size_t SLOTS_NUMBER = MEM_POOL_SIZE / SLOT_SIZE;
        void *base_;
        void *poolEnd_;
        std::bitset<SLOTS_NUMBER> slots_;

        int FindEmptySlot() const {
            for (size_t i = 0; i < SLOTS_NUMBER; ++i) {
                if (!slots_.test(i)) {
                    return i;
                }
            }

            return -1;
        }

    public:
        RunOfSlotsMemoryPool<MEM_POOL_SIZE, SLOT_SIZE>() : base_(new char[MEM_POOL_SIZE]),
                                                           slots_(),
                                                           // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                                                           poolEnd_(reinterpret_cast<char*>(base_) + MEM_POOL_SIZE)
                                                           {}

        NO_MOVE_SEMANTIC(RunOfSlotsMemoryPool);
        NO_COPY_SEMANTIC(RunOfSlotsMemoryPool);

        ~RunOfSlotsMemoryPool<MEM_POOL_SIZE, SLOT_SIZE>()
        {
            delete [] reinterpret_cast<char*>(base_);
        }

        void *Allocate()
        {
            int emptySlotNumber = FindEmptySlot();
            if (emptySlotNumber < 0) {
                return nullptr;
            }
            slots_.set(emptySlotNumber, true);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return reinterpret_cast<char*>(base_) + emptySlotNumber * SLOT_SIZE;
        }

        bool VerifyPtr(void* ptr) const
        {
            return ptr >= base_ && ptr < poolEnd_;
        }

        void Free(void* ptr)
        {
            size_t ptrSlot = (reinterpret_cast<char*>(ptr) - reinterpret_cast<char*>(base_)) / SLOT_SIZE;
            slots_.set(ptrSlot, false);
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
        auto* memPool = GetPool<sizeof(T)>();
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
