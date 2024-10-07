#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H

#include <cstddef>

template <class T>
class Object;

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
    Object() = default;
    explicit Object(std::nullptr_t) : ref_count_(new size_t) {}
    
    explicit Object([[maybe_unused]] T *ptr) : val_(ptr), ref_count_(new size_t)
    {
        (*ref_count_)++;
    }

    ~Object()
    {
        if (*ref_count_ == 1)
        {
            delete ref_count_;
            delete val_;
        }
        else
        {
            (*ref_count_)--;
        }
    }

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other) : val_(other.val_), ref_count_(other.ref_count_) 
    {
        (*ref_count_)++;
    }
    
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)

    Object<T> &operator=([[maybe_unused]] const Object<T> &other)
    {
        (*ref_count_)--;
        val_ = other.val_;
        ref_count_ = other.ref_count_;
                
        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other) : val_(other.val_), ref_count_(other.ref_count_) 
    {
        other.val_ = nullptr;
        other.ref_count_ = nullptr;
    }

    Object<T> &operator=([[maybe_unused]] Object<T> &&other) : val_(other.val_), ref_count_(other.ref_count_)
    {
        other.val_ = nullptr;
        other.ref_count_ = nullptr;

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
        if (val_ == ptr)
        {    
            return;
        }
        
        *val_ = *ptr;
        (*ref_count_) = 1;
    }

    T *Get() const
    {
        return val_;
    }
    size_t UseCount() const
    {
        return *ref_count_;
    }

private:

    size_t* ref_count_ = nullptr;
    T* val_ = nullptr;
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H