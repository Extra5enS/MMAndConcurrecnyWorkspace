#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>
#include <functional>
#include <iostream>

#include "base/macros.h"

template <class T>
class Object;
class Header;

class Header
{
public:
    Header() = default;
    ~Header() = default;

    NO_COPY_SEMANTIC(Header);
    NO_MOVE_SEMANTIC(Header);

    void SetCount(size_t count)
    {
        count_ = count;
    }

    size_t GetCount() const
    {
        return count_;
    }

private:
    size_t count_ = 1;
};

template <class T>
class ObjectOverall {
public:
    ObjectOverall() = default;
    ~ObjectOverall() = default;

    NO_COPY_SEMANTIC(ObjectOverall);
    NO_MOVE_SEMANTIC(ObjectOverall);

    size_t GetCount() const
    {
        return header_.GetCount();
    }
    
    void SetCount(const size_t count)
    {
        header_.SetCount(count);
    }

    T *GetDataPtr()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return reinterpret_cast<T*>(reinterpret_cast<char*>(this) + sizeof(header_)); 
    }

private:
    Header header_ = {};
    T data_ = {};
};

template <class T, class... Args>
static Object<T> MakeObject([[maybe_unused]] Args... args)
{
    auto *objOver = new ObjectOverall<T>{};
    T *data = new (objOver->GetDataPtr()) T{args...};
    Object<T> obj{objOver};
    return obj;
}

// TODO(you): Implement object base on java object representatoin
template <class T>
class Object {
public:
    Object() = default;
    explicit Object(std::nullptr_t) : objOver_(nullptr) {}

    explicit Object([[maybe_unused]] ObjectOverall<T> *objOver) : objOver_(objOver) {}

    ~Object()
    {
        ProcessMemRelease();
    }

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other) : objOver_(other.objOver_)
    {
        SetCount(UseCount() + 1);
    }

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Object<T> &operator=([[maybe_unused]] const Object<T> &other)
    {
        if (this == &other)
        {
            return *this;
        }

        ProcessMemRelease();
        objOver_ = other.objOver_;
        SetCount(UseCount() + 1);
        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other)
    {
        std::swap(objOver_, other.objOver_);
    }

    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
        if (this == &other)
        {
            return *this;
        }

        ProcessMemRelease();
        std::swap(objOver_, other.objOver_);
        return *this;
    }

    // member access operators
    T &operator*() const noexcept
    {
        return *(objOver_->GetDataPtr());
    }

    T *operator->() const noexcept
    {
        return objOver_->GetDataPtr();
    }

    size_t UseCount() const
    {
        if (objOver_ == nullptr)
        {
            return 0;
        } 
        return objOver_->GetCount();
    }

    bool operator==([[maybe_unused]] const Object<T> other) const
    {
        return objOver_ == other.objOver_;
    }

    bool operator!=(const Object<T> other) const
    {
        return objOver_ != other.objOver_;
    }

    bool operator==(std::nullptr_t) const
    {
        return objOver_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const
    {
        return objOver_ != nullptr;
    }

private:
    ObjectOverall<T> *objOver_ = nullptr;

    void ProcessMemRelease()
    {
        if (UseCount() <= 1)
        {
            delete objOver_;
        }
        else
        {
            SetCount(UseCount() - 1);
        }
        objOver_ = nullptr;
    }

    void SetCount(size_t count)
    {
        objOver_->SetCount(count);
    }
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H