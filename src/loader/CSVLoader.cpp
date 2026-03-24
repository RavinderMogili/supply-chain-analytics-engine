#include "loader/CSVLoader.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace sca {

std::vector<std::string> CSVLoader::parse_line(const std::string& line) {
    std::vector<std::string> fields;
    std::stringstream ss(line);
    std::string field;
    while (std::getline(ss, field, ','))
        fields.push_back(field);
    return fields;
}

std::vector<Item> CSVLoader::load_items(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("CSVLoader: cannot open file: " + path);

    std::vector<Item> items;
    std::string line;
    std::getline(file, line); // skip header row

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto f = parse_line(line);
        if (f.size() < 8) continue;

        items.push_back(Item{
            std::stoi(f[0]),  // item_id
            f[1],             // item_name
            f[2],             // warehouse
            std::stoi(f[3]),  // current_stock
            std::stoi(f[4]),  // daily_demand
            std::stoi(f[5]),  // reorder_point
            std::stoi(f[6]),  // reorder_qty
            f[7]              // supplier_id
        });
    }
    return items;
}

std::vector<Supplier> CSVLoader::load_suppliers(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("CSVLoader: cannot open file: " + path);

    std::vector<Supplier> suppliers;
    std::string line;
    std::getline(file, line); // skip header row

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto f = parse_line(line);
        if (f.size() < 5) continue;

        suppliers.push_back(Supplier{
            f[0],             // supplier_id
            f[1],             // supplier_name
            std::stoi(f[2]),  // lead_time_days
            std::stoi(f[3]),  // reliability_score
            f[4]              // region
        });
    }
    return suppliers;
}

std::vector<Shipment> CSVLoader::load_shipments(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("CSVLoader: cannot open file: " + path);

    std::vector<Shipment> shipments;
    std::string line;
    std::getline(file, line); // skip header row

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto f = parse_line(line);
        if (f.size() < 7) continue;

        shipments.push_back(Shipment{
            f[0],             // shipment_id
            std::stoi(f[1]),  // item_id
            f[2],             // supplier_id
            f[3],             // expected_date
            f[4],             // status
            std::stoi(f[5]),  // delayed_days
            f[6]              // notes
        });
    }
    return shipments;
}

} // namespace sca
