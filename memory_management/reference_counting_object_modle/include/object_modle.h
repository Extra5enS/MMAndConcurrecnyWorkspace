#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>
#include <functional>
#include <iostream>

template <class T>
class Object;
class MarkWord;
class Header;

class MarkWord
{
public:
    MarkWord() : count_(new size_t{1}) {}

    explicit MarkWord(std::nullptr_t) {}

    MarkWord([[maybe_unused]] const MarkWord &other) : count_(other.count_)
    {
        (*count_)++;
    }

    ~MarkWord() 
    {
        ProcessMemRelease();
    }

    MarkWord &operator=([[maybe_unused]] const MarkWord &other)
    {
        if (this == &other)
        {
            return *this;
        }

        ProcessMemRelease();

        count_ = other.count_;
        (*count_)++;

        return *this;
    }

    MarkWord(MarkWord &&other)
    {
        std::swap(count_, other.count_);
    }

    MarkWord &operator=([[maybe_unused]] MarkWord &&other)
    {
        if (this != &other)
        {
            std::swap(count_, other.count_);
        }

        return *this;
    }

    size_t GetCount() const
    {
        if (count_ == nullptr)
        {
            return 0;
        }

        return *count_;
    }

private:
    size_t *count_ = nullptr;

    void ProcessMemRelease()
    {
        if (GetCount() <= 1)
        {
            delete count_;
        }
        else
        {
            (*count_)--;
        }
    }
};

class Header
{
public:
    Header() = default;
    ~Header() = default;

    explicit Header(std::nullptr_t) : markWord_(nullptr) {}

    Header([[maybe_unused]] const Header &other) = default;

    Header &operator=([[maybe_unused]] const Header &other)
    {
        if (this != &other)
        {
            markWord_ = other.markWord_;
        }

        return *this;
    }

    Header(Header &&other)
    {
        std::swap(markWord_, other.markWord_);
    }

    Header &operator=([[maybe_unused]] Header &&other)
    {
        if (this != &other)
        {
            std::swap(markWord_, other.markWord_);
        }

        return *this;
    }

    size_t GetCount() const
    {
        return markWord_.GetCount();
    }

private:
    MarkWord markWord_;
};

template <class T, class... Args>
static Object<T> MakeObject([[maybe_unused]] Args... args)
{
    T *val = new T{args...};
    Object<T> obj{val};
    return obj;
}

// TODO(you): Implement object base on java object representatoin
template <class T>
class Object {
public:
    Object() : data_(nullptr), header_(nullptr) {}

    explicit Object(std::nullptr_t) : data_(nullptr), header_(nullptr) {}

    explicit Object([[maybe_unused]] T *ptr) : data_(ptr) {}

    ~Object()
    {
        ProcessMemRelease();
    }

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other) : header_(other.header_), data_(other.data_) {}

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Object<T> &operator=([[maybe_unused]] const Object<T> &other)
    {
        if (this == &other)
        {
            return *this;
        }

        ProcessMemRelease();

        data_ = other.data_;
        header_ = other.header_;
        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other)
    {
        std::swap(header_, other.header_);
        std::swap(data_, other.data_);
    }

    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
        if (this != &other)
        {
            std::swap(header_, other.header_);
            std::swap(data_, other.data_);
        }

        return *this;
    }

    // member access operators
    T &operator*() const noexcept
    {
        return *data_;
    }

    T *operator->() const noexcept
    {
        return data_;
    }

    size_t UseCount() const
    {
        return header_.GetCount();
    }

    bool operator==([[maybe_unused]] const Object<T> other) const
    {
        return data_ == other.data_;
    }

    bool operator!=(const Object<T> other) const
    {
        return !(*this == other);
    }

    bool operator==(std::nullptr_t) const
    {
        return data_ == nullptr;
    }

    bool operator!=(std::nullptr_t) const
    {
        return data_ != nullptr;
    }

private:
    Header  header_; 
    T       *data_  = nullptr;

    void ProcessMemRelease()
    {
        if (UseCount() <= 1)
        {
            delete data_;
        }
    }
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H