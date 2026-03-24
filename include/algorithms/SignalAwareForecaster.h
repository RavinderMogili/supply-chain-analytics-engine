#pragma once

#include "algorithms/IForecaster.h"
#include "algorithms/DemandSignal.h"

#include <memory>
#include <vector>

namespace sca {

/**
 * @brief Decorator that injects real-time demand signals into any IForecaster.
 *
 * Implements the Decorator pattern on top of the existing Strategy pattern:
 * any base forecaster (SMA, EMA, Holt) can be wrapped with
 * SignalAwareForecaster to blend its historical projection with live market
 * signals before the result is returned.
 *
 * Signal application order (per forecast period):
 *  1. Base forecast is computed for all periods.
 *  2. Additive signals are applied (summed, then added to each period).
 *  3. Multiplicative signals are applied (product, then multiplied per period).
 *
 * This matches how real planning systems work: the statistical baseline is
 * computed first, then overridden or adjusted by planner-injected events
 * (promotions, competitor actions, POS spikes, etc.).
 *
 * Example:
 * @code
 *   auto base = std::make_unique<HoltForecaster>(0.3, 0.2);
 *
 *   SignalAwareForecaster forecaster(std::move(base));
 *   forecaster.add_signal({"POS",        +15.0, DemandSignal::Type::Additive,
 *                          "Weekend POS spike detected"});
 *   forecaster.add_signal({"MARKET",      1.10, DemandSignal::Type::Multiplicative,
 *                          "Competitor out of stock +10%"});
 *
 *   auto result = forecaster.forecast(history, 3);
 * @endcode
 */
class SignalAwareForecaster : public IForecaster {
public:
    /**
     * @param base     Underlying forecaster (ownership transferred).
     * @param signals  Initial set of demand signals (may be empty).
     * @throws std::invalid_argument if base is nullptr.
     */
    explicit SignalAwareForecaster(
        std::unique_ptr<IForecaster>  base,
        std::vector<DemandSignal>     signals = {});

    /**
     * @brief Add a demand signal at runtime (e.g. from a live data feed).
     */
    void add_signal(DemandSignal signal);

    /**
     * @brief Clear all registered signals.
     */
    void clear_signals();

    const std::vector<DemandSignal>& signals() const;

    /**
     * @brief Compute forecast then apply all registered signals.
     *
     * If no signals are registered the output is identical to the base
     * forecaster's output.
     */
    std::vector<double> forecast(
        const std::vector<double>& history,
        int periods) const override;

    std::string name() const override;

private:
    std::unique_ptr<IForecaster> base_;
    std::vector<DemandSignal>    signals_;
};

} // namespace sca
