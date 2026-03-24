#pragma once

namespace sca {

/**
 * @brief Calculates safety stock accounting for both demand and lead-time variability.
 *
 * Safety stock is the buffer inventory held to protect against stockouts
 * caused by unpredictable swings in demand and supplier lead time.
 *
 * Formula:
 *   SS = Z * sqrt( avg_lead_time * sigma_d^2 + avg_demand^2 * sigma_lt^2 )
 *
 *   Z          =  service-level Z-score (derived from @p service_level)
 *   sigma_d    =  standard deviation of daily demand
 *   avg_demand =  mean daily demand
 *   sigma_lt   =  standard deviation of lead time (days)
 *
 * The Z-score is approximated using the rational approximation by
 * Abramowitz & Stegun (1964), which is accurate to ±0.00045.
 */
class SafetyStockCalculator {
public:
    /**
     * @brief Compute safety stock.
     *
     * @param service_level    Target fill rate, e.g. 0.95 for 95% service level.
     *                         Must be strictly between 0 and 1.
     * @param avg_lead_time    Average supplier lead time in days.
     * @param sigma_demand     Standard deviation of daily demand (units/day).
     * @param avg_demand       Mean daily demand (units/day).
     * @param sigma_lead_time  Standard deviation of lead time in days.
     * @return Required safety stock in units (always >= 0).
     * @throws std::invalid_argument if service_level is not in (0, 1).
     */
    double calculate(
        double service_level,
        double avg_lead_time,
        double sigma_demand,
        double avg_demand,
        double sigma_lead_time) const;

private:
    /**
     * @brief Approximate inverse normal CDF (percent-point function).
     * @param service_level Value in (0, 1).
     * @return Z-score for the given service level.
     */
    double z_score(double service_level) const;
};

} // namespace sca
