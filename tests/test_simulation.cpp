#include <gtest/gtest.h>
#include "simulation/SupplyChainSimulator.h"
#include "simulation/DisruptionAnalyzer.h"
#include "simulation/ScenarioRunner.h"

#include <unordered_map>

using namespace sca;

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace {

Supplier make_supplier(const std::string& id, int lead_days, int reliability = 90) {
    return Supplier{id, "TestCo", lead_days, reliability, "ON"};
}

Item make_item(int id, const std::string& sup, int stock, int demand,
               int reorder_point, int reorder_qty) {
    return Item{id, "Item-" + std::to_string(id), "WH",
                stock, demand, reorder_point, reorder_qty, sup};
}

} // anonymous namespace

// ── SupplyChainSimulator ──────────────────────────────────────────────────────

TEST(SupplyChainSimulator, EmptyItemsReturnsEmpty) {
    SupplyChainSimulator sim(30);
    std::unordered_map<std::string, Supplier> suppliers;
    EXPECT_TRUE(sim.simulate({}, suppliers).empty());
}

TEST(SupplyChainSimulator, UnknownSupplierSkipsItem) {
    SupplyChainSimulator sim(10);
    std::unordered_map<std::string, Supplier> suppliers; // empty
    auto items = std::vector<Item>{make_item(1, "S1", 100, 10, 30, 50)};
    EXPECT_TRUE(sim.simulate(items, suppliers).empty());
}

TEST(SupplyChainSimulator, TimelineHasCorrectLength) {
    SupplyChainSimulator sim(15);
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 5)}
    };
    auto items = std::vector<Item>{make_item(1, "S1", 200, 5, 20, 50)};
    auto results = sim.simulate(items, suppliers);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(static_cast<int>(results[0].timeline.size()), 15);
}

TEST(SupplyChainSimulator, HighStockItemDoesNotStockout) {
    SupplyChainSimulator sim(30);
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 7)}
    };
    // 500 stock, 5/day demand → 100 days supply — won't stockout in 30 days
    auto items = std::vector<Item>{make_item(1, "S1", 500, 5, 20, 100)};
    auto results = sim.simulate(items, suppliers);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].first_stockout_day, -1);
}

TEST(SupplyChainSimulator, LowStockItemStocksOutQuickly) {
    SupplyChainSimulator sim(30);
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 30)} // long lead time — won't arrive in time
    };
    // 5 stock, 5/day demand → stockout on day 1 (stock hits 0 after demand)
    auto items = std::vector<Item>{make_item(1, "S1", 5, 5, 2, 50)};
    auto results = sim.simulate(items, suppliers);

    ASSERT_EQ(results.size(), 1u);
    // Stock = 5, demand = 5/day: after day 1 stock = 0, after day 2 stock = -5 → stockout day 2
    EXPECT_GT(results[0].first_stockout_day, 0);
    EXPECT_LE(results[0].first_stockout_day, 5);
}

TEST(SupplyChainSimulator, ReorderIsTriggeredAtReorderPoint) {
    SupplyChainSimulator sim(30);
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 5)}
    };
    // Start with 100, demand 10/day, reorder at 50 → reorder triggers on day 5
    auto items = std::vector<Item>{make_item(1, "S1", 100, 10, 50, 200)};
    auto results = sim.simulate(items, suppliers);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_GE(results[0].reorder_count, 1);
}

TEST(SupplyChainSimulator, ReorderArrivalPreventsStockout) {
    SupplyChainSimulator sim(30);
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 3)} // fast supplier
    };
    // High reorder qty so replenishment fully covers demand
    auto items = std::vector<Item>{make_item(1, "S1", 50, 5, 30, 500)};
    auto results = sim.simulate(items, suppliers);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].first_stockout_day, -1); // should never stockout
}

TEST(SupplyChainSimulator, DaySnapshotDayNumbersAreCorrect) {
    SupplyChainSimulator sim(5);
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 7)}
    };
    auto items = std::vector<Item>{make_item(1, "S1", 500, 1, 10, 50)};
    auto results = sim.simulate(items, suppliers);

    ASSERT_EQ(results.size(), 1u);
    for (int i = 0; i < 5; ++i)
        EXPECT_EQ(results[0].timeline[static_cast<std::size_t>(i)].day, i + 1);
}

