#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>
#include <functional>
#include <iostream>

template <class T>
class Object;

class MarkWord;
class KlassWord;
class ObjectHeader;

class MarkWord
{
public:
    MarkWord() : refCount_(new size_t{1}) {}
    
    MarkWord(const size_t refCount) : refCount_(new size_t{refCount}) {}

    ~MarkWord() 
    {
        if (getRefCount() <= 1)
        {
            delete refCount_;
            refCount_ = nullptr;
        }
        else
        {
            (*refCount_)--;
        }
    }

    size_t getRefCount() const
    {
        if (refCount_ == nullptr)
        {
            return 0;
        }

        return *refCount_;
    }

    template <class T>
    void hashData(T *ptr)
    {
        std::hash<T> ptr_hash;
        hash_ = ptr_hash(*ptr);
    }

    size_t hash_ = 0;
    size_t *refCount_ = nullptr;
};

class ObjectHeader
{
public:
    ObjectHeader() : markWord_() {}
    ~ObjectHeader() {}

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

    static void setRefCount(Object<T> &object, const size_t refCount)
    {
        *(object.objectHeader_->markWord_.refCount_) = refCount;
    }

    static size_t getRefCount(Object<T> &object)
    {
        return *(object.objectHeader_->markWord_.refCount_);
    }


    Object() : objectData_(nullptr), objectHeader_(nullptr) {}
    explicit Object(std::nullptr_t) : objectData_(nullptr), objectHeader_(nullptr) {}

    explicit Object([[maybe_unused]] T *ptr) : objectData_(ptr), objectHeader_(new ObjectHeader{}) {}

    ~Object()
    {
        if (UseCount() <= 1)
        {
            delete objectData_;
            delete objectHeader_;
        }
        else
        {
            objectData_ = nullptr;
            objectHeader_->~ObjectHeader();
        }
    }

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other) : objectData_(other.objectData_), objectHeader_(other.objectHeader_)
    {
        setRefCount(*this, getRefCount(*this) + 1);
    }

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
            delete objectHeader_;
        }
        else
        {
            objectHeader_->~ObjectHeader();
        }

        objectData_ = other.objectData_;
        objectHeader_ = other.objectHeader_;
        setRefCount(*this, getRefCount(*this) + 1);

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
        if (objectHeader_ == nullptr)
        {
            return 0;
        }

        return objectHeader_->markWord_.getRefCount();
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
        if (objectData_ == nullptr)
            return true;

        return false;
    }

    bool operator!=(std::nullptr_t) const
    {
        return !(*this == nullptr);
    }

    void dump() const
    {
        if (objectData_)
        {
            std::cout << "Object data is " << objectData_ << std::endl;
        }
        else
        {
            std::cout << "Object data is nullptr" << std::endl;
        }

        if (objectHeader_)
        {
            std::cout << "Object Header is " << objectHeader_ << std::endl;
            std::cout << "Reference count is " << UseCount() << std::endl;
        }
        else
        {
            std::cout << "Object Header is nullptr" << std::endl;
        }
    }

private:
    ObjectHeader *objectHeader_ = nullptr; 
    T *objectData_ = nullptr;
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H