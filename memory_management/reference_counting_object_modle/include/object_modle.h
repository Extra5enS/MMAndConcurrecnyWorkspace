#ifndef MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H
#define MEMORY_MANAGEMENT_REFERECNCE_COUNTING_OBJECT_MODLE_INCLUDE_OBJECT_MODLE_H

#include <cstddef>

template <class T>
class Object;

template <class T, class... Args>
static Object<T> MakeObject([[maybe_unused]] Args... args)
{
    // TODO(you): Implement this function
    return {};
}

// TODO(you): Implement object base on java object representatoin
template <class T>
class Object {
public:
    Object() = default;
    explicit Object(std::nullptr_t) {}

    ~Object() = default;  // this method should be changed

    // copy semantic
    Object([[maybe_unused]] const Object<T> &other) {}
    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    Object<T> &operator=([[maybe_unused]] const Object<T> &other)
    {
        return *this;
    }

    // move semantic
    Object([[maybe_unused]] Object<T> &&other) {}
    Object<T> &operator=([[maybe_unused]] Object<T> &&other)
    {
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

    size_t UseCount() const
    {
        return 0;
    }

    bool operator==([[maybe_unused]] const Object<T> other) const
    {
        return false;
    }

    bool operator!=(const Object<T> other) const
    {
        return !(*this == other);
    }

    bool operator==(std::nullptr_t) const
    {
        return false;
    }

    bool operator!=(std::nullptr_t) const
    {
        return false;
    }

private:
    // Add your constructor

    // TODO(you): Add your fields and methods here...
    T *val_ = nullptr;  // this field can be deleted
};

#endif  // MEMORY_MANAGEMENT_REFERECNCE_COUNTING_GC_INCLUDE_OBJECT_MODLE_H