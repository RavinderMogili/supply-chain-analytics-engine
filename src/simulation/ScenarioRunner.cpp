#include "simulation/ScenarioRunner.h"

#include <future>
#include <algorithm>

namespace sca {

std::vector<ScenarioReport> ScenarioRunner::run_all(
    const std::vector<DisruptionScenario>&            scenarios,
    const std::vector<Item>&                          items,
    const std::unordered_map<std::string, Supplier>&  suppliers,
    const std::unordered_map<int, Shipment>&           shipment_map) const
{
    // Launch one async task per scenario — all run concurrently
    std::vector<std::future<ScenarioReport>> futures;
    futures.reserve(scenarios.size());

    for (const auto& scenario : scenarios) {
        futures.emplace_back(
            std::async(std::launch::async,
                [&items, &suppliers, &shipment_map, scenario]() -> ScenarioReport
                {
                    const auto t0 = std::chrono::high_resolution_clock::now();

                    DisruptionAnalyzer analyzer;
                    auto impacts = analyzer.analyze(
                        items, suppliers, shipment_map, scenario);

                    const auto t1 = std::chrono::high_resolution_clock::now();
                    const double ms =
                        std::chrono::duration<double, std::milli>(t1 - t0).count();

                    int newly_high  = 0;
                    int escalated   = 0;
                    for (const auto& impact : impacts) {
                        if (impact.newly_critical) ++newly_high;
                        if (impact.risk_escalated) ++escalated;
                    }

                    return ScenarioReport{
                        scenario,
                        std::move(impacts),
                        newly_high,
                        escalated,
                        ms
                    };
                }));
    }

    std::vector<ScenarioReport> reports;
    reports.reserve(scenarios.size());
    for (auto& f : futures)
        reports.push_back(f.get());

    return reports;
}

} // namespace sca
