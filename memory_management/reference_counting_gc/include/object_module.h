#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H

#include <algorithm>
#include <cstddef>
#include <type_traits>
#include <utility>

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
    explicit Object(U &&value) : val_(new T(std::forward<U>(value))), header_(new Header_)
    {
        header_->rc = 1;
    }

    explicit Object(std::nullptr_t) noexcept {};

    ~Object()
    {
        if (!header_)
            return;

        size_t &rc = header_->rc;

        if (rc == 1) {
            delete val_;
            delete header_;
        } else {
            rc--;
        }
    }

    Object(const Object &other) : val_(other.val_), header_(other.header_)
    {
        if (header_) {
            header_->rc++;
        }
    }

    Object &operator=(const Object &other)
    {
        if (this == &other) {
            return *this;
        }

        this->~Object();

        val_ = other.val_;
        header_ = other.header_;

        if (header_) {
            header_->rc++;
        }

        return *this;
    }

    Object(Object &&other) : val_(other.val_), header_(other.header_)
    {
        other.val_ = nullptr;
        other.header_ = nullptr;
    }

    Object &operator=(Object &&other)
    {
        if (this == &other) {
            return *this;
        }

        std::swap(val_, other.val_);
        std::swap(header_, other.header_);

        return *this;
    }

    T &operator*() const noexcept
    {
        return *val_;
    }

    T *operator->() const noexcept
    {
        return val_;
    }

    void Reset(T *ptr)
    {
        this->~Object();

        val_ = ptr;
        header_ = new Header_;
        header_->rc = 1;
    }

    T *Get() const
    {
        return val_;
    }

    size_t UseCount() const
    {
        if (header_ == nullptr) {
            return 0;
        }
        return header_->rc;
    }

    template <class... Args>
    static Object MakeObject(Args... args)
    {
        Object obj;

        obj.val_ = new T(args...);
        obj.header_ = new Header_;
        obj.header_->rc = 1;

        return obj;
    }

private:
    struct Header_ {
        size_t rc = 0;
    };

    T *val_ = nullptr;
    Header_ *header_ = nullptr;
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
