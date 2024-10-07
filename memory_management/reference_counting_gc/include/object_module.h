#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H

#include <algorithm>
#include <cstddef>
#include <iostream>

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
    Object() = default;

    explicit Object(std::nullptr_t) {}

    explicit Object(T *ptr)
    {
        val_ = ptr;
    }

    ~Object()
    {
        Release();
    }

    // copy semantic
    Object(const Object<T> &other) : val_(other.val_), header_(other.header_)
    {
        if (nullptr != header_) {
            header_->rc++;
        }
    }
    Object<T> &operator=(const Object<T> &other)
    {
        if (this == &other) {
            return *this;
        }

        Release();

        val_ = other.val_;
        header_ = other.header_;

        if (header_ != nullptr) {
            header_->rc++;
        }

        return *this;
    }

    // move semantic
    Object(Object<T> &&other) : val_(other.val_), header_(other.header_)
    {
        other.val_ = nullptr;
        other.header_ = nullptr;
    }

    Object<T> &operator=(Object<T> &&other)
    {
        if (this == &other) {
            return *this;
        }

        std::swap(val_, other.val_);
        std::swap(header_, other.header_);

        return *this;
    }

    // member access operators
    T &operator*() const noexcept
    {
        return *val_;
    }

    T *operator->() const noexcept
    {
        return val_;
    }

    // internal access
    void Reset(T *ptr)
    {
        Release();

        if (ptr) {
            val_ = ptr;
            header_ = new Header;
            header_->rc = 1;
        }
    }

    T *Get() const
    {
        return val_;
    }

    size_t UseCount() const
    {
        if (nullptr == header_) {
            return 0;
        }

        return header_->rc;
    }

    template <class... Args>
    static Object<T> MakeObject(Args... args)
    {
        Object<T> obj;

        obj.val_ = new T(args...);
        obj.header_ = new Header;
        obj.header_->rc = 1;

        return obj;
    }

private:
    struct Header {
        size_t rc;
    };

    T *val_ = nullptr;
    Header *header_ = nullptr;

    void Release()
    {
        if (nullptr == header_) {
            return;
        }

        --(header_->rc);

        if (header_->rc == 0) {
            delete val_;
            delete header_;
        }
    }
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
