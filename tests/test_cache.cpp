#include <gtest/gtest.h>
#include "planner/PlannerCache.h"
#include "planner/BatchPlanner.h"
#include "planner/InventoryPlanner.h"

#include <thread>
#include <vector>
#include <unordered_map>

using namespace sca;

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace {

PlanningResult dummy_result(RiskLevel level = RiskLevel::Low) {
    return PlanningResult{10, 20, false, 5, level, "test"};
}

} // anonymous namespace

// ── PlannerCache — basic hit/miss ─────────────────────────────────────────────

TEST(PlannerCache, MissOnEmptyCache) {
    PlannerCache cache(16);
    auto result = cache.get(100, 10, 30, 7, 0);
    EXPECT_FALSE(result.has_value());
    EXPECT_EQ(cache.misses(), 1u);
    EXPECT_EQ(cache.hits(),   0u);
}

TEST(PlannerCache, HitAfterPut) {
    PlannerCache cache(16);
    auto r = dummy_result(RiskLevel::High);
    cache.put(50, 5, 20, 7, 0, r);

    auto found = cache.get(50, 5, 20, 7, 0);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->risk_level, RiskLevel::High);
    EXPECT_EQ(cache.hits(), 1u);
}

TEST(PlannerCache, DifferentKeysMiss) {
    PlannerCache cache(16);
    cache.put(50, 5, 20, 7, 0, dummy_result());

    EXPECT_FALSE(cache.get(50, 5, 20, 7, 1).has_value()); // delayed_days differs
    EXPECT_FALSE(cache.get(51, 5, 20, 7, 0).has_value()); // stock differs
}

TEST(PlannerCache, UpdateExistingKey) {
    PlannerCache cache(16);
    cache.put(100, 10, 30, 7, 0, dummy_result(RiskLevel::Low));
    cache.put(100, 10, 30, 7, 0, dummy_result(RiskLevel::High)); // overwrite

    auto found = cache.get(100, 10, 30, 7, 0);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->risk_level, RiskLevel::High);
}

TEST(PlannerCache, SizeTracking) {
    PlannerCache cache(16);
    EXPECT_EQ(cache.size(), 0u);
    cache.put(1, 1, 1, 1, 0, dummy_result());
    EXPECT_EQ(cache.size(), 1u);
    cache.put(2, 2, 2, 2, 0, dummy_result());
    EXPECT_EQ(cache.size(), 2u);
}

// ── LRU eviction ──────────────────────────────────────────────────────────────

TEST(PlannerCache, EvictsLRUWhenFull) {
    PlannerCache cache(2); // capacity = 2

    cache.put(1, 1, 1, 1, 0, dummy_result()); // A — LRU after B and C inserted
    cache.put(2, 2, 2, 2, 0, dummy_result()); // B
    cache.put(3, 3, 3, 3, 0, dummy_result()); // C — A should be evicted

    EXPECT_EQ(cache.size(), 2u);
    EXPECT_FALSE(cache.get(1, 1, 1, 1, 0).has_value()); // A evicted
    EXPECT_TRUE (cache.get(2, 2, 2, 2, 0).has_value()); // B still present
    EXPECT_TRUE (cache.get(3, 3, 3, 3, 0).has_value()); // C still present
}

TEST(PlannerCache, AccessPromotesEntryAndPreventsEviction) {
    PlannerCache cache(2);

    cache.put(1, 1, 1, 1, 0, dummy_result()); // A
    cache.put(2, 2, 2, 2, 0, dummy_result()); // B  (A is now LRU)
    cache.get(1, 1, 1, 1, 0);                 // access A → A becomes MRU, B becomes LRU
    cache.put(3, 3, 3, 3, 0, dummy_result()); // C → B should be evicted

    EXPECT_TRUE (cache.get(1, 1, 1, 1, 0).has_value()); // A promoted, still here
    EXPECT_FALSE(cache.get(2, 2, 2, 2, 0).has_value()); // B evicted
    EXPECT_TRUE (cache.get(3, 3, 3, 3, 0).has_value()); // C present
}

