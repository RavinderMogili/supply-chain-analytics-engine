#pragma once

namespace sca {

/**
 * @brief Calculates the Economic Order Quantity (EOQ).
 *
 * EOQ is the order size that minimises total inventory cost, balancing
 * ordering frequency (ordering cost) against average stock held (holding cost).
 *
 * Formula:  EOQ = sqrt( 2 * D * S / H )
 *
 *   D  =  annual demand (units/year)
 *   S  =  ordering cost per order placed  ($/order)
 *   H  =  annual holding cost per unit    ($/unit/year)
 *
 * Reference: Harris, F.W. (1913). "How Many Parts to Make at Once."
 */
class EOQCalculator {
public:
    /**
     * @brief Compute the Economic Order Quantity.
     *
     * @param annual_demand          Annual units demanded (D).
     * @param ordering_cost          Cost to place one order (S).
     * @param holding_cost_per_unit  Annual cost to hold one unit (H).
     * @return EOQ in units, or 0.0 if any input is <= 0.
     */
    double calculate(
        double annual_demand,
        double ordering_cost,
        double holding_cost_per_unit) const;
};

} // namespace sca
