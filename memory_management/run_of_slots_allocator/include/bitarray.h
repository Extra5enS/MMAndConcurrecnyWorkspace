#ifndef MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_BITARRAY_H
#define MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_BITARRAY_H

#include <cstddef>
#include <cstdint>
#include <array>

static constexpr size_t NO_BIT = size_t(-1);

template <size_t SIZE>
class BitArray {
public:
    char ReadBit(size_t index) const noexcept
    {
        Position p = GetPos(index);

        return data_[p.word] & (ONE << p.bit);
    }

    void SetBit(size_t index) noexcept
    {
        Position p = GetPos(index);

        data_[p.word] |= (ONE << p.bit);
    }

    void ClearBit(size_t index) noexcept
    {
        Position p = GetPos(index);

        data_[p.word] &= ~(ONE << p.bit);
    }

    size_t FindBit(unsigned char val) const noexcept
    {
        if (val > 1) {
            return size_t(-1);
        }

        if (val == 0) {
            return FindBitInternal(0, ~ZERO);
        }

        return FindBitInternal(1, ZERO);
    }

private:
    static constexpr size_t BITS_IN_UINT64_T = 64;
    static constexpr uint64_t ONE = 1;
    static constexpr uint64_t ZERO = 0;

    struct Position {
        size_t word;
        size_t bit;
    };

    std::array<uint64_t, SIZE / sizeof(uint64_t) + (SIZE % sizeof(uint64_t) == 0 ? 1 : 0)> data_ {};

    Position GetPos(size_t index) const noexcept
    {
        return {index / SIZE, index % BITS_IN_UINT64_T};
    }

    size_t FindBitInternal(unsigned char val, uint64_t emptyWord) const noexcept
    {
        for (size_t word = 0; word < data_.size(); word++) {
            uint64_t w = data_[word];

            if (w == emptyWord) {
                continue;
            }

            for (size_t bit = 0; bit < BITS_IN_UINT64_T; bit++) {
                unsigned char lastBit = w & ONE;
                if (lastBit == val) {
                    return word * BITS_IN_UINT64_T + bit;
                }

                w <<= 1U;
            }
        }
        return NO_BIT;
    }
};

#endif  // MEMORY_MANAGEMENT_RUN_OF_SLOTS_ALLOCATOR_INCLUDE_BITARRAY_H
