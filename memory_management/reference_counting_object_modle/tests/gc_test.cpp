#include <gtest/gtest.h>
#include "memory_management/reference_counting_object_modle/include/object_modle.h"
#include "base/macros.h"
#include "delete_detector.h"

class Return42 {
    static constexpr size_t RET_42 = 42U;

public:
    size_t Get() const
    {
        return RET_42;
    }
};

TEST(ReferenceCountingOM, DISABLED_SinglePtrUsage)
{
    Object<size_t> obj;
    ASSERT_EQ(obj.UseCount(), 0);
    ASSERT_EQ(obj, nullptr);

    constexpr size_t VALUE_TO_CREATE = 42U;
    Object<size_t> sizeObj = MakeObject<size_t>(VALUE_TO_CREATE);
    ASSERT_EQ(sizeObj.UseCount(), 1U);
    ASSERT_NE(sizeObj, nullptr);
    ASSERT_EQ(*sizeObj, VALUE_TO_CREATE);

    Object<Return42> classObj = MakeObject<Return42>();
    ASSERT_EQ(classObj.UseCount(), 1U);
    ASSERT_NE(classObj, nullptr);
    ASSERT_EQ(classObj->Get(), Return42().Get());
}

TEST(ReferenceCountingOM, DISABLED_CopySemanticUsage)
{
    constexpr size_t VALUE_TO_CREATE = 42U;
    Object<size_t> obj1 = MakeObject<size_t>(VALUE_TO_CREATE);
    {
        Object<size_t> obj2(obj1);  // NOLINT(performance-unnecessary-copy-initialization)
        ASSERT_EQ(obj1.UseCount(), 2U);
        ASSERT_EQ(obj1, obj2);
    }
    ASSERT_EQ(obj1.UseCount(), 1U);
    {
        Object<size_t> obj2;
        obj2 = obj1;
        ASSERT_EQ(obj1.UseCount(), 2U);
        ASSERT_EQ(obj1, obj2);
    }
    ASSERT_EQ(obj1.UseCount(), 1U);
}

TEST(ReferenceCountingOM, DISABLED_MoveSemanticUsage)
{
    constexpr size_t VALUE_TO_CREATE = 42U;
    Object<size_t> obj1 = MakeObject<size_t>(VALUE_TO_CREATE);
    {
        Object<size_t> obj2(std::move(obj1));
        ASSERT_EQ(obj2.UseCount(), 1U);
        ASSERT_EQ(*obj2, VALUE_TO_CREATE);
        obj1 = obj2;
    }
    ASSERT_EQ(obj1.UseCount(), 1U);
    {
        Object<size_t> obj2;
        obj2 = std::move(obj1);
        ASSERT_EQ(obj2.UseCount(), 1U);
        ASSERT_EQ(*obj2, VALUE_TO_CREATE);
        obj1 = obj2;
    }
    ASSERT_EQ(obj1.UseCount(), 1U);
}

TEST(ReferenceCountingOM, DISABLED_GcDeletingTest)
{
    DeleteDetector::SetDeleteCount(0U);
    auto obj1 = MakeObject<DeleteDetector>();
    {
        auto obj2 = MakeObject<DeleteDetector>();
        obj2->SetDelete(MakeObject<DeleteDetector>());
        obj1->SetDelete(obj2);
        ASSERT_EQ(obj2.UseCount(), 2U);
    }
    ASSERT_EQ(DeleteDetector::GetDeleteCount(), 0U);
    obj1->SetDelete(Object<DeleteDetector>());
    ASSERT_EQ(DeleteDetector::GetDeleteCount(), 2U);
    obj1->~DeleteDetector();
    ASSERT_EQ(DeleteDetector::GetDeleteCount(), 3U);
}

TEST(ReferenceCountingOM, DISABLED_Etest)
{
    constexpr size_t VALUE_TO_CREATE = 42U;
    auto obj1 = MakeObject<size_t>(VALUE_TO_CREATE);
    auto obj2 = MakeObject<size_t>(VALUE_TO_CREATE);

    ASSERT_NE(obj1, obj2);
    ASSERT_EQ(*obj1, *obj2);
}
