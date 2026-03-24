#pragma once

#include "models/PlanningResult.h"

#include <list>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <atomic>
#include <cstddef>

namespace sca {

/**
 * @brief Thread-safe LRU cache for PlanningResult objects.
 *
 * Caches results keyed on the five scalar inputs that fully determine
 * InventoryPlanner output: current_stock, daily_demand, reorder_point,
 * lead_time_days, delayed_days.
 *
 * Motivation: in a cloud-hosted planning system every item evaluation
 * may require a round-trip to fetch supplier/inventory data.  Caching
 * previously computed results avoids redundant reads for items whose
 * inputs have not changed, directly addressing the "small, frequent
 * cloud reads" performance bottleneck.
 *
 * Capacity policy: when full, the least-recently-used entry is evicted.
 * All public methods are safe to call concurrently from multiple threads.
 */
class PlannerCache {
public:
    explicit PlannerCache(std::size_t capacity = 256);

    /**
     * @brief Look up a cached result.
     * @return Cached PlanningResult on hit, std::nullopt on miss.
     */
    std::optional<PlanningResult> get(
        int current_stock,
        int daily_demand,
        int reorder_point,
        int lead_time_days,
        int delayed_days) const;

    /**
     * @brief Insert or refresh a result in the cache.
     *
     * If the key already exists its value is updated and it becomes the
     * most-recently-used entry.  If the cache is full the LRU entry is
     * evicted before insertion.
     */
    void put(
        int current_stock,
        int daily_demand,
        int reorder_point,
        int lead_time_days,
        int delayed_days,
        const PlanningResult& result);

    std::size_t size()     const;
    std::size_t capacity() const;
    std::size_t hits()     const;
    std::size_t misses()   const;
    double      hit_rate() const;

private:
    struct Key {
        int current_stock;
        int daily_demand;
        int reorder_point;
        int lead_time_days;
        int delayed_days;

        bool operator==(const Key& o) const noexcept {
            return current_stock  == o.current_stock  &&
                   daily_demand   == o.daily_demand   &&
                   reorder_point  == o.reorder_point  &&
                   lead_time_days == o.lead_time_days &&
                   delayed_days   == o.delayed_days;
        }
    };

    struct KeyHash {
        std::size_t operator()(const Key& k) const noexcept {
            // FNV-1a inspired mixing
            std::size_t h = 14695981039346656037ULL;
            auto mix = [&](int v) {
                h ^= static_cast<std::size_t>(v);
                h *= 1099511628211ULL;
            };
            mix(k.current_stock);
            mix(k.daily_demand);
            mix(k.reorder_point);
            mix(k.lead_time_days);
            mix(k.delayed_days);
            return h;
        }
    };

    using Entry  = std::pair<Key, PlanningResult>;
    using ListIt = typename std::list<Entry>::iterator;

    std::size_t capacity_;

    mutable std::atomic<std::size_t> hits_{0};
    mutable std::atomic<std::size_t> misses_{0};

    mutable std::list<Entry>                           lru_list_;
    mutable std::unordered_map<Key, ListIt, KeyHash>   map_;
    mutable std::mutex                                 mutex_;
};

} // namespace sca
