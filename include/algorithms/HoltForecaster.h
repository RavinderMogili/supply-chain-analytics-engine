#pragma once

#include "algorithms/IForecaster.h"

namespace sca {

/**
 * @brief Holt's Double Exponential Smoothing (trend-corrected forecaster).
 *
 * Unlike SMA and EMA which produce flat forward projections, Holt's method
 * captures a linear trend in the history and extrapolates it into the future,
 * making it suitable for demand series with a consistent upward or downward drift.
 *
 * Level:   L_t = alpha * x_t + (1 - alpha) * (L_{t-1} + T_{t-1})
 * Trend:   T_t = beta  * (L_t - L_{t-1}) + (1 - beta)  * T_{t-1}
 * Forecast h steps ahead: F_{t+h} = L_t + h * T_t
 *
 * Initialisation: L_0 = x_0, T_0 = x_1 - x_0
 *
 * Reference: Holt, C.E. (1957). "Forecasting seasonals and trends by
 * exponentially weighted averages." ONR Memorandum 52/1957.
 */
class HoltForecaster : public IForecaster {
public:
    /**
     * @param alpha Level smoothing factor — controls responsiveness to new observations.
     *              Must be in the open interval (0, 1).
     * @param beta  Trend smoothing factor — controls how quickly trend estimates adapt.
     *              Must be in the open interval (0, 1).
     * @throws std::invalid_argument if alpha or beta are not in (0, 1).
     */
    explicit HoltForecaster(double alpha = 0.3, double beta = 0.1);

    std::vector<double> forecast(
        const std::vector<double>& history,
        int periods) const override;

    std::string name() const override;

private:
    double alpha_;
    double beta_;
};

} // namespace sca
