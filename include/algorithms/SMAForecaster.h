#pragma once

#include "algorithms/IForecaster.h"

namespace sca {

/**
 * @brief Simple Moving Average (SMA) demand forecaster.
 *
 * Averages the last @p window observations from the history to produce
 * a flat forward projection.  All forecasted periods receive the same
 * value (constant extrapolation).
 *
 * If the history has fewer entries than @p window, all available entries
 * are used instead of throwing.
 */
class SMAForecaster : public IForecaster {
public:
    /**
     * @brief Construct with a look-back window size.
     * @param window Number of most-recent observations to average. Must be >= 1.
     * @throws std::invalid_argument if window < 1.
     */
    explicit SMAForecaster(int window = 3);

    std::vector<double> forecast(
        const std::vector<double>& history,
        int periods) const override;

    std::string name() const override;

private:
    int window_;
};

} // namespace sca