TEST(SupplyChainSimulator, StockDecreasesByDailyDemandEachDay) {
    SupplyChainSimulator sim(3);
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 30)} // long lead time - no reorder arrives
    };
    // High reorder point = 999 so reorder triggers day 1, but won't arrive
    // Start stock = 1000, demand = 100/day, reorder_point = 999
    auto items = std::vector<Item>{make_item(1, "S1", 1000, 100, 100, 50)};
    auto results = sim.simulate(items, suppliers);

    ASSERT_EQ(results.size(), 1u);
    // Day 1: stock = 1000 - 100 = 900, Day 2 = 800, Day 3 = 700
    EXPECT_EQ(results[0].timeline[0].stock, 900);
    EXPECT_EQ(results[0].timeline[1].stock, 800);
    EXPECT_EQ(results[0].timeline[2].stock, 700);
}

// ── DisruptionAnalyzer ────────────────────────────────────────────────────────

TEST(DisruptionAnalyzer, OnlyAffectedSupplierItemsIncluded) {
    const std::vector<Item> items = {
        make_item(1, "S1", 100, 10, 30, 50),
        make_item(2, "S2", 100, 10, 30, 50), // different supplier
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 7)},
        {"S2", make_supplier("S2", 7)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    DisruptionAnalyzer analyzer;
    DisruptionScenario scenario{"S1", 14, "Port strike adds 14 days"};

    auto impacts = analyzer.analyze(items, suppliers, no_shipments, scenario);

    // Only item 1 (S1) should be in the results
    ASSERT_EQ(impacts.size(), 1u);
    EXPECT_EQ(impacts[0].item_name, "Item-1");
}

TEST(DisruptionAnalyzer, DisruptionReducesDaysUntilStockout) {
    const std::vector<Item> items = {
        make_item(1, "S1", 60, 10, 30, 50), // tight stock
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 5)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    DisruptionAnalyzer analyzer;
    DisruptionScenario scenario{"S1", 10, "10 extra days delay"};

    auto impacts = analyzer.analyze(items, suppliers, no_shipments, scenario);

    ASSERT_EQ(impacts.size(), 1u);
    EXPECT_LE(impacts[0].days_until_stockout_after,
              impacts[0].days_until_stockout_before);
}

TEST(DisruptionAnalyzer, NewlyCriticalFlagSetCorrectly) {
    // Item with just enough stock under normal conditions, but goes critical with delay
    const std::vector<Item> items = {
        make_item(1, "S1", 80, 10, 30, 50),
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 5)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    DisruptionAnalyzer analyzer;
    DisruptionScenario scenario{"S1", 30, "Extreme 30-day disruption"};

    auto impacts = analyzer.analyze(items, suppliers, no_shipments, scenario);

    ASSERT_EQ(impacts.size(), 1u);
    // newly_critical is true only if it escalated TO High (the worst level)
    if (impacts[0].risk_after == RiskLevel::High)
        EXPECT_TRUE(impacts[0].newly_critical ||
                    impacts[0].risk_before == RiskLevel::High);
}

TEST(DisruptionAnalyzer, UnknownSupplierScenarioProducesNoImpacts) {
    const std::vector<Item> items = {
        make_item(1, "S1", 100, 10, 30, 50),
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 7)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    DisruptionAnalyzer analyzer;
    DisruptionScenario scenario{"S_UNKNOWN", 14, "Nonexistent supplier"};

    auto impacts = analyzer.analyze(items, suppliers, no_shipments, scenario);
    EXPECT_TRUE(impacts.empty());
}

TEST(DisruptionAnalyzer, ZeroDelayProducesNoRiskEscalation) {
    const std::vector<Item> items = {
        make_item(1, "S1", 100, 10, 30, 50),
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 7)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    DisruptionAnalyzer analyzer;
    DisruptionScenario scenario{"S1", 0, "No disruption"};

    auto impacts = analyzer.analyze(items, suppliers, no_shipments, scenario);

    ASSERT_EQ(impacts.size(), 1u);
    EXPECT_FALSE(impacts[0].risk_escalated);
    EXPECT_FALSE(impacts[0].newly_critical);
    EXPECT_EQ(impacts[0].risk_before, impacts[0].risk_after);
}

