#include "planner/PlannerCache.h"

namespace sca {

PlannerCache::PlannerCache(std::size_t capacity)
    : capacity_(capacity) {}

std::optional<PlanningResult> PlannerCache::get(
    int current_stock,
    int daily_demand,
    int reorder_point,
    int lead_time_days,
    int delayed_days) const
{
    Key key{current_stock, daily_demand, reorder_point, lead_time_days, delayed_days};

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);
    if (it == map_.end()) {
        ++misses_;
        return std::nullopt;
    }

    // Promote to most-recently-used
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
    ++hits_;
    return it->second->second;
}

void PlannerCache::put(
    int current_stock,
    int daily_demand,
    int reorder_point,
    int lead_time_days,
    int delayed_days,
    const PlanningResult& result)
{
    Key key{current_stock, daily_demand, reorder_point, lead_time_days, delayed_days};

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = map_.find(key);
    if (it != map_.end()) {
        it->second->second = result;
        lru_list_.splice(lru_list_.begin(), lru_list_, it->second);
        return;
    }

    // Evict least-recently-used entry when at capacity
    if (lru_list_.size() >= capacity_) {
        auto lru = std::prev(lru_list_.end());
        map_.erase(lru->first);
        lru_list_.erase(lru);
    }

    lru_list_.emplace_front(key, result);
    map_[key] = lru_list_.begin();
}

std::size_t PlannerCache::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return lru_list_.size();
}

std::size_t PlannerCache::capacity() const { return capacity_; }
std::size_t PlannerCache::hits()     const { return hits_.load();   }
std::size_t PlannerCache::misses()   const { return misses_.load(); }

double PlannerCache::hit_rate() const {
    auto h = hits_.load();
    auto m = misses_.load();
    auto total = h + m;
    return total == 0 ? 0.0 : static_cast<double>(h) / static_cast<double>(total);
}

} // namespace sca
