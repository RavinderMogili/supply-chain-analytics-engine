#include <gtest/gtest.h>
#include "planner/SafetyStockCalculator.h"

using namespace sca;

TEST(SafetyStockCalculator, PositiveResultForTypicalInputs) {
    SafetyStockCalculator calc;
    double ss = calc.calculate(0.95, 7.0, 5.0, 20.0, 2.0);
    EXPECT_GT(ss, 0.0);
}

TEST(SafetyStockCalculator, HigherServiceLevelRequiresMoreSafetyStock) {
    SafetyStockCalculator calc;
    double ss_95 = calc.calculate(0.95, 7.0, 5.0, 20.0, 2.0);
    double ss_99 = calc.calculate(0.99, 7.0, 5.0, 20.0, 2.0);
    EXPECT_GT(ss_99, ss_95);
}

TEST(SafetyStockCalculator, LongerLeadTimeRequiresMoreSafetyStock) {
    SafetyStockCalculator calc;
    double ss_short = calc.calculate(0.95,  5.0, 5.0, 20.0, 1.0);
    double ss_long  = calc.calculate(0.95, 15.0, 5.0, 20.0, 1.0);
    EXPECT_GT(ss_long, ss_short);
}

TEST(SafetyStockCalculator, HigherDemandVariabilityRequiresMoreSafetyStock) {
    SafetyStockCalculator calc;
    double ss_low  = calc.calculate(0.95, 7.0, 2.0, 20.0, 1.0);
    double ss_high = calc.calculate(0.95, 7.0, 8.0, 20.0, 1.0);
    EXPECT_GT(ss_high, ss_low);
}

TEST(SafetyStockCalculator, ZeroVariabilityProducesLowSafetyStock) {
    // With no demand variance and no lead-time variance, safety stock → 0
    SafetyStockCalculator calc;
    double ss = calc.calculate(0.95, 7.0, 0.0, 20.0, 0.0);
    EXPECT_NEAR(ss, 0.0, 1e-9);
}

TEST(SafetyStockCalculator, ServiceLevelZeroThrows) {
    SafetyStockCalculator calc;
    EXPECT_THROW(calc.calculate(0.0, 7.0, 5.0, 20.0, 2.0), std::invalid_argument);
}

TEST(SafetyStockCalculator, ServiceLevelOneThrows) {
    SafetyStockCalculator calc;
    EXPECT_THROW(calc.calculate(1.0, 7.0, 5.0, 20.0, 2.0), std::invalid_argument);
}

TEST(SafetyStockCalculator, ServiceLevelAboveOneThrows) {
    SafetyStockCalculator calc;
    EXPECT_THROW(calc.calculate(1.5, 7.0, 5.0, 20.0, 2.0), std::invalid_argument);
}

TEST(SafetyStockCalculator, KnownZScoreAt95Percent) {
    // Z(0.95) ≈ 1.645; with sigma_d=1, LT=1, sigma_lt=0: SS ≈ 1.645
    SafetyStockCalculator calc;
    double ss = calc.calculate(0.95, 1.0, 1.0, 0.0, 0.0);
    EXPECT_NEAR(ss, 1.645, 0.01);
}
