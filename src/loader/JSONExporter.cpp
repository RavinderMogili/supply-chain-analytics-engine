#include "loader/JSONExporter.h"

#include <fstream>
#include <iomanip>
#include <stdexcept>

namespace sca {

std::string JSONExporter::escape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else                out += c;
    }
    return out;
}

void JSONExporter::export_report(
    const std::vector<Item>&           items,
    const std::vector<PlanningResult>& results,
    const std::vector<RiskEntry>&      risk_entries,
    const std::string&                 path)
{
    std::ofstream f(path);
    if (!f.is_open())
        throw std::runtime_error("JSONExporter: cannot open file for writing: " + path);

    f << "{\n";

    // ── planning_results ──────────────────────────────────────────────────────
    f << "  \"planning_results\": [\n";
    for (std::size_t i = 0; i < items.size(); ++i) {
        const Item&          item   = items[i];
        const PlanningResult& result = results[i];

        f << "    {\n";
        f << "      \"item_id\": "              << item.item_id                              << ",\n";
        f << "      \"item_name\": \""          << escape(item.item_name)                    << "\",\n";
        f << "      \"warehouse\": \""          << escape(item.warehouse)                    << "\",\n";
        f << "      \"current_stock\": "        << item.current_stock                        << ",\n";
        f << "      \"daily_demand\": "         << item.daily_demand                         << ",\n";
        f << "      \"supplier_id\": \""        << escape(item.supplier_id)                  << "\",\n";
        f << "      \"days_until_stockout\": "  << result.days_until_stockout                << ",\n";
        f << "      \"lead_time_demand\": "     << result.lead_time_demand                   << ",\n";
        f << "      \"reorder_now\": "          << (result.reorder_now ? "true" : "false")   << ",\n";
        f << "      \"reorder_in_days\": "      << result.reorder_in_days                    << ",\n";
        f << "      \"risk_level\": \""         << escape(to_string(result.risk_level))      << "\",\n";
        f << "      \"recommendation\": \""     << escape(result.recommendation)             << "\"\n";
        f << "    }";
        if (i + 1 < items.size()) f << ",";
        f << "\n";
    }
    f << "  ],\n";

    // ── risk_ranking ──────────────────────────────────────────────────────────
    f << "  \"risk_ranking\": [\n";
    for (std::size_t i = 0; i < risk_entries.size(); ++i) {
        const RiskEntry& entry = risk_entries[i];

        f << "    {\n";
        f << "      \"item_name\": \""  << escape(entry.item_name) << "\",\n";
        f << "      \"score\": "        << std::fixed << std::setprecision(3) << entry.score << ",\n";
        f << "      \"status\": \""     << escape(entry.summary)   << "\"\n";
        f << "    }";
        if (i + 1 < risk_entries.size()) f << ",";
        f << "\n";
    }
    f << "  ]\n";

    f << "}\n";
}

} // namespace sca
