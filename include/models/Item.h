#pragma once

#include <string>

namespace sca {

/**
 * @brief An inventory item held in a warehouse.
 *
 * Fields map directly to a row in data/inventory.csv.
 */
struct Item {
    int         item_id;        ///< Unique numeric identifier.
    std::string item_name;      ///< Human-readable name (e.g. "Widget-A").
    std::string warehouse;      ///< Warehouse location code (e.g. "WH-East").
    int         current_stock;  ///< Units currently on hand.
    int         daily_demand;   ///< Average units consumed per day.
    int         reorder_point;  ///< Stock level that triggers a reorder evaluation.
    int         reorder_qty;    ///< Units to order when a reorder is placed.
    std::string supplier_id;    ///< Foreign key referencing the item's primary Supplier.
};

} // namespace sca
