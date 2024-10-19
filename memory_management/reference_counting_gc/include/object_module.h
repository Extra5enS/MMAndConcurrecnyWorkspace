#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H

#include <cstddef>
#include <utility>

template <class T>
class Object;

template <class T>
class Object {
public:
    Object() noexcept = default;

    explicit Object(T *ptr) : ptr_(ptr), header_(reinterpret_cast<Header *>(new char[sizeof(Header)]))
    {
        header_->ptr = ptr_;
        header_->rc = 1;
    }

    explicit Object(std::nullptr_t) noexcept {}

    ~Object()
    {
        if (header_ == nullptr) {
            return;
        }

        header_->rc--;

        if (header_->rc == 0) {
            ptr_->~T();
            delete header_->ptr;
            delete[] reinterpret_cast<char *>(header_);
        }
    }

    Object(const Object &other) : ptr_(other.ptr_), header_(other.header_)
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

        ptr_ = other.ptr_;
        header_ = other.header_;

        if (header_) {
            header_->rc++;
        }

        return *this;
    }

    Object(Object &&other) : ptr_(std::exchange(other.ptr_, nullptr)), header_(std::exchange(other.header_, nullptr)) {}

    Object &operator=(Object &&other)
    {
        if (this == &other) {
            return *this;
        }

        std::swap(ptr_, other.ptr_);
        std::swap(header_, other.header_);

        return *this;
    }

    T &operator*() const noexcept
    {
        return *ptr_;
    }

    T *operator->() const noexcept
    {
        return ptr_;
    }

    void Reset(T *ptr)
    {
        *this = Object(ptr);
    }

    T *Get() const
    {
        return ptr_;
    }

    size_t UseCount() const
    {
        if (header_ == nullptr) {
            return 0;
        }
        return header_->rc;
    }

    template <class U, class... Args>
    friend Object<U> MakeObject(Args &&...args);

private:
    struct Header {
        size_t rc;
        T *ptr;
        char valBuffer[];  // NOLINT(modernize-avoid-c-arrays)
    };

    T *ptr_ = nullptr;
    Header *header_ = nullptr;
};

template <class T, class... Args>
static Object<T> MakeObject(Args &&...args)
{
    using Header = typename Object<T>::Header;

    Object<T> obj;
    obj.header_ = reinterpret_cast<Header *>(new char[sizeof(Header) + sizeof(T)]);
    obj.header_->rc = 1;
    obj.header_->ptr = nullptr;

    new (&obj.header_->valBuffer) T(std::forward<Args>(args)...);
    obj.ptr_ = reinterpret_cast<T *>(&obj.header_->valBuffer);

    return obj;
}

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODEL_H
