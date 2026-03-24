#include <gtest/gtest.h>
#include "algorithms/HoltForecaster.h"
#include "planner/PlannerPipeline.h"

#include <unordered_map>
#include <cmath>

using namespace sca;

// ── HoltForecaster ────────────────────────────────────────────────────────────

TEST(HoltForecaster, InvalidAlphaThrows) {
    EXPECT_THROW(HoltForecaster(0.0, 0.1),  std::invalid_argument);
    EXPECT_THROW(HoltForecaster(1.0, 0.1),  std::invalid_argument);
    EXPECT_THROW(HoltForecaster(-0.1, 0.1), std::invalid_argument);
}

TEST(HoltForecaster, InvalidBetaThrows) {
    EXPECT_THROW(HoltForecaster(0.3, 0.0),  std::invalid_argument);
    EXPECT_THROW(HoltForecaster(0.3, 1.0),  std::invalid_argument);
    EXPECT_THROW(HoltForecaster(0.3, 1.5),  std::invalid_argument);
}

TEST(HoltForecaster, EmptyHistoryReturnsEmpty) {
    HoltForecaster holt(0.3, 0.1);
    EXPECT_TRUE(holt.forecast({}, 3).empty());
}

TEST(HoltForecaster, ZeroPeriodsReturnsEmpty) {
    HoltForecaster holt(0.3, 0.1);
    EXPECT_TRUE(holt.forecast({10.0, 20.0, 30.0}, 0).empty());
}

TEST(HoltForecaster, SingleElementHistoryReturnsFlatProjection) {
    HoltForecaster holt(0.3, 0.1);
    auto result = holt.forecast({42.0}, 3);
    ASSERT_EQ(result.size(), 3u);
    for (double v : result) EXPECT_DOUBLE_EQ(v, 42.0);
}

TEST(HoltForecaster, CorrectNumberOfPeriodReturned) {
    HoltForecaster holt(0.3, 0.1);
    auto result = holt.forecast({10.0, 20.0, 30.0}, 5);
    EXPECT_EQ(result.size(), 5u);
}

TEST(HoltForecaster, UpwardTrendProducesIncreasingForecast) {
    // Strongly increasing history → each future period should be larger than the last
    HoltForecaster holt(0.5, 0.5);
    auto result = holt.forecast({10.0, 20.0, 30.0, 40.0, 50.0}, 3);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_LT(result[0], result[1]);
    EXPECT_LT(result[1], result[2]);
}

TEST(HoltForecaster, DownwardTrendProducesDecreasingForecast) {
    HoltForecaster holt(0.5, 0.5);
    auto result = holt.forecast({50.0, 40.0, 30.0, 20.0, 10.0}, 3);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_GT(result[0], result[1]);
    EXPECT_GT(result[1], result[2]);
}

TEST(HoltForecaster, FlatHistoryProducesNearFlatForecast) {
    // Constant demand → trend should converge near zero
    HoltForecaster holt(0.3, 0.1);
    auto result = holt.forecast({50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0, 50.0}, 3);
    ASSERT_EQ(result.size(), 3u);
    for (double v : result)
        EXPECT_NEAR(v, 50.0, 5.0); // within 10% of actual flat demand
}

TEST(HoltForecaster, TrendForecastDiffersFromEMAFlatProjection) {
    // For a rising trend, Holt should forecast HIGHER than a flat EMA projection
    HoltForecaster holt(0.3, 0.3);
    auto rising_history = std::vector<double>{70, 75, 80, 85, 90, 95};
    auto result = holt.forecast(rising_history, 3);
    // All Holt forecasts should be above 95 (the last observation)
    for (double v : result)
        EXPECT_GT(v, 95.0);
}

TEST(HoltForecaster, NameContainsAlphaAndBeta) {
    HoltForecaster holt(0.3, 0.1);
    const auto n = holt.name();
    EXPECT_NE(n.find("Holt"), std::string::npos);
    EXPECT_NE(n.find("0.3"),  std::string::npos);
    EXPECT_NE(n.find("0.1"),  std::string::npos);
}

// ── PlannerPipeline ───────────────────────────────────────────────────────────

namespace {

Item make_item(int id, const std::string& sup_id, int stock = 100, int demand = 10) {
    return Item{id, "Item-" + std::to_string(id), "WH", stock, demand, 30, 50, sup_id};
}

} // anonymous namespace

TEST(PlannerPipeline, SuccessForValidInput) {
    PlannerPipeline pipeline;
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", Supplier{"S1", "TestCo", 7, 90, "ON"}},
    };
    std::unordered_map<int, Shipment> no_shipments;

    auto result = pipeline.run(make_item(1, "S1"), suppliers, no_shipments);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value().days_until_stockout, 10); // 100/10
}

TEST(PlannerPipeline, ErrorWhenSupplierNotFound) {
    PlannerPipeline pipeline;
    std::unordered_map<std::string, Supplier> suppliers; // empty
    std::unordered_map<int, Shipment> no_shipments;

    auto result = pipeline.run(make_item(1, "MISSING"), suppliers, no_shipments);
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("MISSING"), std::string::npos);
}

TEST(PlannerPipeline, ErrorForNegativeStock) {
    PlannerPipeline pipeline;
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", Supplier{"S1", "TestCo", 7, 90, "ON"}},
    };
    std::unordered_map<int, Shipment> no_shipments;

    auto result = pipeline.run(make_item(1, "S1", -10), suppliers, no_shipments);
    EXPECT_FALSE(result.has_value());
}

TEST(PlannerPipeline, ExpectedHasValueOnSuccess) {
    PlannerPipeline pipeline;
    std::unordered_map<std::string, Supplier> suppliers = {
        {"S1", Supplier{"S1", "TestCo", 7, 90, "ON"}},
    };
    std::unordered_map<int, Shipment> no_shipments;

    auto result = pipeline.run(make_item(1, "S1"), suppliers, no_shipments);

    ASSERT_TRUE(result.has_value());
    EXPECT_GE(result.value().days_until_stockout, 0);
}

TEST(PlannerPipeline, ExpectedHasErrorOnFailure) {
    PlannerPipeline pipeline;
    std::unordered_map<std::string, Supplier> suppliers;
    std::unordered_map<int, Shipment> no_shipments;

    auto result = pipeline.run(make_item(1, "NONE"), suppliers, no_shipments);

    ASSERT_FALSE(result.has_value());
    EXPECT_FALSE(result.error().empty());
}
