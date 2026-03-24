#include "planner/PlannerPipeline.h"
#include "planner/InventoryPlanner.h"

#include <expected>
#include <optional>

namespace sca {

PipelineResult PlannerPipeline::run(
    const Item&                                       item,
    const std::unordered_map<std::string, Supplier>&  suppliers,
    const std::unordered_map<int, Shipment>&           shipment_map) const
{
    // Validate: supplier must exist
    auto sup_it = suppliers.find(item.supplier_id);
    if (sup_it == suppliers.end())
        return std::unexpected{"Supplier not found: " + item.supplier_id};

    // Validate: stock must be non-negative
    if (item.current_stock < 0)
        return std::unexpected{
            "Invalid stock level (" + std::to_string(item.current_stock) +
            ") for item: " + item.item_name};

    std::optional<Shipment> shipment;
    auto sh_it = shipment_map.find(item.item_id);
    if (sh_it != shipment_map.end())
        shipment = sh_it->second;

    InventoryPlanner planner;
    return planner.analyze(item, sup_it->second, shipment);
}

} // namespace sca
