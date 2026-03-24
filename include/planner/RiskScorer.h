#pragma once

#include "models/Item.h"
#include "models/Supplier.h"
#include "models/PlanningResult.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace sca {

/**
 * @brief Composite risk score entry for a single item.
 */
struct RiskEntry {
    std::string item_name; ///< Name of the evaluated item.
    double      score;     ///< Composite score: 0.0 (no risk) … 1.0 (max risk).
    std::string summary;   ///< Textual label: "Critical", "Elevated", or "Normal".
};

/**
 * @brief Computes and ranks composite supply risk scores across a portfolio.
 *
 * The composite score blends two signals:
 *  - Stockout proximity  (weight 0.6): how close the item is to running out
 *    relative to its effective lead-time demand.
 *  - Supplier unreliability (weight 0.4): 1.0 – (reliability_score / 100).
 *
 * Thresholds:
 *  score >= 0.7  → "Critical"
 *  score >= 0.4  → "Elevated"
 *  score <  0.4  → "Normal"
 */
class RiskScorer {
public:
    /**
     * @brief Score all items and return them ranked highest-risk first.
     *
     * @param items     All inventory items (same order as @p results).
     * @param suppliers Map of supplier_id → Supplier for O(1) lookup.
     * @param results   Pre-computed PlanningResult for each item in @p items.
     * @return          RiskEntry list sorted by score descending.
     */
    std::vector<RiskEntry> score(
        const std::vector<Item>&                         items,
        const std::unordered_map<std::string, Supplier>& suppliers,
        const std::vector<PlanningResult>&               results) const;
};

} // namespace sca
