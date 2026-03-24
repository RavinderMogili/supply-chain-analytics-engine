#pragma once

#include <string>

namespace sca {

/**
 * @brief Represents a real-time external demand signal.
 *
 * Traditional static forecasts can be augmented with near-real-time market
 * signals such as point-of-sale data, purchase order activity, and
 * competitive market factors.
 *
 * Signals are applied on top of the base forecaster output:
 *  - Additive:       forecast[i] += adjustment
 *  - Multiplicative: forecast[i] *= adjustment
 */
struct DemandSignal {
    enum class Type {
        Additive,       ///< Add a fixed unit delta (e.g. +20 units from a promo spike)
        Multiplicative  ///< Scale by a factor   (e.g. ×1.15 for a competitor stockout)
    };

    std::string source;      ///< Signal origin: "POS", "PO_ACTIVITY", "MARKET", etc.
    double      adjustment;  ///< Delta (additive) or multiplier (multiplicative)
    Type        type;        ///< How to apply the adjustment
    std::string description; ///< Human-readable reason for the signal
};

} // namespace sca
