#pragma once

#include <string>

namespace sca {

/**
 * @brief Risk classification for a single item.
 */
enum class RiskLevel {
    NoDemand, ///< Item has zero daily demand; no planning action needed.
    Low,      ///< Stock is above the reorder point with buffer days remaining.
    Medium,   ///< Stock is at or below the reorder point but not yet critical.
    High      ///< Stock will run out before replenishment can arrive.
};

/**
 * @brief Convert a RiskLevel to its display string.
 */
inline std::string to_string(RiskLevel level) {
    switch (level) {
        case RiskLevel::NoDemand: return "No Demand";
        case RiskLevel::Low:      return "Low";
        case RiskLevel::Medium:   return "Medium";
        case RiskLevel::High:     return "High";
    }
    return "Unknown";
}

/**
 * @brief Output produced by InventoryPlanner::analyze() for a single item.
 *
 * All values are computed deterministically from Item, Supplier, and Shipment
 * inputs so the result is fully reproducible and unit-testable.
 */
struct PlanningResult {
    int         days_until_stockout; ///< Days of stock at current demand. -1 = no demand.
    int         lead_time_demand;    ///< Units consumed during the effective lead time.
    bool        reorder_now;         ///< True when current_stock <= reorder_point.
    int         reorder_in_days;     ///< Days until reorder point is reached. 0 = reorder now.
    RiskLevel   risk_level;          ///< Classified risk level.
    std::string recommendation;      ///< Plain-language action recommendation.
};

} // namespace sca
