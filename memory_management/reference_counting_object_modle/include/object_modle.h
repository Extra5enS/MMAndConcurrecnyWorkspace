#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>
#include <utility>

struct DataHeader
{
    size_t refCount;
};

template <class T>
class Object;

template <class T, class... Args>
static Object<T> MakeObject([[maybe_unused]] Args... args)
{
    char *objPtr = new char [sizeof(DataHeader) + sizeof(T)];

// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    T *val = new (objPtr + sizeof(DataHeader)) T{args...};

//NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ((DataHeader *) (objPtr))->refCount = 1;

    Object<T> newObj{objPtr};

    return newObj;
}

// TODO(you): Implement object base on java object representatoin
template <class T>
class Object {
public:

    Object() = default;

    explicit Object(char *objPtr)
    {
        obj_ = objPtr;
    }

    explicit Object(std::nullptr_t) {}

    ~Object()
    {
        Release();
    }

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other)
    {
        obj_ = other.obj_;

        if (obj_ != nullptr)
        {
            ++(((DataHeader *) obj_)->refCount);
        }
    }
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Object<T> &operator=([[maybe_unused]] const Object<T> &other)
    {
        Release();

        obj_ = other.obj_;

        if (obj_ != nullptr)
        {
            ++(((DataHeader *) obj_)->refCount);
        }

        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other)
    {
        obj_ = std::move(other.obj_);

        other.obj_ = nullptr;
    }
    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
        Release();

        obj_ = std::move(other.obj_);

        other.obj_ = nullptr;

        return *this;
    }

    // member access operators
    T &operator*() const noexcept
    {
// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return *(T *) (obj_ + sizeof(DataHeader));
    }

    T *operator->() const noexcept
    {
// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return (T *) (obj_ + sizeof(DataHeader));
    }

    size_t UseCount() const
    {
        if (obj_ != nullptr)
        {
            // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
            return ((DataHeader *) obj_)->refCount;
        }

        return 0;
    }

    bool operator==([[maybe_unused]] const Object<T> other) const
    {
        return obj_ == other.obj_;
    }

    bool operator!=(const Object<T> other) const
    {
        return !(obj_ == other.obj_);
    }

    bool operator==(std::nullptr_t) const
    {
        return obj_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const
    {
        return obj_ != nullptr;
    }

private:
    void Release()
    {
        if (obj_ != nullptr)
        {
// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (--(((DataHeader *) obj_)->refCount) == 0)
            {
// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                ((T *)(obj_ + sizeof(DataHeader)))->~T();

                delete []obj_;

                obj_ = nullptr;
            }
        }
    }

    char *obj_ = nullptr;
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H