// ── ScenarioRunner ────────────────────────────────────────────────────────────

TEST(ScenarioRunner, EmptyScenariosReturnsEmpty) {
    ScenarioRunner runner;
    std::unordered_map<std::string, Supplier> suppliers;
    std::unordered_map<int, Shipment> no_shipments;
    auto reports = runner.run_all({}, {}, suppliers, no_shipments);
    EXPECT_TRUE(reports.empty());
}

TEST(ScenarioRunner, ReportCountMatchesScenarioCount) {
    const std::vector<Item> items = {
        make_item(1, "S1", 60, 10, 30, 50),
        make_item(2, "S2", 60, 10, 30, 50),
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 5)},
        {"S2", make_supplier("S2", 7)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    const std::vector<DisruptionScenario> scenarios = {
        {"S1",  7, "S1 +7d"},
        {"S1", 14, "S1 +14d"},
        {"S2", 10, "S2 +10d"},
    };

    ScenarioRunner runner;
    auto reports = runner.run_all(scenarios, items, suppliers, no_shipments);

    ASSERT_EQ(reports.size(), 3u);
}

TEST(ScenarioRunner, ReportOrderMatchesScenarioInputOrder) {
    const std::vector<Item> items = {
        make_item(1, "S1", 100, 5, 30, 50),
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 5)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    const std::vector<DisruptionScenario> scenarios = {
        {"S1",  3, "alpha"},
        {"S1", 14, "beta"},
        {"S1", 30, "gamma"},
    };

    ScenarioRunner runner;
    auto reports = runner.run_all(scenarios, items, suppliers, no_shipments);

    ASSERT_EQ(reports.size(), 3u);
    EXPECT_EQ(reports[0].scenario.description, "alpha");
    EXPECT_EQ(reports[1].scenario.description, "beta");
    EXPECT_EQ(reports[2].scenario.description, "gamma");
}

TEST(ScenarioRunner, LargerDelayProducesMoreOrEqualEscalations) {
    const std::vector<Item> items = {
        make_item(1, "S1", 60, 10, 30, 50),
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 5)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    const std::vector<DisruptionScenario> scenarios = {
        {"S1",  1, "tiny delay"},
        {"S1", 30, "huge delay"},
    };

    ScenarioRunner runner;
    auto reports = runner.run_all(scenarios, items, suppliers, no_shipments);

    ASSERT_EQ(reports.size(), 2u);
    // Larger delay cannot produce fewer escalations than a smaller delay
    EXPECT_GE(reports[1].escalated_count, reports[0].escalated_count);
}

TEST(ScenarioRunner, ElapsedTimeIsNonNegative) {
    const std::vector<Item> items = {
        make_item(1, "S1", 100, 5, 30, 50),
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 7)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    ScenarioRunner runner;
    auto reports = runner.run_all({{"S1", 10, "test"}}, items, suppliers, no_shipments);

    ASSERT_EQ(reports.size(), 1u);
    EXPECT_GE(reports[0].elapsed_ms, 0.0);
}

TEST(ScenarioRunner, CountersAreConsistentWithImpacts) {
    const std::vector<Item> items = {
        make_item(1, "S1", 50, 10, 30, 50),
        make_item(2, "S1", 50, 10, 30, 50),
    };
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 5)},
    };
    std::unordered_map<int, Shipment> no_shipments;

    ScenarioRunner runner;
    auto reports = runner.run_all({{"S1", 20, "big delay"}}, items, suppliers, no_shipments);

    ASSERT_EQ(reports.size(), 1u);
    const auto& report = reports[0];

    // Recount manually and verify
    int manual_newly_high = 0;
    int manual_escalated  = 0;
    for (const auto& impact : report.impacts) {
        if (impact.newly_critical) ++manual_newly_high;
        if (impact.risk_escalated) ++manual_escalated;
    }
    EXPECT_EQ(report.newly_high_risk_count, manual_newly_high);
    EXPECT_EQ(report.escalated_count,       manual_escalated);
}
