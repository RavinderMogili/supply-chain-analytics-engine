#include "simulation/DisruptionAnalyzer.h"
#include "planner/InventoryPlanner.h"

#include <algorithm>
#include <optional>

namespace sca {

std::vector<DisruptionImpact> DisruptionAnalyzer::analyze(
    const std::vector<Item>&                          items,
    const std::unordered_map<std::string, Supplier>&  suppliers,
    const std::unordered_map<int, Shipment>&           shipment_map,
    const DisruptionScenario&                         scenario) const
{
    // Build a disrupted copy of the supplier map — inputs are never mutated
    auto disrupted_suppliers = suppliers;
    auto dis_it = disrupted_suppliers.find(scenario.supplier_id);
    if (dis_it != disrupted_suppliers.end())
        dis_it->second.lead_time_days += scenario.extra_delay_days;

    InventoryPlanner planner;
    std::vector<DisruptionImpact> impacts;

    for (const auto& item : items) {
        // Only analyse items sourced from the disrupted supplier
        if (item.supplier_id != scenario.supplier_id) continue;

        auto sup_it = suppliers.find(item.supplier_id);
        if (sup_it == suppliers.end()) continue;

        std::optional<Shipment> shipment;
        auto sh_it = shipment_map.find(item.item_id);
        if (sh_it != shipment_map.end())
            shipment = sh_it->second;

        const PlanningResult before = planner.analyze(item, sup_it->second, shipment);

        auto dis_sup_it = disrupted_suppliers.find(item.supplier_id);
        const PlanningResult after = planner.analyze(item, dis_sup_it->second, shipment);

        const bool newly_critical  = (before.risk_level != RiskLevel::High) &&
                                     (after.risk_level  == RiskLevel::High);
        const bool risk_escalated  = static_cast<int>(after.risk_level) >
                                     static_cast<int>(before.risk_level);

        impacts.push_back(DisruptionImpact{
            item.item_name,
            item.supplier_id,
            before.risk_level,
            after.risk_level,
            before.days_until_stockout,
            after.days_until_stockout,
            newly_critical,
            risk_escalated
        });
    }

    // Sort: newly critical first, then by risk escalation, then alphabetically
    std::sort(impacts.begin(), impacts.end(),
        [](const DisruptionImpact& a, const DisruptionImpact& b) {
            if (a.newly_critical != b.newly_critical) return a.newly_critical > b.newly_critical;
            if (a.risk_escalated != b.risk_escalated) return a.risk_escalated > b.risk_escalated;
            return a.item_name < b.item_name;
        });

    return impacts;
}

} // namespace sca
