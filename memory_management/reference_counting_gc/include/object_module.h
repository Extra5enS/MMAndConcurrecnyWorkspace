#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H

#include <cstddef>

template <class T>
class Object;

template <class T, class... Args>
static Object<T> MakeObject(Args... args)
{
    T *val = new T {args...};
    Object<T> obj {val};
    return obj;
}

template <class T>
class Object {
public:
    Object() = default;
    explicit Object(std::nullptr_t) : refCount_(new size_t {1}) {}
    explicit Object(T *ptr) : val_(ptr), refCount_(new size_t {1}) {}

    ~Object()
    {
        Release();
    }

    // copy semantic
    Object(const Object<T> &other) : val_(other.val_), refCount_(other.refCount_)
    {
        ++(*refCount_);
    }

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Object<T> &operator=(const Object<T> &other)
    {
        if (this != &other) {
            Release();
            val_ = other.val_;
            refCount_ = other.refCount_;
            ++(*refCount_);
        }

        return *this;
    }

    // move semantic
    Object(Object<T> &&other) : val_(other.val_), refCount_(other.refCount_)
    {
        other.val_ = nullptr;
        other.refCount_ = nullptr;
    }

    Object<T> &operator=(Object<T> &&other)
    {
        if (this != &other) {
            Release();
            val_ = other.val_;
            refCount_ = other.refCount_;

            other.val_ = nullptr;
            other.refCount_ = nullptr;
        }

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
        if (val_ != ptr) {
            Release();
            val_ = ptr;
            refCount_ = new size_t {1};
        }
    }

    T *Get() const
    {
        return val_;
    }

    size_t UseCount() const
    {
        if (refCount_ == nullptr) {
            return 0;
        }

        return *refCount_;
    }

private:
    T *val_ = nullptr;
    size_t *refCount_ = nullptr;

    void Release()
    {
        if (refCount_ == nullptr) {
            return;
        }

        if (--(*refCount_) == 0) {
            delete val_;
            delete refCount_;
        }
    }
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
