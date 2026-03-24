#include "planner/InventoryPlanner.h"

namespace sca {

PlanningResult InventoryPlanner::analyze(
    const Item&             item,
    const Supplier&         supplier,
    std::optional<Shipment> shipment) const
{
    if (item.daily_demand <= 0) {
        return PlanningResult{-1, 0, false, -1, RiskLevel::NoDemand, "No reorder needed"};
    }

    const int days_until_stockout = item.current_stock / item.daily_demand;
    const int delay_days          = shipment.has_value() ? shipment->delayed_days : 0;
    const int effective_lead_time = supplier.lead_time_days + delay_days;
    const int lead_time_demand    = item.daily_demand * effective_lead_time;
    const bool reorder_now        = item.current_stock <= item.reorder_point;

    int reorder_in_days = 0;
    if (!reorder_now) {
        const int stock_above_reorder = item.current_stock - item.reorder_point;
        reorder_in_days = stock_above_reorder / item.daily_demand;
    }

    RiskLevel   risk_level;
    std::string recommendation;

    if (days_until_stockout < effective_lead_time) {
        risk_level     = RiskLevel::High;
        recommendation = "Reorder now - likely stockout before replenishment";
    } else if (reorder_now) {
        risk_level     = RiskLevel::Medium;
        recommendation = "Reorder now";
    } else {
        risk_level     = RiskLevel::Low;
        recommendation = "Reorder in " + std::to_string(reorder_in_days) + " days";
    }

    return PlanningResult{
        days_until_stockout,
        lead_time_demand,
        reorder_now,
        reorder_in_days,
        risk_level,
        recommendation
    };
}

} // namespace sca
