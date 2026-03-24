#include "algorithms/HoltForecaster.h"

#include <stdexcept>
#include <string>

namespace sca {

HoltForecaster::HoltForecaster(double alpha, double beta)
    : alpha_(alpha), beta_(beta)
{
    if (alpha_ <= 0.0 || alpha_ >= 1.0)
        throw std::invalid_argument("HoltForecaster: alpha must be in the open interval (0, 1)");
    if (beta_ <= 0.0 || beta_ >= 1.0)
        throw std::invalid_argument("HoltForecaster: beta must be in the open interval (0, 1)");
}

std::vector<double> HoltForecaster::forecast(
    const std::vector<double>& history,
    int periods) const
{
    if (history.empty() || periods <= 0)
        return {};

    if (history.size() == 1)
        return std::vector<double>(static_cast<std::size_t>(periods), history[0]);

    // Initialise level and trend from the first two observations
    double L = history[0];
    double T = history[1] - history[0];

    // Walk through remaining history
    for (std::size_t t = 1; t < history.size(); ++t) {
        const double L_prev = L;
        L = alpha_ * history[t] + (1.0 - alpha_) * (L_prev + T);
        T = beta_  * (L - L_prev) + (1.0 - beta_) * T;
    }

    // Project forward: each future period extrapolates the trend
    std::vector<double> result;
    result.reserve(static_cast<std::size_t>(periods));
    for (int h = 1; h <= periods; ++h)
        result.push_back(L + static_cast<double>(h) * T);

    return result;
}

std::string HoltForecaster::name() const {
    return "Holt(alpha=" + std::to_string(alpha_) +
           ", beta="      + std::to_string(beta_)  + ")";
}

} // namespace sca
