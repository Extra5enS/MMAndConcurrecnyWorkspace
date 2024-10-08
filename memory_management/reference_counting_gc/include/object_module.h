#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H

#include <cstddef>
#include <utility>
#include <iostream>
//<string_view>

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
    Object() : val_(nullptr), ref_count_(nullptr) {}
    explicit Object(std::nullptr_t) : val_(nullptr), ref_count_(nullptr) {}
    
    explicit Object([[maybe_unused]] T *ptr) : val_(ptr), ref_count_(new size_t)
    {
        *ref_count_ = 1;
    }

    ~Object()
    {
        if (UseCount() == 0 || UseCount() == 1)
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
        if (UseCount() != 0)
        {
            (*ref_count_)--;
        }
        
        val_ = other.val_;
        ref_count_ = other.ref_count_;
        (*ref_count_)++;
     
        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other) //: val_(other.val_), ref_count_(other.ref_count_)
    {
        std::swap(val_, other.val_);
        std::swap(ref_count_, other.ref_count_);
    }

    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
        if (this == &other)
        {
            return *this;
        }

        std::swap(val_, other.val_);
        std::swap(ref_count_, other.ref_count_);
        
        other.~Object();

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

        if (UseCount() == 0)
        {
            ref_count_ = new size_t;
        }
        else
        {
            delete val_;
        }
        
        (*ref_count_) = 1;
        val_ = ptr;
    }

    T *Get() const
    {
        return val_;
    }
    size_t UseCount() const
    {
        if (ref_count_ == nullptr)
        {
            return 0;
        }
        else
        {
            return *ref_count_;
        }
    }    

private:

    void Dump() const
    {
        if (ref_count_ == nullptr)
        {   
            std::cout << "reference counting is not exist" << std::endl;
        }
        else
        {
            std::cout << "reference counting " << UseCount() << std::endl;
        }
        
        if (val_ == nullptr)
        {   
            std::cout << "value of object is not exist" << std::endl;
        }
        else
        {
            std::cout << "value of object " << Get() << std::endl;
        }
    }

    size_t* ref_count_ = nullptr;
    T* val_ = nullptr;
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H