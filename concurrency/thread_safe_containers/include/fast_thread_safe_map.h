#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H

#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include <utility>

template<class Key, class Val>
class ThreadSafeMap {
public:
    void Insert(Key key, Val val) {
        std::unique_lock<std::shared_mutex> lock(lock_);
        map_.insert(std::make_pair(key, val));
    }

    // returns true if erase was completed successfully, otherwise false
    bool Erase(Key key) {
        std::unique_lock<std::shared_mutex> lock(lock_);
        return map_.erase(key) != 0;
    }

    // return Val instance if object contains it, otherwise std::nullopt
    std::optional<Val> Get(Key key) {
        std::shared_lock<std::shared_mutex> lock(lock_);
        auto valIt = map_.find(key);
        if (valIt == map_.end()) {
            return std::nullopt;
        }
        return valIt->second;
    }

    bool Test(Key key) {
        return Get(key) != std::nullopt;
    }

private:
    std::unordered_map<Key, Val> map_;
    std::shared_mutex lock_;
};

#endif
