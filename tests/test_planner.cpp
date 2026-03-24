#include <gtest/gtest.h>
#include "planner/InventoryPlanner.h"

using namespace sca;

// ── Helper factories ──────────────────────────────────────────────────────────

namespace {

Item make_item(int stock, int demand, int reorder_pt, int reorder_qty = 50) {
    return Item{1, "TestItem", "WH-Test", stock, demand, reorder_pt, reorder_qty, "S1"};
}

Supplier make_supplier(int lead_time, int reliability = 90) {
    return Supplier{"S1", "Test Supplier", lead_time, reliability, "Ontario"};
}

Shipment make_shipment(int delayed_days, int item_id = 1) {
    return Shipment{"SH001", item_id, "S1", "2026-04-01", "Delayed", delayed_days, "Test delay"};
}

} // anonymous namespace

// ── NoDemand ──────────────────────────────────────────────────────────────────

TEST(InventoryPlanner, ZeroDemandReturnsNoDemandRisk) {
    InventoryPlanner planner;
    auto result = planner.analyze(make_item(100, 0, 30), make_supplier(7), std::nullopt);
    EXPECT_EQ(result.risk_level,        RiskLevel::NoDemand);
    EXPECT_EQ(result.days_until_stockout, -1);
    EXPECT_EQ(result.lead_time_demand,    0);
    EXPECT_FALSE(result.reorder_now);
}

TEST(InventoryPlanner, NegativeDemandAlsoReturnsNoDemand) {
    InventoryPlanner planner;
    auto result = planner.analyze(make_item(100, -5, 30), make_supplier(7), std::nullopt);
    EXPECT_EQ(result.risk_level, RiskLevel::NoDemand);
}

// ── High Risk ─────────────────────────────────────────────────────────────────

TEST(InventoryPlanner, HighRiskWhenStockoutBeforeReplenishment) {
    // stock=30, demand=10 → 3 days left; lead_time=7 → stockout before arrival
    InventoryPlanner planner;
    auto result = planner.analyze(make_item(30, 10, 20), make_supplier(7), std::nullopt);
    EXPECT_EQ(result.risk_level,         RiskLevel::High);
    EXPECT_EQ(result.days_until_stockout, 3);
    EXPECT_EQ(result.lead_time_demand,   70);  // 10 * 7
}

TEST(InventoryPlanner, ShipmentDelayEscalatesRiskFromLowToHigh) {
    // Without delay: stock=60, demand=5 → 12 days; lead_time=7 → Low
    // With 6 days delay: effective_lead_time=13 > 12 days → High
    InventoryPlanner planner;
    auto item     = make_item(60, 5, 30);
    auto supplier = make_supplier(7);

    auto result_no_delay = planner.analyze(item, supplier, std::nullopt);
    EXPECT_EQ(result_no_delay.risk_level, RiskLevel::Low);

    auto result_delayed = planner.analyze(item, supplier, make_shipment(6));
    EXPECT_EQ(result_delayed.risk_level, RiskLevel::High);
}

// ── Medium Risk ───────────────────────────────────────────────────────────────

TEST(InventoryPlanner, MediumRiskExactlyAtReorderPoint) {
    // stock=20=reorder_point, demand=5, lead_time=3 → days=4 > 3, but reorder_now=true
    InventoryPlanner planner;
    auto result = planner.analyze(make_item(20, 5, 20), make_supplier(3), std::nullopt);
    EXPECT_EQ(result.risk_level, RiskLevel::Medium);
    EXPECT_TRUE(result.reorder_now);
    EXPECT_EQ(result.reorder_in_days, 0);
}

TEST(InventoryPlanner, MediumRiskBelowReorderPoint) {
    // stock=15 < reorder_point=20, not yet high risk
    InventoryPlanner planner;
    auto result = planner.analyze(make_item(15, 5, 20), make_supplier(2), std::nullopt);
    EXPECT_TRUE(result.reorder_now);
}

// ── Low Risk ──────────────────────────────────────────────────────────────────

TEST(InventoryPlanner, LowRiskWhenStockWellAboveReorderPoint) {
    // stock=200, demand=10, lead_time=7 → 20 days remaining; above reorder_pt=50
    InventoryPlanner planner;
    auto result = planner.analyze(make_item(200, 10, 50), make_supplier(7), std::nullopt);
    EXPECT_EQ(result.risk_level, RiskLevel::Low);
    EXPECT_FALSE(result.reorder_now);
    EXPECT_EQ(result.reorder_in_days, 15); // (200-50)/10
}

// ── Derived metrics ───────────────────────────────────────────────────────────

TEST(InventoryPlanner, LeadTimeDemandUsesBaseLeadTimeWhenNoShipment) {
    InventoryPlanner planner;
    auto result = planner.analyze(make_item(100, 10, 30), make_supplier(5), std::nullopt);
    EXPECT_EQ(result.lead_time_demand, 50);  // 10 * 5
}

TEST(InventoryPlanner, LeadTimeDemandIncludesShipmentDelay) {
    InventoryPlanner planner;
    // lead_time=5 + delay=3 = 8 effective days; demand=10 → 80
    auto result = planner.analyze(
        make_item(100, 10, 30), make_supplier(5), make_shipment(3));
    EXPECT_EQ(result.lead_time_demand, 80);
}

TEST(InventoryPlanner, DaysUntilStockoutUsesIntegerDivision) {
    InventoryPlanner planner;
    // stock=35, demand=10 → 3 full days (35/10 = 3)
    auto result = planner.analyze(make_item(35, 10, 5), make_supplier(1), std::nullopt);
    EXPECT_EQ(result.days_until_stockout, 3);
}

TEST(InventoryPlanner, ReorderInDaysZeroWhenAtReorderPoint) {
    InventoryPlanner planner;
    auto result = planner.analyze(make_item(20, 5, 20), make_supplier(3), std::nullopt);
    EXPECT_EQ(result.reorder_in_days, 0);
}
