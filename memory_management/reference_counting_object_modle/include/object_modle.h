#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>
#include <functional>
#include <iostream>
#include <bit>

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
    MarkWord() : refCount_(1) {}
    ~MarkWord() = default;

    size_t GetRefCount() const
    {
        return refCount_;
    }

    void SetRefCount(const size_t refCount)
    {
        refCount_ = refCount;
    }

private:
    size_t refCount_;
};

class ObjectHeader
{
public:
    ObjectHeader() = default;
    ~ObjectHeader() = default;

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

    size_t GetRefCount() const
    {
        return objectHeader_.GetRefCount();
    }
    
    void SetRefCount(const size_t refCount)
    {
        objectHeader_.SetRefCount(refCount);
    }

    ObjectHeader objectHeader_;
    T objectData_;
};


template <class T, class... Args>
static Object<T> MakeObject([[maybe_unused]] Args... args)
{
    ObjectWrapper<T> *wrapper = new ObjectWrapper<T>();
    T *data = new (wrapper + sizeof(ObjectHeader)) T{args...};
    Object<T> obj{wrapper};

    return obj;
}

template <class T>
class Object {
public:

    Object() : objectWrapper_(nullptr)// objectWrapper_(new ObjectWrapper<T>())
    {
        // SetRefCount(0);
    }
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
        if (this == &other)
        {
            return *this;
        }
        
        RefCountDtor();
        objectWrapper_ = other.objectWrapper_;
        SetRefCount(UseCount() + 1);

        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other)
    {
        // std::swap(objectWrapper_, other.objectWrapper_);
        objectWrapper_ = other.objectWrapper_;
        other.objectWrapper_ = nullptr;
    }

    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
        if (this != &other)
        {
            // std::swap(objectWrapper_, other.objectWrapper_);
            // std::swap(objectWrapper_->objectData_, other.objectWrapper_->objectData_);
            // std::swap(*objectWrapper_, *(other.objectWrapper_));
            objectWrapper_ = other.objectWrapper_;
            other.objectWrapper_ = nullptr;
        }

        return *this;
    }

    // member access operators
    T &operator*() const noexcept
    {
        return *((T*) (objectWrapper_ + sizeof(ObjectHeader)));
        // (objectWrapper_->objectData_);
    }

    T *operator->() const noexcept
    {
        return (T*) (objectWrapper_ + sizeof(ObjectHeader));//reinterpret_cast<T*>(objectWrapper_ + sizeof(ObjectHeader));
        // &(objectWrapper_->objectData_);
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
        return (this->objectWrapper_ == other.objectWrapper_);
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
        objectWrapper_ = nullptr;
    }
    
    void SetRefCount(const size_t refCount)
    {
        objectWrapper_->SetRefCount(refCount);
    }

    ObjectWrapper<T> *objectWrapper_;
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H