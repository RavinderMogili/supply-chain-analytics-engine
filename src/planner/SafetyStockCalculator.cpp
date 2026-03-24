#include "planner/SafetyStockCalculator.h"

#include <cmath>
#include <stdexcept>

namespace sca {

// Rational approximation of the inverse normal CDF.
// Source: Abramowitz & Stegun (1964), formula 26.2.17. Max error: ±0.00045.
double SafetyStockCalculator::z_score(double service_level) const {
    if (service_level <= 0.0 || service_level >= 1.0)
        throw std::invalid_argument(
            "SafetyStockCalculator: service_level must be in the open interval (0, 1)");

    double p = service_level;
    if (p > 0.5) p = 1.0 - p;

    double t = std::sqrt(-2.0 * std::log(p));

    constexpr double c0 = 2.515517;
    constexpr double c1 = 0.802853;
    constexpr double c2 = 0.010328;
    constexpr double d1 = 1.432788;
    constexpr double d2 = 0.189269;
    constexpr double d3 = 0.001308;

    double z = t - (c0 + c1 * t + c2 * t * t)
                   / (1.0 + d1 * t + d2 * t * t + d3 * t * t * t);

    return (service_level >= 0.5) ? z : -z;
}

double SafetyStockCalculator::calculate(
    double service_level,
    double avg_lead_time,
    double sigma_demand,
    double avg_demand,
    double sigma_lead_time) const
{
    double z = z_score(service_level);  // throws if service_level is out of range

    // Combined variance from demand variability and lead-time variability
    double variance =
        avg_lead_time   * sigma_demand   * sigma_demand +
        avg_demand      * avg_demand     * sigma_lead_time * sigma_lead_time;

    return z * std::sqrt(variance);
}

} // namespace sca
