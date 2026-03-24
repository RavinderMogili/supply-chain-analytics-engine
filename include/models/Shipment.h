#pragma once

#include <string>

namespace sca {

/**
 * @brief A single in-progress or expected delivery of an item from a supplier.
 *
 * Fields map directly to a row in data/shipments.csv.
 */
struct Shipment {
    std::string shipment_id;    ///< Unique shipment reference (e.g. "SH001").
    int         item_id;        ///< Foreign key referencing Item::item_id.
    std::string supplier_id;    ///< Foreign key referencing Supplier::supplier_id.
    std::string expected_date;  ///< ISO-format expected delivery date (YYYY-MM-DD).
    std::string status;         ///< Current status: "In Transit", "Delayed", etc.
    int         delayed_days;   ///< Days behind schedule; 0 if on time.
    std::string notes;          ///< Free-text operational note or delay reason.
};

} // namespace sca
