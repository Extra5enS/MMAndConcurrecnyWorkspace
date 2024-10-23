#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>
#include <functional>
#include <iostream>
#include "base/macros.h"

template <class T>
class Object;

template <class T>
class ObjectWrapper;

class MarkWord;
class KlassWord;
class ObjectHeader;

class MarkWord
{
public:
    MarkWord() = default;
    ~MarkWord() = default;

    NO_COPY_SEMANTIC(MarkWord);
    NO_MOVE_SEMANTIC(MarkWord);

    size_t GetRefCount() const
    {
        return refCount_;
    }

    void SetRefCount(const size_t refCount)
    {
        refCount_ = refCount;
    }

private:
    size_t refCount_ = 1;
};

class ObjectHeader
{
public:
    ObjectHeader() = default;
    ~ObjectHeader() = default;

    NO_COPY_SEMANTIC(ObjectHeader);
    NO_MOVE_SEMANTIC(ObjectHeader);

    size_t GetRefCount() const
    {
        return markWord_.GetRefCount();
    }

    void SetRefCount(const size_t refCount)
    {
        markWord_.SetRefCount(refCount);
    }

private:
    MarkWord markWord_;
};

template <class T>
class ObjectWrapper {
public:
    ObjectWrapper() = default;
    ~ObjectWrapper() = default;

    NO_COPY_SEMANTIC(ObjectWrapper);
    NO_MOVE_SEMANTIC(ObjectWrapper);

    size_t GetRefCount() const
    {
        return objectHeader_.GetRefCount();
    }
    
    void SetRefCount(const size_t refCount)
    {
        objectHeader_.SetRefCount(refCount);
    }

private:
    ObjectHeader objectHeader_;
    T objectData_ = {};
};


template <class T, class... Args>
static Object<T> MakeObject([[maybe_unused]] Args... args)
{
    auto *wrapper = new ObjectWrapper<T>();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    auto dataPtr = reinterpret_cast<char*>(wrapper) + sizeof(ObjectHeader);
    T *data = new (dataPtr) T{args...};
    wrapper->SetRefCount(1);
    Object<T> obj{wrapper};

    return obj;
}

template <class T>
class Object {
public:

    Object() = default;
    explicit Object(std::nullptr_t) : objectWrapper_(nullptr) {}
    
    Object(T *ptr) = delete;

    explicit Object([[maybe_unused]] ObjectWrapper<T> *wrapper) : objectWrapper_(wrapper) {}

    ~Object()
    {
        RefCountDtor();
    }

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other) : objectWrapper_(other.objectWrapper_)
    {
        SetRefCount(UseCount() + 1);
    }

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Object<T> &operator=([[maybe_unused]] const Object<T> &other)
    {
        if (this != &other)
        {
            RefCountDtor();
            objectWrapper_ = other.objectWrapper_;
            SetRefCount(UseCount() + 1);
        }

        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other)
    {
        std::swap(objectWrapper_, other.objectWrapper_);
    }

    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
        if (this != &other)
        {
            RefCountDtor();
            std::swap(objectWrapper_, other.objectWrapper_);
        }

        return *this;
    }

    // member access operators
    T &operator*() const noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto ptr = reinterpret_cast<char*>(objectWrapper_) + sizeof(ObjectHeader);
        return *reinterpret_cast<T*>(ptr);
    }

    T *operator->() const noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        auto ptr = reinterpret_cast<char*>(objectWrapper_) + sizeof(ObjectHeader);
        return reinterpret_cast<T*>(ptr);
    }

    size_t UseCount() const
    {
        if (objectWrapper_ == nullptr)
        {
            return 0;
        }

        return objectWrapper_->GetRefCount();
    }

    bool operator==([[maybe_unused]] const Object<T> other) const
    {
        return (objectWrapper_ == other.objectWrapper_);
    }

    bool operator!=(const Object<T> other) const
    {
        return !(*this == other);
    }

    bool operator==(std::nullptr_t) const
    {
        return (objectWrapper_ == nullptr);
    }

    bool operator!=(std::nullptr_t) const
    {
        return (objectWrapper_ != nullptr);
    }

private:

    void RefCountDtor()
    {
        if (UseCount() <= 1)
        {
            delete objectWrapper_;
        }
        else
        {
            SetRefCount(UseCount() - 1);
        }
    }
    
    void SetRefCount(const size_t refCount)
    {
        objectWrapper_->SetRefCount(refCount);
    }

    ObjectWrapper<T> *objectWrapper_ = nullptr;
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H