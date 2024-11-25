#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H

// реализуйте потоко защищенный map (unordered_map), который был бы эффективным на чтения (обычно такие структуры читают намного чаще, чем изменяют )

#include <optional>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

template<class Key, class Val>
class ThreadSafeMap {
public:

    void Insert([[maybe_unused]] Key key,[[maybe_unused]] Val val) {
        std::unique_lock lock(mutex_);

        map_.insert({key, val});
    }

    // returns true if erase was completed successfully, otherwise false
    bool Erase([[maybe_unused]] Key key) {
        std::unique_lock lock(mutex_);

        auto elem = map_.find(key);

        if (elem != map_.end()) {
            map_.erase(elem);

            return true;
        }

        return false;
    }

    // return Val instance if object contains it, otherwise std::nullopt
    std::optional<Val> Get([[maybe_unused]] Key key) {
        std::shared_lock lock(mutex_);

        auto elem = map_.find(key);

        if (elem != map_.end()) {
            return elem->second;
        }

        return std::nullopt;
    }

    bool Test([[maybe_unused]] Key key) {
        return Get(key) != std::nullopt;
    }

private:
    std::unordered_map<Key, Val> map_;
    mutable std::shared_mutex mutex_;
};

#endif
