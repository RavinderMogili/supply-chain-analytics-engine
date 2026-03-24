#include "planner/BatchPlanner.h"
#include "planner/PlannerCache.h"
#include "models/Item.h"
#include "models/Supplier.h"
#include "models/Shipment.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <random>
#include <string>

using namespace sca;

// ── Synthetic data generators ─────────────────────────────────────────────────

static std::vector<Item> generate_items(int n, std::mt19937& rng) {
    std::uniform_int_distribution<int> stock_dist(5, 500);
    std::uniform_int_distribution<int> demand_dist(1, 50);
    std::uniform_int_distribution<int> sup_dist(1, 4);

    std::vector<Item> items;
    items.reserve(static_cast<std::size_t>(n));

    for (int i = 0; i < n; ++i) {
        const int  demand = demand_dist(rng);
        const int  stock  = stock_dist(rng);
        std::string sup   = "S" + std::to_string(sup_dist(rng));

        items.push_back(Item{
            i,
            "Item-" + std::to_string(i),
            "WH",
            stock,
            demand,
            stock / 3,
            demand * 7,
            sup
        });
    }
    return items;
}

static std::unordered_map<std::string, Supplier> make_suppliers() {
    return {
        {"S1", Supplier{"S1", "Alpha Components",  7, 92, "Ontario"}},
        {"S2", Supplier{"S2", "Delta Supply",     12, 80, "Texas"}},
        {"S3", Supplier{"S3", "Nova Industrial",   5, 95, "Quebec"}},
        {"S4", Supplier{"S4", "Crest Logistics",  10, 70, "BC"}},
    };
}

// ── Timing helper ─────────────────────────────────────────────────────────────

struct TimedResult {
    double      elapsed_ms;
    double      items_per_sec;
};

template<typename Fn>
static TimedResult timed(int n, Fn&& fn) {
    auto t0 = std::chrono::high_resolution_clock::now();
    fn();
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return {ms, static_cast<double>(n) / (ms / 1000.0)};
}

static void print_row(const std::string& label, const TimedResult& r,
                      const std::string& note = "") {
    std::cout << std::left  << std::setw(32) << label
              << std::right << std::setw(10) << std::fixed << std::setprecision(1)
              << r.elapsed_ms << " ms"
              << std::setw(16) << std::fixed << std::setprecision(0)
              << r.items_per_sec << " items/s";
    if (!note.empty()) std::cout << "   " << note;
    std::cout << "\n";
}

// ── Main ──────────────────────────────────────────────────────────────────────

int main() {
    constexpr int N_ITEMS    = 50'000;
    constexpr int BATCH_SIZE = 512;
    constexpr int CACHE_CAP  = 60'000; // larger than dataset → 100% warm hit rate

    std::mt19937 rng(42); // fixed seed → reproducible results

    const auto items       = generate_items(N_ITEMS, rng);
    const auto suppliers   = make_suppliers();
    const std::unordered_map<int, Shipment> no_shipments;

    std::cout << "\n";
    std::cout << "===================================================================\n";
    std::cout << "   Supply Chain Analytics Engine - Performance Benchmark\n";
    std::cout << "===================================================================\n\n";
    std::cout << "  Dataset    : " << N_ITEMS    << " synthetic items (fixed seed = 42)\n";
    std::cout << "  Batch size : " << BATCH_SIZE << " items/task\n";
    std::cout << "  Cache cap  : " << CACHE_CAP  << " entries\n\n";

    std::cout << std::left  << std::setw(32) << "Scenario"
              << std::right << std::setw(12) << "Time"
              << std::setw(18) << "Throughput" << "\n";
    std::cout << std::string(70, '-') << "\n";

    // ── 1. Single-threaded, no cache (baseline) ───────────────────────────────
    {
        BatchPlanner planner(N_ITEMS, nullptr); // one giant batch = serial
        auto r = timed(N_ITEMS, [&]() {
            planner.analyze_all(items, suppliers, no_shipments);
        });
        print_row("1. Serial / no cache (baseline)", r);
    }

    // ── 2. Multi-threaded, no cache ───────────────────────────────────────────
    {
        BatchPlanner planner(BATCH_SIZE, nullptr);
        auto r = timed(N_ITEMS, [&]() {
            planner.analyze_all(items, suppliers, no_shipments);
        });
        print_row("2. Parallel batches / no cache", r, "<-- std::async speedup");
    }

    // ── 3. Multi-threaded, cache cold (first pass — populates cache) ──────────
    PlannerCache cache(CACHE_CAP);
    {
        BatchPlanner planner(BATCH_SIZE, &cache);
        auto r = timed(N_ITEMS, [&]() {
            planner.analyze_all(items, suppliers, no_shipments);
        });
        print_row("3. Parallel + cache (cold pass)", r,
            "<-- " + std::to_string(cache.misses()) + " misses, cache populated");
    }

    // ── 4. Multi-threaded, cache warm (second pass — all hits) ───────────────
    {
        BatchPlanner planner(BATCH_SIZE, &cache);
        auto r = timed(N_ITEMS, [&]() {
            planner.analyze_all(items, suppliers, no_shipments);
        });
        const int hit_pct = static_cast<int>(cache.hit_rate() * 100.0 + 0.5);
        print_row("4. Parallel + cache (warm pass)", r,
            "<-- " + std::to_string(hit_pct) + "% cache hit rate");
    }

    std::cout << std::string(70, '-') << "\n\n";

    // ── Cache summary ─────────────────────────────────────────────────────────
    std::cout << "Cache statistics (across all passes):\n";
    std::cout << "  Hits    : " << cache.hits()   << "\n";
    std::cout << "  Misses  : " << cache.misses() << "\n";
    std::cout << "  Size    : " << cache.size()   << " / " << cache.capacity() << "\n";
    std::cout << "  Hit rate: " << std::fixed << std::setprecision(1)
              << (cache.hit_rate() * 100.0) << "%\n\n";

    std::cout << "Why this matters in a cloud planning system:\n";
    std::cout << "  Each 'analyze' call represents a round-trip to fetch live\n";
    std::cout << "  inventory + supplier data. Caching eliminates redundant reads\n";
    std::cout << "  for items unchanged since the last planning cycle.\n";
    std::cout << "  Parallel batching amortises per-request latency across threads.\n\n";

    return 0;
}
