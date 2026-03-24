#include "planner/RiskScorer.h"

#include <algorithm>
#include <cmath>

namespace sca {

std::vector<RiskEntry> RiskScorer::score(
    const std::vector<Item>&                         items,
    const std::unordered_map<std::string, Supplier>& suppliers,
    const std::vector<PlanningResult>&               results) const
{
    std::vector<RiskEntry> entries;
    entries.reserve(items.size());

    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item&          item   = items[i];
        const PlanningResult& result = results[i];

        // Supplier reliability: fall back to 0.5 if supplier not found
        double reliability = 0.5;
        auto it = suppliers.find(item.supplier_id);
        if (it != suppliers.end())
            reliability = it->second.reliability_score / 100.0;

        // Stockout proximity: ratio of lead-time demand to current stock.
        // Clamped to [0, 1]: 1.0 means demand during lead time >= current stock.
        double stockout_ratio = 0.0;
        if (result.days_until_stockout >= 0 && item.current_stock > 0) {
            stockout_ratio = std::min(
                1.0,
                static_cast<double>(result.lead_time_demand)
                    / static_cast<double>(item.current_stock));
        }

        // Composite score: stockout proximity weighted more heavily than
        // supplier unreliability
        const double unreliability = 1.0 - reliability;
        double composite = 0.6 * stockout_ratio + 0.4 * unreliability;
        composite = std::max(0.0, std::min(1.0, composite));

        std::string summary;
        if      (composite >= 0.7) summary = "Critical";
        else if (composite >= 0.4) summary = "Elevated";
        else                       summary = "Normal";

        entries.push_back(RiskEntry{item.item_name, composite, summary});
    }

    // Sort highest risk first
    std::sort(entries.begin(), entries.end(),
        [](const RiskEntry& a, const RiskEntry& b) {
            return a.score > b.score;
        });

    return entries;
}

} // namespace sca
