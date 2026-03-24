#pragma once

#include "algorithms/IForecaster.h"

namespace sca {

/**
 * @brief Exponential Moving Average (EMA) demand forecaster.
 *
 * Applies exponential smoothing: each new observation is blended with the
 * current EMA using a smoothing factor @p alpha.  Higher @p alpha gives
 * more weight to recent observations; lower @p alpha gives more weight to
 * older history.
 *
 * Formula per step:  EMA_t = alpha * x_t + (1 - alpha) * EMA_{t-1}
 *
 * All forecasted future periods receive the EMA value computed from the
 * last observation in the history (flat projection).
 */
class EMAForecaster : public IForecaster {
public:
    /**
     * @brief Construct with a smoothing factor.
     * @param alpha Smoothing factor in the open interval (0, 1).
     * @throws std::invalid_argument if alpha is not in (0, 1).
     */
    explicit EMAForecaster(double alpha = 0.3);

    std::vector<double> forecast(
        const std::vector<double>& history,
        int periods) const override;

    std::string name() const override;

private:
    double alpha_;
};

} // namespace sca
