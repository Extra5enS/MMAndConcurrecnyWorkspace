#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H

#include <unistd.h>
#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>
#include "Types.hpp"

template <class T>
class Object;

template <class T, class... Args>
static Object<T> MakeObject(Args... args)
{
    return Object<T>::MakeObject(args...);
}

template <class T>
class Object {
public:
    Object() noexcept = default;

    template <class U = T, std::enable_if_t<std::is_constructible_v<T, U>>>
    Object(U &&value)
    {
        new (Get()) T(std::forward<U>(value));
        getHeader_()->rc = 1;
    }

    explicit Object(std::nullptr_t) : val_(nullptr) {}

    ~Object()
    {
        if (!val_)
            return;

        sz *rc = &getHeader_()->rc;

        if (*rc == 1) {
            getValue_()->~T();
            delete[] (char *)val_;
        } else {
            --*rc;
        }
    }

    Object(const Object &other) : val_(other.val_)
    {
        if (val_)
            getHeader_()->rc++;
    }

    Object &operator=(const Object &other)
    {
        this->~Object();

        val_ = other.val_;

        if (val_)
            getHeader_()->rc++;

        return *this;
    }

    Object(Object &&other) : val_(other.val_)
    {
        other.val_ = nullptr;
    }

    Object &operator=(Object &&other)
    {
        std::swap(val_, other.val_);

        return *this;
    }

    T &operator*() const noexcept
    {
        return *getValue_();
    }

    T *operator->() const noexcept
    {
        return getValue_();
    }

    void Reset(T *ptr) {}

    T *Get() const
    {
        return getValue_();
    }

    sz UseCount() const
    {
        if (val_)
            return getHeader_()->rc;
        return 0;
    }

    template <class... Args>
    static Object MakeObject(Args... args)
    {
        Object obj;

        obj.val_ = allocateObj_();
        obj.getHeader_()->rc = 1;

        new (obj.getValue_()) T(args...);

        return obj;
    }

private:
    struct Header {
        sz rc;
    };

    char *val_ = nullptr;

    static char *allocateObj_()
    {
        return new char[sizeof(Header) + sizeof(T)];
    }

private:
    Header *getHeader_() const noexcept
    {
        return reinterpret_cast<Header *>(val_);
    }

    T *getValue_() const noexcept
    {
        if (val_)
            return reinterpret_cast<T *>(val_ + sizeof(Header));
        return nullptr;
    }
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
