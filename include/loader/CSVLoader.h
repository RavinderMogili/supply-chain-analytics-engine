#pragma once

#include "models/Item.h"
#include "models/Supplier.h"
#include "models/Shipment.h"

#include <string>
#include <vector>

namespace sca {

/**
 * @brief Factory-style CSV loader for supply chain data.
 *
 * Parses CSV files into typed model objects, applying all necessary
 * field-level type coercions (string → int) so callers receive clean,
 * correctly typed instances.
 *
 * All methods are static; CSVLoader itself holds no state.
 *
 * Expected CSV schemas
 * ─────────────────────
 *  inventory.csv  : item_id, item_name, warehouse, current_stock,
 *                   daily_demand, reorder_point, reorder_qty, supplier_id
 *  suppliers.csv  : supplier_id, supplier_name, lead_time_days,
 *                   reliability_score, region
 *  shipments.csv  : shipment_id, item_id, supplier_id, expected_date,
 *                   status, delayed_days, notes
 */
class CSVLoader {
public:
    /**
     * @brief Load inventory items from a CSV file.
     * @param path  Path to inventory.csv.
     * @return      List of Item objects, one per data row.
     * @throws std::runtime_error if the file cannot be opened.
     */
    static std::vector<Item>     load_items(const std::string& path);

    /**
     * @brief Load supplier records from a CSV file.
     * @param path  Path to suppliers.csv.
     * @return      List of Supplier objects, one per data row.
     * @throws std::runtime_error if the file cannot be opened.
     */
    static std::vector<Supplier> load_suppliers(const std::string& path);

    /**
     * @brief Load shipment records from a CSV file.
     * @param path  Path to shipments.csv.
     * @return      List of Shipment objects, one per data row.
     * @throws std::runtime_error if the file cannot be opened.
     */
    static std::vector<Shipment> load_shipments(const std::string& path);

private:
    /**
     * @brief Split a single CSV line on commas.
     * @param line  Raw text line (no newline).
     * @return      Vector of field strings.
     */
    static std::vector<std::string> parse_line(const std::string& line);
};

} // namespace sca
