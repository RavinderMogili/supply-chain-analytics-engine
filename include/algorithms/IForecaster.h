#pragma once

#include <vector>
#include <string>

namespace sca {

/**
 * @brief Strategy interface for demand forecasting algorithms.
 *
 * Concrete implementations (SMAForecaster, EMAForecaster, …) are
 * interchangeable through this interface, following the Strategy pattern.
 * The caller decides at runtime which algorithm to use without any
 * coupling to its implementation details.
 *
 * @see SMAForecaster
 * @see EMAForecaster
 */
class IForecaster {
public:
    virtual ~IForecaster() = default;

    /**
     * @brief Generate a demand forecast for future periods.
     *
     * @param history  Historical demand values, oldest observation first.
     * @param periods  Number of future periods to forecast.
     * @return         Forecasted demand for each of the next @p periods.
     *                 Returns an empty vector if @p history is empty or
     *                 @p periods <= 0.
     */
    virtual std::vector<double> forecast(
        const std::vector<double>& history,
        int periods) const = 0;

    /**
     * @brief Human-readable identifier for this algorithm (e.g. "SMA(window=3)").
     */
    virtual std::string name() const = 0;
};

} // namespace sca