// ── Hit rate ─────────────────────────────────────────────────────────────────

TEST(PlannerCache, HitRateZeroOnEmptyCache) {
    PlannerCache cache(16);
    EXPECT_DOUBLE_EQ(cache.hit_rate(), 0.0);
}

TEST(PlannerCache, HitRateCalculation) {
    PlannerCache cache(16);
    cache.put(1, 1, 1, 1, 0, dummy_result());
    cache.get(1, 1, 1, 1, 0); // hit
    cache.get(9, 9, 9, 9, 0); // miss

    EXPECT_NEAR(cache.hit_rate(), 0.5, 1e-9); // 1 hit / 2 total
}

// ── Thread safety ─────────────────────────────────────────────────────────────

TEST(PlannerCache, ConcurrentPutsDoNotCorruptCache) {
    PlannerCache cache(1024);
    constexpr int N_THREADS = 8;
    constexpr int N_PER     = 64;

    std::vector<std::thread> threads;
    threads.reserve(N_THREADS);

    for (int t = 0; t < N_THREADS; ++t) {
        threads.emplace_back([&cache, t, N_PER]() {
            for (int i = 0; i < N_PER; ++i) {
                const int key = t * N_PER + i;
                cache.put(key, key, key, key % 15 + 1, 0, dummy_result());
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_LE(cache.size(), 1024u);
    EXPECT_GE(cache.size(), 1u);
}

// ── BatchPlanner ──────────────────────────────────────────────────────────────

TEST(BatchPlanner, ResultsMatchSingleItemPlanner) {
    const std::vector<Item> items = {
        {1, "A", "WH", 100, 10, 40, 50, "S1"},
        {2, "B", "WH",  20,  8, 30, 40, "S1"},
        {3, "C", "WH", 200, 25, 80, 100, "S1"},
    };
    const std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", Supplier{"S1", "TestCo", 7, 90, "ON"}},
    };
    const std::unordered_map<int, Shipment> no_shipments;

    BatchPlanner  batch(2, nullptr); // batch_size=2 → two batches
    InventoryPlanner single;

    auto batch_results = batch.analyze_all(items, suppliers, no_shipments);

    for (std::size_t i = 0; i < items.size(); ++i) {
        auto expected = single.analyze(items[i], suppliers.at("S1"), std::nullopt);
        EXPECT_EQ(batch_results[i].risk_level,         expected.risk_level);
        EXPECT_EQ(batch_results[i].days_until_stockout, expected.days_until_stockout);
        EXPECT_EQ(batch_results[i].lead_time_demand,    expected.lead_time_demand);
    }
}

TEST(BatchPlanner, EmptyInputReturnsEmptyOutput) {
    BatchPlanner planner(64, nullptr);
    std::unordered_map<std::string, Supplier> suppliers;
    std::unordered_map<int, Shipment> no_shipments;
    auto results = planner.analyze_all({}, suppliers, no_shipments);
    EXPECT_TRUE(results.empty());
}

TEST(BatchPlanner, CacheIsPopulatedAfterFirstRun) {
    const std::vector<Item> items = {
        {1, "A", "WH", 100, 10, 40, 50, "S1"},
        {2, "B", "WH",  20,  8, 30, 40, "S1"},
    };
    const std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", Supplier{"S1", "TestCo", 7, 90, "ON"}},
    };
    const std::unordered_map<int, Shipment> no_shipments;

    PlannerCache cache(64);
    BatchPlanner planner(8, &cache);

    planner.analyze_all(items, suppliers, no_shipments);
    EXPECT_EQ(cache.misses(), 2u); // both items computed fresh
    EXPECT_EQ(cache.hits(),   0u);

    planner.analyze_all(items, suppliers, no_shipments);
    EXPECT_EQ(cache.hits(), 2u);   // both served from cache
}
