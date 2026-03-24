#pragma once

#include "models/Item.h"
#include "models/Supplier.h"
#include "models/Shipment.h"
#include "models/PlanningResult.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace sca {

/**
 * @brief Describes a supplier disruption to simulate.
 *
 * Example: "What if supplier S1 is hit by a port strike adding 14 days?"
 */
struct DisruptionScenario {
    std::string supplier_id;
    int         extra_delay_days;
    std::string description;
};

/**
 * @brief Per-item impact of a disruption scenario.
 */
struct DisruptionImpact {
    std::string item_name;
    std::string supplier_id;
    RiskLevel   risk_before;
    RiskLevel   risk_after;
    int         days_until_stockout_before;
    int         days_until_stockout_after;
    bool        newly_critical;   ///< Escalated to High (worst) from a lower level
    bool        risk_escalated;   ///< Any upward risk movement
};

/**
 * @brief "What-if" disruption scenario analyser.
 *
 * Applies a supplier delay to a copy of the supplier map and re-runs
 * InventoryPlanner for all items supplied by that supplier.  The before/after
 * comparison shows which items are newly at risk — useful for understanding
 * the downstream impact of any supplier disruption.
 *
 * This is entirely value-based (no mutation of inputs) and safe to call
 * concurrently for different scenarios.
 */
class DisruptionAnalyzer {
public:
    /**
     * @brief Evaluate the impact of @p scenario on the item portfolio.
     *
     * Only items sourced from scenario.supplier_id are included in the output.
     *
     * @param items        Full item portfolio.
     * @param suppliers    Supplier lookup map.
     * @param shipment_map Latest known shipment per item_id.
     * @param scenario     Disruption to apply.
     * @return Impact per affected item, ordered by escalation severity (newly High first).
     */
    std::vector<DisruptionImpact> analyze(
        const std::vector<Item>&                          items,
        const std::unordered_map<std::string, Supplier>&  suppliers,
        const std::unordered_map<int, Shipment>&           shipment_map,
        const DisruptionScenario&                         scenario) const;
};

} // namespace sca
