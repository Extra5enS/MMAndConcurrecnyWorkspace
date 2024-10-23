#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H

// реализуйте потоко защищенный map (unordered_map), который был бы эффективным на чтения (обычно такие структуры читают намного чаще, чем изменяют )

#include <optional>

template<class Key, class Val>
class ThreadSafeMap {
public:

    void Insert([[maybe_unused]] Key key,[[maybe_unused]] Val val) {
        // ... implement this
    }

    // returns true if erase was completed successfully, otherwise false
    bool Erase([[maybe_unused]] Key key) {
        // ... implement this
        return false;
    }

    // return Val instance if object contains it, otherwise std::nullopt
    std::optional<Val> Get([[maybe_unused]] Key key) {
        // ... implement this
        return std::nullopt;
    }

    bool Test([[maybe_unused]] Key key) {
        return Get(key) != std::nullopt;
    }

private:
};

#endif