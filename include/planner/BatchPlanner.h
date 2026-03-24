#pragma once

#include "models/Item.h"
#include "models/Supplier.h"
#include "models/Shipment.h"
#include "models/PlanningResult.h"
#include "planner/PlannerCache.h"

#include <vector>
#include <unordered_map>

namespace sca {

/**
 * @brief Parallel batch planner using std::async + optional PlannerCache.
 *
 * Demonstrates the request-batching pattern used to amortise per-call
 * overhead in cloud-based planning systems:
 *
 *  1. The item portfolio is split into fixed-size batches.
 *  2. Each batch is dispatched as an independent std::async task so all
 *     batches execute concurrently on the thread pool.
 *  3. Before invoking InventoryPlanner::analyze(), each task checks the
 *     shared PlannerCache to avoid recomputing results for items whose
 *     inputs have not changed (cache-hit = zero computation cost).
 *
 * In a real cloud system the "computation" would be replaced by an HTTP
 * call to a data service; the same pattern eliminates redundant network
 * round-trips and saturates available concurrency.
 */
class BatchPlanner {
public:
    /**
     * @param batch_size  Number of items per async task (default 256).
     * @param cache       Optional shared cache; pass nullptr to disable.
     */
    explicit BatchPlanner(int batch_size = 256, PlannerCache* cache = nullptr);

    /**
     * @brief Analyse all items using parallel batches.
     *
     * @param items        Full item portfolio.
     * @param suppliers    Supplier lookup map (supplier_id → Supplier).
     * @param shipment_map Latest shipment per item  (item_id → Shipment).
     * @return PlanningResult for every item, in the same order as @p items.
     */
    std::vector<PlanningResult> analyze_all(
        const std::vector<Item>&                          items,
        const std::unordered_map<std::string, Supplier>&  suppliers,
        const std::unordered_map<int, Shipment>&           shipment_map) const;

private:
    int           batch_size_;
    PlannerCache* cache_;
};

} // namespace sca
