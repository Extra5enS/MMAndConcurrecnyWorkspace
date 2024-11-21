#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>

template <class T>
class Object;

struct ObjectHeader {
    size_t refCount;
};

template <class T, class... Args>
static Object<T> MakeObject(Args... args)
{
    auto *header = reinterpret_cast<ObjectHeader*>(new char[sizeof(ObjectHeader) + sizeof(T)]);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,-warnings-as-errors)
    T *val = new (header + 1) T{args...};
    header->refCount = 1;
    return Object<T>{header};
}

// TODO(you): Implement object base on java object representatoin
template <class T>
class Object {
public:
    Object() = default;
    explicit Object(std::nullptr_t) {}
    explicit Object(ObjectHeader *header) : header_(header) {}

    ~Object()
    {
        Release();
    }

    // copy semantic
    Object(const Object<T> &other) : header_(other.header_)
    {
        ++header_->refCount;
    }
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Object<T> &operator=(const Object<T> &other)
    {
        if (this != &other) {
            Release();
            header_ = other.header_;
            ++header_->refCount;
        }

        return *this;
    }

    // move semantic
    Object(Object<T> &&other) : header_(other.header_)
    {
        other.header_ = nullptr;
    }
    Object<T> &operator=(Object<T> &&other)
    {
        if (this != &other) {
            Release();
            header_ = other.header_;
            other.header_ = nullptr;
        }
        return *this;
    }

    // member access operators
    T &operator*() const noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,-warnings-as-errors)
        return *reinterpret_cast<T*>(header_ + 1);
    }

    T *operator->() const noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,-warnings-as-errors)
        return reinterpret_cast<T*>(header_ + 1);
    }

    size_t UseCount() const
    {
        if (header_ == nullptr) {
            return 0;
        }

        return header_->refCount;
    }

    bool operator==(const Object<T> other) const
    {
        return header_ == other.header_;
    }

    bool operator!=(const Object<T> other) const
    {
        return !(*this == other);
    }

    bool operator==(std::nullptr_t) const
    {
        return header_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const
    {
        return !(*this == nullptr);
    }

private:
    ObjectHeader* header_ = nullptr;

    void Release()
    {
        if (header_ == nullptr) {
            return;
        }
        if (--header_->refCount == 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,-warnings-as-errors)
            reinterpret_cast<T*>(header_ + 1)->~T();
            delete [] header_;
        }
    }
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H
