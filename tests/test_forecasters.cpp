#include <gtest/gtest.h>
#include "algorithms/SMAForecaster.h"
#include "algorithms/EMAForecaster.h"

using namespace sca;

// ── SMAForecaster ─────────────────────────────────────────────────────────────

TEST(SMAForecaster, InvalidWindowThrows) {
    EXPECT_THROW(SMAForecaster(0),  std::invalid_argument);
    EXPECT_THROW(SMAForecaster(-1), std::invalid_argument);
}

TEST(SMAForecaster, SingleElementHistory) {
    SMAForecaster sma(3);
    auto result = sma.forecast({42.0}, 2);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], 42.0);
    EXPECT_DOUBLE_EQ(result[1], 42.0);
}

TEST(SMAForecaster, WindowLargerThanHistoryUsesAllValues) {
    SMAForecaster sma(5);
    auto result = sma.forecast({4.0, 6.0}, 1);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result[0], 5.0);   // (4+6)/2
}

TEST(SMAForecaster, ExactWindowSize) {
    SMAForecaster sma(3);
    // last 3 values: 10, 20, 30 → average = 20
    auto result = sma.forecast({5.0, 10.0, 20.0, 30.0}, 2);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0], 20.0);
    EXPECT_DOUBLE_EQ(result[1], 20.0);
}

TEST(SMAForecaster, WindowOfOne) {
    SMAForecaster sma(1);
    auto result = sma.forecast({10.0, 20.0, 99.0}, 3);
    // Window=1 → always returns last value
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 99.0);
}

TEST(SMAForecaster, EmptyHistoryReturnsEmpty) {
    SMAForecaster sma(3);
    EXPECT_TRUE(sma.forecast({}, 3).empty());
}

TEST(SMAForecaster, ZeroPeriodsReturnsEmpty) {
    SMAForecaster sma(3);
    EXPECT_TRUE(sma.forecast({10.0, 20.0}, 0).empty());
}

TEST(SMAForecaster, NameContainsWindow) {
    SMAForecaster sma(5);
    EXPECT_NE(sma.name().find("5"), std::string::npos);
}

// ── EMAForecaster ─────────────────────────────────────────────────────────────

TEST(EMAForecaster, InvalidAlphaThrows) {
    EXPECT_THROW(EMAForecaster(0.0),  std::invalid_argument);
    EXPECT_THROW(EMAForecaster(1.0),  std::invalid_argument);
    EXPECT_THROW(EMAForecaster(-0.1), std::invalid_argument);
    EXPECT_THROW(EMAForecaster(1.5),  std::invalid_argument);
}

TEST(EMAForecaster, SingleElementHistory) {
    EMAForecaster ema(0.5);
    auto result = ema.forecast({42.0}, 3);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0], 42.0);
}

TEST(EMAForecaster, HighAlphaWeightsRecentObservationMoreHeavily) {
    // alpha=0.9 means the last observation dominates
    EMAForecaster ema(0.9);
    auto result = ema.forecast({10.0, 100.0}, 1);
    ASSERT_EQ(result.size(), 1u);
    // EMA = 0.9*100 + 0.1*10 = 91.0
    EXPECT_NEAR(result[0], 91.0, 1e-9);
}

TEST(EMAForecaster, LowAlphaSmoothesPastHistory) {
    // alpha=0.1 means history dominates, recent spike has little impact
    EMAForecaster ema(0.1);
    auto result_low  = ema.forecast({10.0, 10.0, 10.0, 10.0, 100.0}, 1);
    EMAForecaster ema_high(0.9);
    auto result_high = ema_high.forecast({10.0, 10.0, 10.0, 10.0, 100.0}, 1);
    EXPECT_LT(result_low[0], result_high[0]);
}

TEST(EMAForecaster, AllPeriodsReceiveSameFlatValue) {
    EMAForecaster ema(0.3);
    auto result = ema.forecast({10.0, 20.0, 30.0}, 4);
    ASSERT_EQ(result.size(), 4u);
    EXPECT_DOUBLE_EQ(result[0], result[1]);
    EXPECT_DOUBLE_EQ(result[1], result[2]);
    EXPECT_DOUBLE_EQ(result[2], result[3]);
}

TEST(EMAForecaster, EmptyHistoryReturnsEmpty) {
    EMAForecaster ema(0.3);
    EXPECT_TRUE(ema.forecast({}, 3).empty());
}

TEST(EMAForecaster, NameContainsAlpha) {
    EMAForecaster ema(0.3);
    EXPECT_NE(ema.name().find("0.3"), std::string::npos);
}
