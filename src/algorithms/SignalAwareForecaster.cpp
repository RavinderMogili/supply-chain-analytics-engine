#include "algorithms/SignalAwareForecaster.h"

#include <stdexcept>
#include <numeric>

namespace sca {

SignalAwareForecaster::SignalAwareForecaster(
    std::unique_ptr<IForecaster> base,
    std::vector<DemandSignal>    signals)
    : base_(std::move(base)), signals_(std::move(signals))
{
    if (!base_)
        throw std::invalid_argument("SignalAwareForecaster: base forecaster must not be null");
}

void SignalAwareForecaster::add_signal(DemandSignal signal) {
    signals_.push_back(std::move(signal));
}

void SignalAwareForecaster::clear_signals() {
    signals_.clear();
}

const std::vector<DemandSignal>& SignalAwareForecaster::signals() const {
    return signals_;
}

std::vector<double> SignalAwareForecaster::forecast(
    const std::vector<double>& history,
    int periods) const
{
    auto result = base_->forecast(history, periods);
    if (result.empty()) return result;

    // Accumulate adjustments across all signals
    double additive_total   = 0.0;
    double multiplicative   = 1.0;

    for (const auto& sig : signals_) {
        if (sig.type == DemandSignal::Type::Additive)
            additive_total += sig.adjustment;
        else
            multiplicative *= sig.adjustment;
    }

    // Apply to every forecast period
    for (auto& v : result) {
        v = (v + additive_total) * multiplicative;
        if (v < 0.0) v = 0.0; // demand cannot be negative
    }

    return result;
}

std::string SignalAwareForecaster::name() const {
    std::string n = "SignalAware(" + base_->name() + ")";
    if (!signals_.empty()) {
        n += "[";
        for (std::size_t i = 0; i < signals_.size(); ++i) {
            n += signals_[i].source;
            if (i + 1 < signals_.size()) n += ",";
        }
        n += "]";
    }
    return n;
}

} // namespace sca
