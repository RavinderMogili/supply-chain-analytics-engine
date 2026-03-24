#pragma once

#include "simulation/DisruptionAnalyzer.h"
#include "models/Item.h"
#include "models/Supplier.h"
#include "models/Shipment.h"

#include <vector>
#include <unordered_map>
#include <chrono>

namespace sca {

/**
 * @brief Summary of one disruption scenario's analysis results.
 */
struct ScenarioReport {
    DisruptionScenario            scenario;
    std::vector<DisruptionImpact> impacts;
    int                           newly_high_risk_count;  ///< Items that escalated to High
    int                           escalated_count;        ///< Items with any risk increase
    double                        elapsed_ms;             ///< Wall-clock time for this scenario
};

/**
 * @brief Runs multiple disruption scenarios in parallel using std::async.
 *
 * Motivation (from industry research):
 *   Agile supply chain systems must evaluate *many* what-if scenarios
 *   simultaneously — e.g. "what if S1 is delayed 7 days AND S2 is delayed
 *   14 days AND there's a raw-material shortage?" — so planners can compare
 *   outcomes and choose a mitigation strategy before committing resources.
 *
 * Implementation:
 *   Each scenario is dispatched as an independent std::async task.  Because
 *   DisruptionAnalyzer is stateless and operates on immutable inputs, all
 *   tasks run safely without locks.  Results are collected once all futures
 *   resolve.
 *
 * This mirrors the concurrent scenario-evaluation pattern used in enterprise
 * supply chain planning systems.
 */
class ScenarioRunner {
public:
    /**
     * @brief Evaluate all @p scenarios concurrently.
     *
     * @param scenarios    List of disruption scenarios to evaluate.
     * @param items        Item portfolio (shared, read-only across tasks).
     * @param suppliers    Supplier lookup map (shared, read-only).
     * @param shipment_map Latest shipment per item_id (shared, read-only).
     * @return ScenarioReport for each scenario, in the same order as input.
     */
    std::vector<ScenarioReport> run_all(
        const std::vector<DisruptionScenario>&            scenarios,
        const std::vector<Item>&                          items,
        const std::unordered_map<std::string, Supplier>&  suppliers,
        const std::unordered_map<int, Shipment>&           shipment_map) const;
};

} // namespace sca
