#pragma once

#include "models/Item.h"
#include "models/Supplier.h"
#include "models/Shipment.h"
#include "models/PlanningResult.h"

#include <expected>
#include <string>
#include <unordered_map>

namespace sca {

/**
 * @brief Typed pipeline result using std::expected (C++23).
 *
 * Holds either a successful PlanningResult or a descriptive error string.
 * The caller uses .has_value(), .value(), and .error() to distinguish
 * outcomes — demonstrating the modern C++23 approach to error propagation
 * without exceptions, null pointers, or error codes.
 *
 * std::expected is purpose-built for this pattern (unlike std::variant which
 * requires std::visit or std::get, both of which carry type-safety overhead).
 */
using PipelineResult = std::expected<PlanningResult, std::string>;

/**
 * @brief std::variant-based single-item planning pipeline.
 *
 * Validates inputs (supplier lookup, stock sanity) before delegating to
 * InventoryPlanner::analyze().  Errors are returned as values rather than
 * thrown as exceptions, making the error path explicit and testable.
 *
 * Usage with std::expected API:
 * @code
 *   auto r = pipeline.run(item, suppliers, shipments);
 *   if (r.has_value())
 *       std::cout << to_string(r.value().risk_level) << "\n";
 *   else
 *       std::cerr << "Error: " << r.error() << "\n";
 * @endcode
 */
class PlannerPipeline {
public:
    /**
     * @brief Run the full planning pipeline for one item.
     *
     * @param item         Item to evaluate.
     * @param suppliers    Supplier lookup map.
     * @param shipment_map Latest shipment per item_id.
     * @return PlanningResult on success, or an error string if supplier
     *         is not found or stock data is invalid.
     */
    PipelineResult run(
        const Item&                                       item,
        const std::unordered_map<std::string, Supplier>&  suppliers,
        const std::unordered_map<int, Shipment>&           shipment_map) const;
};

} // namespace sca
