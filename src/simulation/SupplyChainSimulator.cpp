#include "simulation/SupplyChainSimulator.h"

#include <algorithm>

namespace sca {

SupplyChainSimulator::SupplyChainSimulator(int horizon_days)
    : horizon_days_(horizon_days) {}

std::vector<SimulationResult> SupplyChainSimulator::simulate(
    const std::vector<Item>&                         items,
    const std::unordered_map<std::string, Supplier>& suppliers) const
{
    std::vector<SimulationResult> results;
    results.reserve(items.size());

    for (const auto& item : items) {
        auto sup_it = suppliers.find(item.supplier_id);
        if (sup_it == suppliers.end()) continue;

        const Supplier& sup = sup_it->second;

        SimulationResult result;
        result.item_id            = item.item_id;
        result.item_name          = item.item_name;
        result.first_stockout_day = -1;
        result.reorder_count      = 0;
        result.timeline.reserve(static_cast<std::size_t>(horizon_days_));

        int  stock         = item.current_stock;
        int  order_arrives = -1;
        bool order_pending = false;

        for (int day = 1; day <= horizon_days_; ++day) {
            // Receive pending order if it arrives today
            if (order_pending && day == order_arrives) {
                stock        += item.reorder_qty;
                order_pending = false;
                order_arrives = -1;
            }

            // Consume daily demand
            stock -= item.daily_demand;

            // Trigger reorder if at or below reorder point and no order pending
            bool reorder_triggered = false;
            if (stock <= item.reorder_point && !order_pending) {
                order_pending    = true;
                order_arrives    = day + sup.lead_time_days;
                reorder_triggered = true;
                ++result.reorder_count;
            }

            // Record stockout
            const bool in_stockout = stock < 0;
            if (in_stockout && result.first_stockout_day == -1)
                result.first_stockout_day = day;

            // Clamp stock — cannot go below zero in the model
            stock = std::max(stock, 0);

            result.timeline.push_back(
                DaySnapshot{day, stock, reorder_triggered, in_stockout});
        }

        results.push_back(std::move(result));
    }

    return results;
}

} // namespace sca
