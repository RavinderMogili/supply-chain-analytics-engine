#include <gtest/gtest.h>
#include "planner/RiskScorer.h"

#include <unordered_map>

using namespace sca;

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace {

Item make_item(int id, const std::string& name, int stock, int demand,
               int reorder_pt, const std::string& sup_id) {
    return Item{id, name, "WH", stock, demand, reorder_pt, 50, sup_id};
}

Supplier make_supplier(const std::string& id, int reliability) {
    return Supplier{id, "Supplier " + id, 7, reliability, "Region"};
}

PlanningResult make_result(RiskLevel level, int days, int ltd) {
    return PlanningResult{days, ltd, false, days, level, "test"};
}

} // anonymous namespace

// ── Tests ─────────────────────────────────────────────────────────────────────

TEST(RiskScorer, HigherRiskItemRankedFirst) {
    RiskScorer scorer;

    std::vector<Item> items = {
        make_item(1, "Safe-Item",  200, 5,  30, "S1"),
        make_item(2, "Risky-Item",  20, 10, 30, "S2"),
    };

    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 95)},
        {"S2", make_supplier("S2", 60)},
    };

    std::vector<PlanningResult> results = {
        make_result(RiskLevel::Low,  40, 35),
        make_result(RiskLevel::High,  2, 70),
    };

    auto entries = scorer.score(items, suppliers, results);
    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].item_name, "Risky-Item");
    EXPECT_EQ(entries[1].item_name, "Safe-Item");
}

TEST(RiskScorer, ScoresAreClampedBetweenZeroAndOne) {
    RiskScorer scorer;

    std::vector<Item> items = {
        make_item(1, "ItemA", 5, 100, 10, "S1"),
    };

    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 0)},  // worst possible reliability
    };

    std::vector<PlanningResult> results = {
        make_result(RiskLevel::High, 0, 700),
    };

    auto entries = scorer.score(items, suppliers, results);
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_GE(entries[0].score, 0.0);
    EXPECT_LE(entries[0].score, 1.0);
}

TEST(RiskScorer, CriticalLabelForHighScore) {
    RiskScorer scorer;

    std::vector<Item> items = {
        make_item(1, "Critical-Item", 1, 50, 50, "S1"),
    };

    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", make_supplier("S1", 10)},  // very unreliable
    };

    std::vector<PlanningResult> results = {
        make_result(RiskLevel::High, 0, 350),
    };

    auto entries = scorer.score(items, suppliers, results);
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].summary, "Critical");
}

TEST(RiskScorer, EmptyInputReturnsEmptyOutput) {
    RiskScorer scorer;
    std::vector<Item> items;
    std::unordered_map<std::string, Supplier> suppliers;
    std::vector<PlanningResult> results;

    auto entries = scorer.score(items, suppliers, results);
    EXPECT_TRUE(entries.empty());
}

TEST(RiskScorer, MissingSupplierDefaultsToMidpointReliability) {
    RiskScorer scorer;

    std::vector<Item> items = {
        make_item(1, "Item", 100, 5, 30, "UNKNOWN"),
    };

    std::unordered_map<std::string, Supplier> suppliers; // empty

    std::vector<PlanningResult> results = {
        make_result(RiskLevel::Low, 20, 35),
    };

    // Should not throw; falls back to reliability=0.5
    EXPECT_NO_THROW(scorer.score(items, suppliers, results));
}
