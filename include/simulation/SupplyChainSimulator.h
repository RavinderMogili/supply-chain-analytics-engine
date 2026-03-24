#pragma once

#include "models/Item.h"
#include "models/Supplier.h"
#include "simulation/SimulationResult.h"

#include <vector>
#include <unordered_map>

namespace sca {

/**
 * @brief Day-by-day digital twin of the supply chain.
 *
 * Simulates inventory evolution over a configurable horizon for each item,
 * modelling daily demand consumption, reorder-point triggers, and order
 * receipt after supplier lead time.
 *
 * The planner asks "what does my stock level look like 30 days from now,
 * given current demand and supplier performance?"
 *
 * Simulation rules per day (per item):
 *  1. If a pending order is due today, receive it (stock += reorder_qty).
 *  2. Consume daily demand (stock -= daily_demand).
 *  3. If stock <= reorder_point and no order is pending, trigger a reorder;
 *     it will arrive in lead_time_days days.
 *  4. Clamp stock to 0 (stockouts cannot go negative in the model).
 */
class SupplyChainSimulator {
public:
    /**
     * @param horizon_days Number of days to simulate (default 30).
     */
    explicit SupplyChainSimulator(int horizon_days = 30);

    /**
     * @brief Simulate all items over the planning horizon.
     *
     * Items whose supplier is not in @p suppliers are silently skipped.
     *
     * @param items     Item portfolio to simulate.
     * @param suppliers Supplier lookup used for lead-time information.
     * @return SimulationResult for every matched item.
     */
    std::vector<SimulationResult> simulate(
        const std::vector<Item>&                         items,
        const std::unordered_map<std::string, Supplier>& suppliers) const;

    int horizon_days() const { return horizon_days_; }

private:
    int horizon_days_;
};

} // namespace sca
