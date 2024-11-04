#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H

#include <optional>
#include <unordered_map>

template<class Key, class Val>
class ThreadSafeMap {
public:

    void Insert([[maybe_unused]] Key key,[[maybe_unused]] Val val) {
        std::unique_lock<std::mutex> lock(mutex_);
        map_.insert(std::make_pair(key, val));
    }

    // returns true if erase was completed successfully, otherwise false
    bool Erase([[maybe_unused]] Key key) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        auto hit = map_.find(key);

        if (hit == map_.end())
            return false;
        
        map_.erase(hit);

        return true;
    }

    // return Val instance if object contains it, otherwise std::nullopt
    std::optional<Val> Get([[maybe_unused]] Key key) {
        std::unique_lock<std::mutex> lock(mutex_);

        auto hit = map_.find(key);

        if (hit == map_.end())
            return std::nullopt;

        return std::optional<Val>(hit->second);
    }

    bool Test([[maybe_unused]] Key key) {
        return Get(key) != std::nullopt;
    }

private:
    std::mutex mutex_;
    std::condition_variable condVar_;
    std::unordered_map<Key, Val> map_;
};

#endif