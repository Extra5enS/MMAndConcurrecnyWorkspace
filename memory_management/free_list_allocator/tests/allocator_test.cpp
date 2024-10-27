
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
    ASSERT_LE(size_t(size2) - size_t(size1), MEMORY_POOL_SIZE); // in one pool

    auto *str1 = allocator.Allocate<char>(MEMORY_POOL_SIZE / 2);
    auto *str2 = allocator.Allocate<char>(MEMORY_POOL_SIZE / 2);
    ASSERT_NE(str1, nullptr);
    ASSERT_TRUE(allocator.VerifyPtr(str1));
    size_t pointDiff = (size_t(str2) > size_t(size1)) ? size_t(str2) - size_t(size1) : size_t(size1) - size_t(str2);
    ASSERT_GE(pointDiff, MEMORY_POOL_SIZE); // in different pool

    allocator.Free(size1);
    allocator.Free(size2);
    allocator.Free(str1);
    allocator.Free(str2);
}

TEST(FreeListAllocatorTest, AllocatorMemPoolOverflowTest)  // remove DISABLED_ prefix to use test
{
    constexpr size_t MEMORY_POOL_SIZE = 8U;
    FreeListAllocator<MEMORY_POOL_SIZE> allocator;
    auto *mem = allocator.Allocate<size_t>(1U);
    ASSERT_EQ(mem, nullptr);
}
