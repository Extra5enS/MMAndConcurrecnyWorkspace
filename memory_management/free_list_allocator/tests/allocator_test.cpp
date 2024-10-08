
#include <gtest/gtest.h>
#include <cstddef>
#include "memory_management/free_list_allocator/include/free_list_allocator.h"

TEST(FreeListAllocatorTest, TemplateAllocationTest)  // remove DISABLED_ prefix to use test
{
    constexpr size_t MEMORY_POOL_SIZE = 4048U;
    FreeListAllocator<MEMORY_POOL_SIZE> allocator;

    auto *size1 = allocator.Allocate<size_t>(1U);
    auto *size2 = allocator.Allocate<size_t>(1U);
    ASSERT_NE(size1, nullptr);
    ASSERT_TRUE(allocator.VerifyPtr(size1));
    ASSERT_NE(size2, nullptr);
    ASSERT_TRUE(allocator.VerifyPtr(size2));
    ASSERT_EQ(size_t(size2) - size_t(size1), sizeof(size_t));

    auto *int1 = allocator.Allocate<int>(1U);
    ASSERT_NE(int1, nullptr);
    ASSERT_TRUE(allocator.VerifyPtr(int1));
    size_t pointDiff = (size_t(size2) > size_t(int1)) ? size_t(size2) - size_t(int1) : size_t(int1) - size_t(size2);
    ASSERT_GE(pointDiff, MEMORY_POOL_SIZE);  // in different pools

    allocator.Free(size1);
    allocator.Free(size2);
    allocator.Free(int1);
}

TEST(FreeListAllocatorTest, AllocatorMemPoolOverflowTest)  // remove DISABLED_ prefix to use test
{
    constexpr size_t MEMORY_POOL_SIZE = 8U;
    FreeListAllocator<MEMORY_POOL_SIZE> allocator;
    auto *mem = allocator.Allocate<size_t>(1U);
    ASSERT_EQ(mem, nullptr);
}