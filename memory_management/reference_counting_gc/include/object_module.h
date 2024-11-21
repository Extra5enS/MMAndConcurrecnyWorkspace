#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H

#include <cstddef>
#include <utility>

template <class T>
class Object;

template <class T, class... Args>
static Object<T> MakeObject([[maybe_unused]] Args... args)
{
    Object<T> newObj{new T{args...}};

    return newObj;
}

template <class T>
class Object {
public:
    Object() = default;
    explicit Object(std::nullptr_t) {}
    explicit Object([[maybe_unused]] T *ptr)
    {
        val_ = ptr;

        refCount_ = new int{1};
    }

    ~Object()
    {
        Release();
    }

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other)
    {
        val_      = other.val_;
        refCount_ = other.refCount_;

        if (refCount_ != nullptr)
        {
            ++(*refCount_);
        }
    }
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Object<T> &operator=([[maybe_unused]] const Object<T> &other)
    {
        Release();

        val_      = other.val_;
        refCount_ = other.refCount_;

        if (refCount_ != nullptr)
        {
            ++(*refCount_);
        }

        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other)
    {
        val_      = std::move(other.val_);
        refCount_ = std::move(other.refCount_);

        other.val_      = nullptr;
        other.refCount_ = nullptr;
    }
    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
        Release();

        val_      = std::move(other.val_);
        refCount_ = std::move(other.refCount_);

        other.val_      = nullptr;
        other.refCount_ = nullptr;

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
    void Reset([[maybe_unused]] T *ptr)
    {
        Release();

        val_ = ptr;

        refCount_ = new int{1};
    }
    T *Get() const
    {
        return val_;
    }
    size_t UseCount() const
    {
        if (refCount_ != nullptr)
        {
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
            return *refCount_;
        }

        return 0;
    }

private:
    void Release()
    {
        if (refCount_ != nullptr)
        {
            if (--(*refCount_) == 0)
            {
                delete val_;
                delete refCount_;

                val_      = nullptr;
                refCount_ = nullptr;
            }
        }
    }

    int *refCount_ = nullptr;

    T *val_ = nullptr; // this field can be deleted
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
