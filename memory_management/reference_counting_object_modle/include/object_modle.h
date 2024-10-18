#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>
#include <utility>

template <class T>
class Object;

template <class T>
class Object {
public:
    Object() = default;
    explicit Object(std::nullptr_t) {}

    ~Object()
    {
        if (hdr_ == nullptr) {
            return;
        }

        hdr_->rc--;

        if (hdr_->rc == 0) {
            this->GetValue()->~T();
            delete[] hdr_;
        }
    }

    Object(const Object &other) noexcept : hdr_(other.hdr_)
    {
        if (hdr_) {
            hdr_->rc++;
        }
    }

    Object &operator=(const Object &other)
    {
        if (this == &other) {
            return *this;
        }

        this->~Object();

        hdr_ = other.hdr_;

        if (hdr_) {
            hdr_->rc++;
        }

        return *this;
    }

    Object(Object &&other) noexcept : hdr_(std::exchange(other.hdr_, nullptr)) {}

    Object &operator=(Object &&other)
    {
        if (this == &other) {
            return *this;
        }

        std::swap(hdr_, other.hdr_);

        return *this;
    }

    T &operator*() noexcept
    {
        return *GetValue();
    }

    const T &operator*() const noexcept
    {
        return *GetValue();
    }

    T *operator->() noexcept
    {
        return GetValue();
    }

    const T *operator->() const noexcept
    {
        return GetValue();
    }

    size_t UseCount() const noexcept
    {
        if (hdr_ == nullptr) {
            return 0;
        }

        return hdr_->rc;
    }

    bool operator==(const Object &other) const
    {
        return hdr_ == other.hdr_;
    }

    bool operator!=(const Object &other) const
    {
        return !(*this == other);
    }

    bool operator==(std::nullptr_t) const noexcept
    {
        return hdr_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const noexcept
    {
        return hdr_ != nullptr;
    }

    template <class U, class... Args>
    friend Object<U> MakeObject(Args &&...args);

private:
    struct Header {
        size_t rc;
    };

    Header *hdr_ = nullptr;

    explicit Object(Header *header) noexcept : hdr_(header) {}

    T *GetValue() const noexcept
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return reinterpret_cast<T *>(hdr_ + 1);
    }
};

template <class T, class... Args>
Object<T> MakeObject(Args &&...args)
{
    using Header = typename Object<T>::Header;

    auto *header = reinterpret_cast<Header *>(new char[sizeof(Header) + sizeof(T)]);
    if (header == nullptr) {
        return {};
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    new (header + 1) T(std::forward<Args>(args)...);

    header->rc = 1;

    return Object<T>(reinterpret_cast<Header *>(header));
}

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H
