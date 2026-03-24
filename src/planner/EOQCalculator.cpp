#include "planner/EOQCalculator.h"

#include <cmath>

namespace sca {

double EOQCalculator::calculate(
    double annual_demand,
    double ordering_cost,
    double holding_cost_per_unit) const
{
    if (annual_demand <= 0.0 || ordering_cost <= 0.0 || holding_cost_per_unit <= 0.0)
        return 0.0;

    return std::sqrt((2.0 * annual_demand * ordering_cost) / holding_cost_per_unit);
}

} // namespace sca
