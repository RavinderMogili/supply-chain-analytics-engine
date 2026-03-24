#include <gtest/gtest.h>
#include "planner/EOQCalculator.h"

#include <cmath>

using namespace sca;

TEST(EOQCalculator, ClassicTextbookExample) {
    // D=1000, S=10, H=2  →  EOQ = sqrt(20000/2) = 100
    EOQCalculator eoq;
    EXPECT_NEAR(eoq.calculate(1000.0, 10.0, 2.0), 100.0, 0.001);
}

TEST(EOQCalculator, ZeroDemandReturnsZero) {
    EOQCalculator eoq;
    EXPECT_EQ(eoq.calculate(0.0, 10.0, 2.0), 0.0);
}

TEST(EOQCalculator, NegativeDemandReturnsZero) {
    EOQCalculator eoq;
    EXPECT_EQ(eoq.calculate(-100.0, 10.0, 2.0), 0.0);
}

TEST(EOQCalculator, ZeroOrderingCostReturnsZero) {
    EOQCalculator eoq;
    EXPECT_EQ(eoq.calculate(1000.0, 0.0, 2.0), 0.0);
}

TEST(EOQCalculator, ZeroHoldingCostReturnsZero) {
    EOQCalculator eoq;
    EXPECT_EQ(eoq.calculate(1000.0, 10.0, 0.0), 0.0);
}

TEST(EOQCalculator, DoublingDemandScalesEOQBySqrt2) {
    // EOQ is proportional to sqrt(D), so doubling D multiplies EOQ by sqrt(2)
    EOQCalculator eoq;
    const double e1 = eoq.calculate(1000.0, 10.0, 2.0);
    const double e2 = eoq.calculate(2000.0, 10.0, 2.0);
    EXPECT_NEAR(e2 / e1, std::sqrt(2.0), 0.001);
}

TEST(EOQCalculator, DoublingOrderingCostScalesEOQBySqrt2) {
    EOQCalculator eoq;
    const double e1 = eoq.calculate(1000.0, 10.0, 2.0);
    const double e2 = eoq.calculate(1000.0, 20.0, 2.0);
    EXPECT_NEAR(e2 / e1, std::sqrt(2.0), 0.001);
}

TEST(EOQCalculator, DoublingHoldingCostReducesEOQBySqrt2) {
    // Higher holding cost → smaller optimal batch
    EOQCalculator eoq;
    const double e1 = eoq.calculate(1000.0, 10.0, 2.0);
    const double e2 = eoq.calculate(1000.0, 10.0, 4.0);
    EXPECT_NEAR(e1 / e2, std::sqrt(2.0), 0.001);
}

TEST(EOQCalculator, ResultIsAlwaysPositive) {
    EOQCalculator eoq;
    EXPECT_GT(eoq.calculate(500.0, 25.0, 5.0), 0.0);
}
