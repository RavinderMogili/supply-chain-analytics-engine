#pragma once

#include <string>
#include <vector>

namespace sca {

/**
 * @brief Snapshot of one item's inventory state on a single simulated day.
 */
struct DaySnapshot {
    int  day;
    int  stock;
    bool reorder_triggered;
    bool in_stockout;
};

/**
 * @brief Full simulation timeline for one item across the planning horizon.
 *
 * first_stockout_day == -1 means the item never stocked out within the horizon.
 */
struct SimulationResult {
    int                      item_id;
    std::string              item_name;
    std::vector<DaySnapshot> timeline;
    int                      first_stockout_day;
    int                      reorder_count;
};

} // namespace sca
