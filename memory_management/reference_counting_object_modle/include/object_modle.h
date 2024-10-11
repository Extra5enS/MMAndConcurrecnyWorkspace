#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>
#include <functional>
#include <iostream>

/*

template <class T>
static size_t GetHash(T *ptr)
{
    std::hash<T> ptrHash;
    return ptrHash(*ptr);
}

size_t hash_ = 0;    
*/

template <class T>
class Object;

class MarkWord;
class KlassWord;
class ObjectHeader;

class MarkWord
{
public:
    MarkWord() : refCount_(new size_t{1}) {}
    explicit MarkWord(std::nullptr_t) {}
        
    MarkWord([[maybe_unused]] const MarkWord &other) : refCount_(other.refCount_)
    {
        SetRefCount(GetRefCount() + 1);
    }

    MarkWord &operator=([[maybe_unused]] const MarkWord &other)
    {
        if (this == &other)
        {
            return *this;
        }
    
        if (GetRefCount() <= 1)
        {
            delete refCount_;
            refCount_ = nullptr;
        }
        else
        {
            SetRefCount(GetRefCount() - 1);
        }

        refCount_ = other.refCount_;
        SetRefCount(GetRefCount() + 1);

        return *this;
    }

    MarkWord(MarkWord &&other)
    {
        std::swap(refCount_, other.refCount_);
    }

    MarkWord &operator=([[maybe_unused]] MarkWord &&other)
    {
        if (this != &other)
        {
            std::swap(refCount_, other.refCount_);
        }

        return *this;
    }

    ~MarkWord() 
    {
        if (GetRefCount() <= 1)
        {
            delete refCount_;
            refCount_ = nullptr;
        }
        else
        {
            SetRefCount(GetRefCount() - 1);
        }
    }

    size_t GetRefCount() const
    {
        if (refCount_ == nullptr)
        {
            return 0;
        }

        return *refCount_;
    }

private:
    void SetRefCount(const size_t refCount)
    {
        if (refCount_ == nullptr)
        {
            return;
        }

        *refCount_ = refCount;
    }

    size_t *refCount_ = nullptr;
};

class ObjectHeader
{
public:
    ObjectHeader() = default;
    ~ObjectHeader() = default;

    explicit ObjectHeader(std::nullptr_t) : markWord_(nullptr) {}

    ObjectHeader([[maybe_unused]] const ObjectHeader &other) = default;

    ObjectHeader &operator=([[maybe_unused]] const ObjectHeader &other)
    {
        if (this != &other)
        {
            markWord_ = other.markWord_;
        }

        return *this;
    }

    ObjectHeader(ObjectHeader &&other)
    {
        std::swap(markWord_, other.markWord_);
    }

    ObjectHeader &operator=([[maybe_unused]] ObjectHeader &&other)
    {
        if (this != &other)
        {
            std::swap(markWord_, other.markWord_);
        }

        return *this;
    }
    
    size_t GetRefCount() const
    {
        return markWord_.GetRefCount();
    }

private:
    MarkWord markWord_;
};

template <class T, class... Args>
static Object<T> MakeObject([[maybe_unused]] Args... args)
{
    T *temp = new T{args...};
    Object<T> obj{temp};
    return obj;
}

template <class T>
class Object {
public:

    Object() : objectData_(nullptr), objectHeader_(nullptr) {}
    explicit Object(std::nullptr_t) : objectData_(nullptr), objectHeader_(nullptr) {}

    explicit Object([[maybe_unused]] T *ptr) : objectData_(ptr) {}

    ~Object()
    {
        if (UseCount() <= 1)
        {
            delete objectData_;
        }
    }

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other) : objectData_(other.objectData_), objectHeader_(other.objectHeader_) {}

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Object<T> &operator=([[maybe_unused]] const Object<T> &other)
    {
        if (this == &other)
        {
            return *this;
        }

        if (UseCount() <= 1)
        {
            delete objectData_;
        }

        objectData_ = other.objectData_;
        objectHeader_ = other.objectHeader_;

        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other)
    {
        std::swap(objectHeader_, other.objectHeader_);
        std::swap(objectData_, other.objectData_);
    }

    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
        if (this != &other)
        {
            std::swap(objectHeader_, other.objectHeader_);
            std::swap(objectData_, other.objectData_);
        }

        return *this;
    }

    // member access operators
    T &operator*() const noexcept
    {
        return *objectData_;
    }

    T *operator->() const noexcept
    {
        return objectData_;
    }

    size_t UseCount() const
    {
        return objectHeader_.GetRefCount();
    }

    bool operator==([[maybe_unused]] const Object<T> other) const
    {
        return (this->objectData_ == other.objectData_);
    }

    bool operator!=(const Object<T> other) const
    {
        return !(*this == other);
    }

    bool operator==(std::nullptr_t) const
    {
        return (objectData_ == nullptr);
    }

    bool operator!=(std::nullptr_t) const
    {
        return (objectData_ != nullptr);
    }

    void Dump() const
    {
        if (objectData_ != nullptr)
        {
            std::cout << "Object data is " << objectData_ << std::endl;
        }
        else
        {
            std::cout << "Object data is nullptr" << std::endl;
        }

        std::cout << "Reference count is " << UseCount() << std::endl;
    }

private:
    ObjectHeader objectHeader_; 
    T *objectData_ = nullptr;
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H