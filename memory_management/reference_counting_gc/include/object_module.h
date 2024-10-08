#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H

#include <cstddef>
#include <memory>
#include <iostream>

template <class T>
class Object;

template <class T, class... Args>
static Object<T> MakeObject([[maybe_unused]] Args... args)
{
    T *val = new T{args...};
    Object<T> obj{val};
    return obj;
}

template <class T>
class Object {
public:
    Object() = default;

    explicit Object(std::nullptr_t)
    {
        count_ = new size_t{1};
    }

    explicit Object([[maybe_unused]] T *ptr) : val_(ptr)
    {
        count_ = new size_t{1};
    }

    ~Object()
    {
        Dtor();
    }

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other)
    {
        val_ = other.val_;
        count_ = other.count_;
        (*count_)++;
    }
    
    Object<T> &operator=([[maybe_unused]] const Object<T> &other)
    {
        if (this == &other)
        {
            return *this;
        }
        
        Dtor();

        val_ = other.val_;
        count_ = other.count_;
        (*count_)++;
        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other)
    {
        std::swap(val_, other.val_);
        std::swap(count_, other.count_);
    }

    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
        std::swap(val_, other.val_);
        std::swap(count_, other.count_);
        return *this;
    }

    // member access operators
    T &operator*() const noexcept {
        return *val_;
    }

    T *operator->() const noexcept {
        return val_;
    }

    // internal access
    void Reset([[maybe_unused]] T *ptr)
    {
        Dtor();
        val_ = ptr;
        count_ = new size_t{1}; 
    }

    T *Get() const
    {
        return val_;
    }

    size_t UseCount() const
    {
        if (count_ == nullptr)
        {
            return 0;
        }
        return *count_;
    }

private:
    
    T *val_ = nullptr;
    size_t *count_ = nullptr;

    void Dtor()
    {
        if (count_ == nullptr)
        {
            return;
        }

        (*count_)--;
        if (*count_ <= 0)
        {
            delete count_;
            delete val_;
        }
    }
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H