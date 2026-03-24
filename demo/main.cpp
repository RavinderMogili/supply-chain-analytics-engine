#include "loader/CSVLoader.h"
#include "planner/InventoryPlanner.h"
#include "planner/EOQCalculator.h"
#include "planner/SafetyStockCalculator.h"
#include "planner/RiskScorer.h"
#include "algorithms/SMAForecaster.h"
#include "algorithms/EMAForecaster.h"
#include "algorithms/HoltForecaster.h"
#include "algorithms/SignalAwareForecaster.h"
#include "algorithms/DemandSignal.h"
#include "planner/PlannerPipeline.h"
#include "loader/JSONExporter.h"
#include "simulation/SupplyChainSimulator.h"
#include "simulation/DisruptionAnalyzer.h"
#include "simulation/ScenarioRunner.h"
#include "models/PlanningResult.h"

#include <iostream>
#include <iomanip>
#include <unordered_map>
#include <expected>
#include <optional>

using namespace sca;

int main() {
    // ── Load data ─────────────────────────────────────────────────────────────
    const auto items         = CSVLoader::load_items("data/inventory.csv");
    const auto supplier_vec  = CSVLoader::load_suppliers("data/suppliers.csv");
    const auto shipments     = CSVLoader::load_shipments("data/shipments.csv");

    std::unordered_map<std::string, Supplier> supplier_map;
    for (const auto& s : supplier_vec)
        supplier_map[s.supplier_id] = s;

    std::unordered_map<int, Shipment> shipment_map;
    for (const auto& sh : shipments)
        shipment_map[sh.item_id] = sh;

    // ── Inventory Risk Report ─────────────────────────────────────────────────
    std::cout << "\n=== Inventory Risk Report ===\n";
    std::cout << std::left
              << std::setw(12) << "Item"
              << std::setw(8)  << "Stock"
              << std::setw(8)  << "Days"
              << std::setw(12) << "Lead Dem"
              << std::setw(10) << "Risk"
              << "Recommendation\n";
    std::cout << std::string(72, '-') << "\n";

    InventoryPlanner planner;
    std::vector<PlanningResult> results;
    results.reserve(items.size());

    for (const auto& item : items) {
        auto sup_it = supplier_map.find(item.supplier_id);
        if (sup_it == supplier_map.end()) continue;

        std::optional<Shipment> shipment;
        if (auto sh_it = shipment_map.find(item.item_id); sh_it != shipment_map.end())
            shipment = sh_it->second;

        const auto result = planner.analyze(item, sup_it->second, shipment);
        results.push_back(result);

        std::cout << std::left
                  << std::setw(12) << item.item_name
                  << std::setw(8)  << item.current_stock
                  << std::setw(8)  << result.days_until_stockout
                  << std::setw(12) << result.lead_time_demand
                  << std::setw(10) << to_string(result.risk_level)
                  << result.recommendation << "\n";
    }

    // ── EOQ Calculations ──────────────────────────────────────────────────────
    std::cout << "\n=== Economic Order Quantity (EOQ) ===\n";
    std::cout << "Assumptions: ordering cost = $50/order, holding cost = $2/unit/year\n\n";

    EOQCalculator eoq_calc;
    for (const auto& item : items) {
        const double annual_demand = item.daily_demand * 365.0;
        const double eoq_qty       = eoq_calc.calculate(annual_demand, 50.0, 2.0);
        std::cout << std::left << std::setw(12) << item.item_name
                  << "EOQ = " << std::fixed << std::setprecision(1)
                  << eoq_qty << " units"
                  << "  (annual demand = " << static_cast<int>(annual_demand) << ")\n";
    }

    // ── Safety Stock ──────────────────────────────────────────────────────────
    std::cout << "\n=== Safety Stock at 95% Service Level ===\n";
    std::cout << "Assumptions: sigma_demand = 3 units/day, sigma_lead_time = 1.5 days\n\n";

    SafetyStockCalculator ss_calc;
    for (const auto& item : items) {
        auto sup_it = supplier_map.find(item.supplier_id);
        if (sup_it == supplier_map.end()) continue;

        const double safety = ss_calc.calculate(
            0.95,
            static_cast<double>(sup_it->second.lead_time_days),
            3.0,
            static_cast<double>(item.daily_demand),
            1.5);

        std::cout << std::left << std::setw(12) << item.item_name
                  << "Safety stock = " << std::fixed << std::setprecision(1)
                  << safety << " units\n";
    }

    // ── Demand Forecasting ────────────────────────────────────────────────────
    std::cout << "\n=== Demand Forecast (next 3 periods) ===\n";
    std::cout << "Using sample demand history: 80 85 90 88 92 95 91 89\n\n";

    const std::vector<double> demand_history = {80, 85, 90, 88, 92, 95, 91, 89};

    SMAForecaster  sma(3);
    EMAForecaster  ema(0.3);
    HoltForecaster holt(0.3, 0.2);

    auto print_forecast = [](const std::string& label,
                              const std::vector<double>& forecast) {
        std::cout << std::setw(36) << std::left << label;
        for (double v : forecast)
            std::cout << std::fixed << std::setprecision(1) << v << "  ";
        std::cout << "\n";
    };

    print_forecast(sma.name()  + ":", sma.forecast(demand_history, 3));
    print_forecast(ema.name()  + ":", ema.forecast(demand_history, 3));
    print_forecast(holt.name() + ":", holt.forecast(demand_history, 3));
    std::cout << "  (Holt extrapolates the upward trend — higher than SMA/EMA flat projections)\n";

    // ── Portfolio Risk Ranking ────────────────────────────────────────────────
    std::cout << "\n=== Portfolio Risk Ranking (highest risk first) ===\n";
    std::cout << std::left
              << std::setw(14) << "Item"
              << std::setw(10) << "Score"
              << "Status\n";
    std::cout << std::string(34, '-') << "\n";

    RiskScorer scorer;
    const auto risk_entries = scorer.score(items, supplier_map, results);

    for (const auto& entry : risk_entries) {
        std::cout << std::left
                  << std::setw(14) << entry.item_name
                  << std::setw(10) << std::fixed << std::setprecision(3) << entry.score
                  << entry.summary << "\n";
    }

    // ── PlannerPipeline demo (std::expected C++23 typed error handling) ───────
    std::cout << "\n=== PlannerPipeline (std::expected<T,E> result/error — C++23) ===\n";

    PlannerPipeline pipeline;
    const Item bad_item{99, "Ghost-Item", "WH", 50, 5, 20, 40, "S_UNKNOWN"};

    for (const auto* item_ptr : {&items[0], &bad_item}) {
        auto result = pipeline.run(*item_ptr, supplier_map, shipment_map);
        if (result.has_value())
            std::cout << "  " << std::left << std::setw(12) << item_ptr->item_name
                      << "-> " << to_string(result.value().risk_level)
                      << " | " << result.value().recommendation << "\n";
        else
            std::cout << "  " << std::left << std::setw(12) << item_ptr->item_name
                      << "-> ERROR: " << result.error() << "\n";
    }

    // ── Export JSON report ────────────────────────────────────────────────────
    const std::string report_path = "report.json";
    JSONExporter::export_report(items, results, risk_entries, report_path);
    std::cout << "\nReport exported to: " << report_path << "\n";

    // ── Digital Twin: 30-day simulation ──────────────────────────────────────
    std::cout << "\n=== Digital Twin: 30-Day Inventory Simulation ===\n";
    std::cout << std::left
              << std::setw(14) << "Item"
              << std::setw(14) << "Stockout Day"
              << std::setw(12) << "Reorders"
              << "Status\n";
    std::cout << std::string(54, '-') << "\n";

    SupplyChainSimulator simulator(30);
    const auto sim_results = simulator.simulate(items, supplier_map);

    for (const auto& sr : sim_results) {
        const std::string stockout_str =
            sr.first_stockout_day == -1
            ? "Never"
            : "Day " + std::to_string(sr.first_stockout_day);

        const std::string status =
            sr.first_stockout_day == -1
            ? "Safe through horizon"
            : "STOCKOUT projected on day " + std::to_string(sr.first_stockout_day);

        std::cout << std::left
                  << std::setw(14) << sr.item_name
                  << std::setw(14) << stockout_str
                  << std::setw(12) << sr.reorder_count
                  << status << "\n";
    }

    // ── Disruption Scenario: What if S1 is delayed 14 days? ──────────────────
    std::cout << "\n=== Disruption Scenario: Supplier S1 delayed +14 days ===\n";
    std::cout << "(Simulates a port strike or logistics disruption)\n\n";
    std::cout << std::left
              << std::setw(14) << "Item"
              << std::setw(14) << "Risk Before"
              << std::setw(14) << "Risk After"
              << std::setw(12) << "Days Before"
              << std::setw(12) << "Days After"
              << "Alert\n";
    std::cout << std::string(74, '-') << "\n";

    DisruptionAnalyzer disruption_analyzer;
    const DisruptionScenario scenario{"S1", 14, "Port strike: +14 day delay"};
    const auto impacts = disruption_analyzer.analyze(
        items, supplier_map, shipment_map, scenario);

    if (impacts.empty()) {
        std::cout << "  No items sourced from S1 in this dataset.\n";
    } else {
        for (const auto& impact : impacts) {
            const std::string alert =
                impact.newly_critical  ? "*** NEWLY HIGH RISK ***" :
                impact.risk_escalated  ? "^ Risk escalated"       : "Unchanged";

            std::cout << std::left
                      << std::setw(14) << impact.item_name
                      << std::setw(14) << to_string(impact.risk_before)
                      << std::setw(14) << to_string(impact.risk_after)
                      << std::setw(12) << impact.days_until_stockout_before
                      << std::setw(12) << impact.days_until_stockout_after
                      << alert << "\n";
        }
    }

    // ── Parallel Scenario Runner (agility & resilience) ───────────────────────
    std::cout << "\n=== Parallel Scenario Runner: 4 disruptions evaluated concurrently ===\n";
    std::cout << "(Each scenario runs in its own std::async task)\n\n";

    const std::vector<DisruptionScenario> scenarios = {
        {"S1",  7, "S1: minor delay +7d (weather)"},
        {"S1", 14, "S1: moderate delay +14d (port strike)"},
        {"S2", 10, "S2: delay +10d (customs hold)"},
        {"S3", 21, "S3: severe delay +21d (factory shutdown)"},
    };

    ScenarioRunner runner;
    const auto reports = runner.run_all(scenarios, items, supplier_map, shipment_map);

    std::cout << std::left
              << std::setw(38) << "Scenario"
              << std::setw(12) << "Escalated"
              << std::setw(16) << "Newly High Risk"
              << "Time\n";
    std::cout << std::string(72, '-') << "\n";

    for (const auto& report : reports) {
        std::cout << std::left
                  << std::setw(38) << report.scenario.description
                  << std::setw(12) << report.escalated_count
                  << std::setw(16) << report.newly_high_risk_count
                  << std::fixed << std::setprecision(2)
                  << report.elapsed_ms << " ms\n";
    }
    std::cout << "\n  All " << reports.size()
              << " scenarios evaluated in parallel — planners see all impacts at once.\n";

    // ── Real-time Demand Signal Injection ────────────────────────────────────
    std::cout << "\n=== Real-Time Demand Signal Injection ===\n";
    std::cout << "Static forecasts augmented with live market signals\n";
    std::cout << "(POS spike, competitor stockout, promotional uplift)\n\n";

    const std::vector<double> signal_history = {80, 85, 90, 88, 92, 95, 91, 89};

    // Baseline Holt forecast (no signals)
    SignalAwareForecaster signal_forecaster(
        std::make_unique<HoltForecaster>(0.3, 0.2));

    auto baseline = signal_forecaster.forecast(signal_history, 3);

    // Inject real-time signals
    signal_forecaster.add_signal({"POS",
        +15.0, DemandSignal::Type::Additive,
        "Weekend POS spike: +15 units/period"});
    signal_forecaster.add_signal({"MARKET",
        1.10, DemandSignal::Type::Multiplicative,
        "Competitor out-of-stock: demand up 10%"});
    signal_forecaster.add_signal({"PROMO",
        +8.0, DemandSignal::Type::Additive,
        "Promotional campaign: +8 units/period"});

    auto adjusted = signal_forecaster.forecast(signal_history, 3);

    std::cout << std::left << std::setw(36) << "Forecast"
              << "Period 1    Period 2    Period 3\n";
    std::cout << std::string(63, '-') << "\n";

    auto print_row = [](const std::string& label, const std::vector<double>& v) {
        std::cout << std::left << std::setw(36) << label;
        for (double x : v)
            std::cout << std::fixed << std::setprecision(1)
                      << std::setw(12) << x;
        std::cout << "\n";
    };

    print_row("Holt baseline (no signals):", baseline);
    print_row(signal_forecaster.name() + ":", adjusted);

    std::cout << "\nSignals applied:\n";
    for (const auto& sig : signal_forecaster.signals())
        std::cout << "  [" << sig.source << "] " << sig.description << "\n";

    std::cout << "\n";
    return 0;
}
