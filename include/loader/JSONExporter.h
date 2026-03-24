#pragma once

#include "models/Item.h"
#include "models/PlanningResult.h"
#include "planner/RiskScorer.h"

#include <string>
#include <vector>

namespace sca {

/**
 * @brief Exports planning results to a structured JSON report file.
 *
 * Implements a lightweight JSON writer with no external dependencies.
 * All methods are static; JSONExporter holds no state.
 *
 * Output schema
 * ─────────────
 *  {
 *    "planning_results": [ { item fields + result fields }, ... ],
 *    "risk_ranking":     [ { item_name, score, status }, ... ]
 *  }
 */
class JSONExporter {
public:
    /**
     * @brief Write a full planning report to a JSON file.
     *
     * @param items        Inventory items (same order as @p results).
     * @param results      Planning results for each item.
     * @param risk_entries Risk-ranked entries from RiskScorer.
     * @param path         Output file path (e.g. "report.json").
     * @throws std::runtime_error if the file cannot be opened for writing.
     */
    static void export_report(
        const std::vector<Item>&           items,
        const std::vector<PlanningResult>& results,
        const std::vector<RiskEntry>&      risk_entries,
        const std::string&                 path);

private:
    static std::string escape(const std::string& s);
};

} // namespace sca
