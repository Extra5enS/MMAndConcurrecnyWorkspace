#ifndef CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H
#define CONCURRENCY_THREAD_SAFE_CONTAINERS_INCLUDE_FAST_THREAD_SAFE_MAP_H

// реализуйте потоко защищенный map (unordered_map), который был бы эффективным на чтения (обычно такие структуры читают
// намного чаще, чем изменяют )

#include <optional>
#include <mutex>
#include <unordered_map>
#include <shared_mutex>

template <class Key, class Val>
class ThreadSafeMap {
public:
    void Insert(Key key, Val val)
    {
        std::unique_lock lock {mutex_};

        map_[key] = std::move(val);
    }

    // returns true if erase was completed successfully, otherwise false
    bool Erase(Key key)
    {
        std::unique_lock lock {mutex_};

        auto it = map_.find(key);
        if (it == map_.cend()) {
            return false;
        }

        map_.erase(it);

        return true;
    }

    // return Val instance if object contains it, otherwise std::nullopt
    std::optional<Val> Get(Key key)
    {
        std::shared_lock lock {mutex_};

        auto it = map_.find(key);

        return it != map_.cend() ? std::make_optional(it->second) : std::nullopt;
    }

    bool Test(Key key)
    {
        return Get(key) != std::nullopt;
    }

private:
    std::shared_mutex mutex_;
    std::unordered_map<Key, Val> map_;
};

#endif
