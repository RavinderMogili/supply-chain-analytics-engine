#pragma once

#include "models/Item.h"
#include "models/Supplier.h"
#include "models/Shipment.h"
#include "models/PlanningResult.h"

#include <optional>

namespace sca {

/**
 * @brief Deterministic inventory risk analyser for a single item.
 *
 * All logic is rule-based and side-effect-free, making every result
 * fully deterministic, reproducible, and unit-testable without mocks.
 *
 * Risk classification rules
 * ─────────────────────────
 *  High   : days_until_stockout < effective_lead_time
 *           (stock will run out before replenishment arrives)
 *  Medium : current_stock <= reorder_point  (but not High)
 *  Low    : current_stock > reorder_point
 *
 * Key derived quantities
 * ──────────────────────
 *  days_until_stockout  = current_stock / daily_demand
 *  effective_lead_time  = supplier.lead_time_days + shipment.delayed_days
 *  lead_time_demand     = daily_demand * effective_lead_time
 */
class InventoryPlanner {
public:
    /**
     * @brief Analyse inventory risk for one item and generate a recommendation.
     *
     * @param item      The inventory item to evaluate.
     * @param supplier  The item's primary supplier (provides lead-time data).
     * @param shipment  Most recent shipment record, or std::nullopt if none.
     *                  When absent, the supplier's base lead time is used with
     *                  zero delay adjustment.
     * @return PlanningResult containing all computed metrics and recommendation.
     */
    PlanningResult analyze(
        const Item&                  item,
        const Supplier&              supplier,
        std::optional<Shipment>      shipment) const;
};

} // namespace sca
