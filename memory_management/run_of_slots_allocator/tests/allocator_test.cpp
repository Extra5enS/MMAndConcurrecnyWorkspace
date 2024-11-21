#include <gtest/gtest.h>
#include <cstddef>
#include "memory_management/run_of_slots_allocator/include/run_of_slots_allocator.h"

TEST(RunOfSlotsAllocatorTest, TemplateAllocationTest)
{
    constexpr size_t MEMORY_POOL_SIZE = 2048U;
    RunOfSlotsAllocator<MEMORY_POOL_SIZE, 1U, 2U, 4U, 8U> allocator;

    auto *size1 = allocator.Allocate<size_t>();
    auto *size2 = allocator.Allocate<size_t>();
    ASSERT_NE(size1, nullptr);
    ASSERT_TRUE(allocator.VerifyPtr(size1));
    ASSERT_NE(size2, nullptr);
    ASSERT_TRUE(allocator.VerifyPtr(size2));
    ASSERT_EQ(size_t(size2) - size_t(size1), sizeof(size_t));

    auto *int1 = allocator.Allocate<int>();
    ASSERT_NE(int1, nullptr);
    ASSERT_TRUE(allocator.VerifyPtr(int1));
    size_t pointDiff = (size_t(size2) > size_t(int1)) ? size_t(size2) - size_t(int1) : size_t(int1) - size_t(size2);
    ASSERT_GE(pointDiff, MEMORY_POOL_SIZE);  // in different pools

    allocator.Free(size1);
    allocator.Free(size2);
    allocator.Free(int1);
}

TEST(RunOfSlotsAllocatorTest, AllocatorMemPoolOverflowTest)
{
    constexpr size_t MEMORY_POOL_SIZE = 8U;
    RunOfSlotsAllocator<MEMORY_POOL_SIZE, 1U, 2U, 4U, 8U> allocator;

    auto *mem = allocator.Allocate<size_t>();
    ASSERT_NE(mem, nullptr);
    ASSERT_TRUE(allocator.VerifyPtr(mem));

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    ASSERT_FALSE(allocator.VerifyPtr(mem + 1U));  // in should be false becase you can not allocate mem with 0 size
    ASSERT_EQ(allocator.Allocate<size_t>(), nullptr);

    auto *memInt = allocator.Allocate<int>();
    ASSERT_NE(memInt, nullptr);
    ASSERT_TRUE(allocator.VerifyPtr(memInt));

    allocator.Free(mem);
    allocator.Free(memInt);
}
