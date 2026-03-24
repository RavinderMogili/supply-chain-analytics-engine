#include "algorithms/EMAForecaster.h"

#include <stdexcept>

namespace sca {

EMAForecaster::EMAForecaster(double alpha) : alpha_(alpha) {
    if (alpha_ <= 0.0 || alpha_ >= 1.0)
        throw std::invalid_argument("EMAForecaster: alpha must be in the open interval (0, 1)");
}

std::vector<double> EMAForecaster::forecast(
    const std::vector<double>& history,
    int periods) const
{
    if (history.empty() || periods <= 0)
        return {};

    // Seed with the first observation, then walk through the rest
    double ema = history[0];
    for (std::size_t i = 1; i < history.size(); ++i) {
        ema = alpha_ * history[i] + (1.0 - alpha_) * ema;
    }

    // Flat projection: all future periods receive the final EMA value
    return std::vector<double>(static_cast<std::size_t>(periods), ema);
}

std::string EMAForecaster::name() const {
    return "EMA(alpha=" + std::to_string(alpha_) + ")";
}

} // namespace sca
