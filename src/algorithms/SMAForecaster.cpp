#include "algorithms/SMAForecaster.h"

#include <numeric>
#include <stdexcept>
#include <algorithm>

namespace sca {

SMAForecaster::SMAForecaster(int window) : window_(window) {
    if (window_ < 1)
        throw std::invalid_argument("SMAForecaster: window must be >= 1");
}

std::vector<double> SMAForecaster::forecast(
    const std::vector<double>& history,
    int periods) const
{
    if (history.empty() || periods <= 0)
        return {};

    int n            = static_cast<int>(history.size());
    int actual_window = std::min(window_, n);

    double sum = std::accumulate(
        history.end() - actual_window,
        history.end(),
        0.0);

    double avg = sum / actual_window;
    return std::vector<double>(static_cast<std::size_t>(periods), avg);
}

std::string SMAForecaster::name() const {
    return "SMA(window=" + std::to_string(window_) + ")";
}

} // namespace sca
