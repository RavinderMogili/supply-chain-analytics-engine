#include "planner/BatchPlanner.h"
#include "planner/InventoryPlanner.h"

#include <future>
#include <optional>
#include <algorithm>

namespace sca {

BatchPlanner::BatchPlanner(int batch_size, PlannerCache* cache)
    : batch_size_(batch_size), cache_(cache) {}

std::vector<PlanningResult> BatchPlanner::analyze_all(
    const std::vector<Item>&                          items,
    const std::unordered_map<std::string, Supplier>&  suppliers,
    const std::unordered_map<int, Shipment>&           shipment_map) const
{
    const int n = static_cast<int>(items.size());
    std::vector<PlanningResult> results(static_cast<std::size_t>(n));

    // Launch one async task per batch
    std::vector<std::future<void>> futures;
    futures.reserve(static_cast<std::size_t>((n + batch_size_ - 1) / batch_size_));

    for (int start = 0; start < n; start += batch_size_) {
        const int end = std::min(start + batch_size_, n);

        futures.emplace_back(
            std::async(std::launch::async,
                [this, &items, &suppliers, &shipment_map, &results, start, end]()
                {
                    InventoryPlanner planner;

                    for (int i = start; i < end; ++i) {
                        const Item& item = items[static_cast<std::size_t>(i)];

                        auto sup_it = suppliers.find(item.supplier_id);
                        if (sup_it == suppliers.end()) continue;

                        const Supplier& sup = sup_it->second;

                        std::optional<Shipment> shipment;
                        auto sh_it = shipment_map.find(item.item_id);
                        if (sh_it != shipment_map.end())
                            shipment = sh_it->second;

                        const int delayed = shipment.has_value()
                            ? shipment->delayed_days : 0;

                        // Check cache before computing
                        if (cache_) {
                            auto cached = cache_->get(
                                item.current_stock, item.daily_demand,
                                item.reorder_point, sup.lead_time_days, delayed);
                            if (cached.has_value()) {
                                results[static_cast<std::size_t>(i)] = *cached;
                                continue;
                            }
                        }

                        PlanningResult r = planner.analyze(item, sup, shipment);

                        if (cache_) {
                            cache_->put(
                                item.current_stock, item.daily_demand,
                                item.reorder_point, sup.lead_time_days, delayed, r);
                        }

                        results[static_cast<std::size_t>(i)] = r;
                    }
                }));
    }

    for (auto& f : futures)
        f.get();

    return results;
}

} // namespace sca
