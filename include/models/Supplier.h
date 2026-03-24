#pragma once

#include <string>

namespace sca {

/**
 * @brief A supplier that provides one or more inventory items.
 *
 * Fields map directly to a row in data/suppliers.csv.
 */
struct Supplier {
    std::string supplier_id;       ///< Unique identifier (e.g. "S1").
    std::string supplier_name;     ///< Full company name.
    int         lead_time_days;    ///< Typical days from order placement to delivery.
    int         reliability_score; ///< On-time delivery score, 0–100.
    std::string region;            ///< Geographic region of the supplier.
};

} // namespace sca
