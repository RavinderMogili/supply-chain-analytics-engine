# Supply Chain Analytics Engine

![CI](https://github.com/RavinderMogili/supply-chain-analytics-engine/actions/workflows/build.yml/badge.svg)

A focused C++ library demonstrating high-performance, testable supply chain
planning algorithms — designed to showcase algorithm development skills
relevant to supply chain software roles.

---

## Table of Contents

1. [Overview](#overview)
2. [Project structure](#project-structure)
3. [Architecture & design patterns](#architecture--design-patterns)
4. [Algorithms implemented](#algorithms-implemented)
5. [Build instructions](#build-instructions)
6. [Run the demo](#run-the-demo)
7. [Run the tests](#run-the-tests)
8. [Example output](#example-output)
9. [Technologies](#technologies)

---

## Overview

This project implements core inventory planning algorithms used in real
supply chain systems:

- **Stockout risk classification** — identifies items at risk of running out
  before replenishment arrives, accounting for supplier delay
- **Economic Order Quantity (EOQ)** — calculates the optimal reorder batch size
  that minimises total ordering + holding cost
- **Safety stock** — computes buffer inventory required for a target service
  level given demand and lead-time variability
- **Demand forecasting** — pluggable Strategy interface with SMA, EMA, and
  Holt's trend-corrected double exponential smoothing
- **Portfolio risk scoring** — ranks all items by a composite risk score to
  prioritise planner attention
- **LRU cache + parallel batch planner** — thread-safe result cache with
  `std::async` batch processing; simulates the caching + request-batching
  pattern used to address cloud round-trip overhead in large planning systems
- **std::variant pipeline** — typed result/error propagation without exceptions
- **JSON report export** — machine-readable output with no external dependencies

---

## Project structure

```
supply-chain-analytics-engine/
├── CMakeLists.txt
├── include/
│   ├── algorithms/
│   │   ├── IForecaster.h          # Strategy interface
│   │   ├── SMAForecaster.h        # Simple Moving Average
│   │   ├── EMAForecaster.h        # Exponential Moving Average
│   │   └── HoltForecaster.h       # Holt's trend-corrected forecaster
│   ├── loader/
│   │   ├── CSVLoader.h            # Factory-style CSV ingestion
│   │   └── JSONExporter.h         # Structured JSON report writer
│   ├── models/
│   │   ├── Item.h
│   │   ├── Supplier.h
│   │   ├── Shipment.h
│   │   └── PlanningResult.h       # RiskLevel enum + result struct
│   └── planner/
│       ├── BatchPlanner.h         # std::async parallel batch planner
│       ├── EOQCalculator.h
│       ├── InventoryPlanner.h
│       ├── PlannerCache.h         # Thread-safe LRU cache
│       ├── PlannerPipeline.h      # std::variant result/error pipeline
│       ├── RiskScorer.h
│       └── SafetyStockCalculator.h
├── src/
│   ├── algorithms/
│   │   ├── SMAForecaster.cpp
│   │   ├── EMAForecaster.cpp
│   │   └── HoltForecaster.cpp
│   ├── loader/
│   │   ├── CSVLoader.cpp
│   │   └── JSONExporter.cpp
│   └── planner/
│       ├── BatchPlanner.cpp
│       ├── EOQCalculator.cpp
│       ├── InventoryPlanner.cpp
│       ├── PlannerCache.cpp
│       ├── PlannerPipeline.cpp
│       ├── RiskScorer.cpp
│       └── SafetyStockCalculator.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── test_cache.cpp             # LRU cache + BatchPlanner + thread safety
│   ├── test_eoq.cpp
│   ├── test_forecasters.cpp       # SMA + EMA
│   ├── test_holt.cpp              # HoltForecaster + PlannerPipeline
│   ├── test_planner.cpp
│   ├── test_risk_scorer.cpp
│   └── test_safety_stock.cpp
├── demo/
│   ├── benchmark.cpp              # 50K item throughput benchmark
│   └── main.cpp                   # Full CLI report + JSON export
├── .github/
│   └── workflows/
│       └── build.yml              # CI: build + test on every push
└── data/
    ├── inventory.csv
    ├── shipments.csv
    └── suppliers.csv
```

---

## Architecture & design patterns

### Strategy — pluggable forecasting algorithms

`IForecaster` is a pure virtual interface. `SMAForecaster`, `EMAForecaster`,
and `HoltForecaster` are interchangeable implementations. The caller selects
the algorithm at runtime without depending on any concrete type:

```cpp
std::unique_ptr<IForecaster> algo = std::make_unique<HoltForecaster>(0.3, 0.2);
auto forecast = algo->forecast(history, 3);
```

New algorithms (e.g. Holt-Winters) can be added without touching existing code.

### Factory — stateless CSV loading

`CSVLoader` exposes only static methods and owns no state. It acts as a
factory that reads raw CSV rows and returns typed model objects, isolating
all parsing and coercion in one place.

### Separation of concerns — three clear layers

| Layer | Classes | Responsibility |
|---|---|---|
| **Data** | `Item`, `Supplier`, `Shipment`, `PlanningResult` | Pure data; no logic |
| **Ingestion** | `CSVLoader` | CSV → typed objects |
| **Planning** | `InventoryPlanner`, `EOQCalculator`, `SafetyStockCalculator`, `RiskScorer` | All algorithm logic |
| **Forecasting** | `IForecaster`, `SMAForecaster`, `EMAForecaster` | Demand projection |

---

## Algorithms implemented

### Inventory risk classification (`InventoryPlanner`)

```
days_until_stockout  = current_stock / daily_demand
effective_lead_time  = lead_time_days + delayed_days
lead_time_demand     = daily_demand × effective_lead_time

High   : days_until_stockout < effective_lead_time
Medium : current_stock ≤ reorder_point  (not High)
Low    : current_stock > reorder_point
```

### Economic Order Quantity (`EOQCalculator`)

```
EOQ = sqrt( 2 × D × S / H )

D = annual demand (units/year)
S = ordering cost per order  ($/order)
H = holding cost per unit per year  ($/unit/year)
```

### Safety stock (`SafetyStockCalculator`)

```
SS = Z × sqrt( avg_LT × σ_d² + avg_d² × σ_LT² )

Z      = inverse normal CDF at service level  (Abramowitz & Stegun approximation)
σ_d    = std dev of daily demand
avg_d  = mean daily demand
σ_LT   = std dev of lead time in days
```

### Simple Moving Average (`SMAForecaster`)

```
SMA_t = mean( x_{t-w}, …, x_{t-1} )   where w = window size
```

### Exponential Moving Average (`EMAForecaster`)

```
EMA_t = α × x_t + (1 − α) × EMA_{t−1}   where α ∈ (0, 1)
```

### Portfolio risk score (`RiskScorer`)

```
stockout_proximity = min(1, lead_time_demand / current_stock)
unreliability      = 1 − (reliability_score / 100)
composite_score    = 0.6 × stockout_proximity + 0.4 × unreliability
```

---

## Build instructions

**Requirements:**
- CMake 3.16+
- C++17-compatible compiler (MSVC 2019+, GCC 9+, Clang 10+)
- Internet access at configure time (CMake fetches Google Test automatically)

```bash
cmake -S . -B build
cmake --build build
```

On Windows with Visual Studio:
```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

---

## Run the demo

```bash
# From the project root (so data/ is resolved correctly)
./build/sca_demo          # Linux / macOS
build\Release\sca_demo    # Windows
```

---

## Run the tests

```bash
cd build
ctest --output-on-failure
```

Or run the test binary directly:
```bash
./build/tests/sca_tests          # Linux / macOS
build\tests\Release\sca_tests    # Windows
```

---

## Example output

```
=== Inventory Risk Report ===
Item        Stock   Days    Lead Dem    Risk      Recommendation
------------------------------------------------------------------------
Widget-A    120     12      84          Low       Reorder in 5 days
Widget-B    40      5       128         High      Reorder now - likely stockout before replenishment
Widget-C    200     8       175         High      Reorder now - likely stockout before replenishment
Widget-D    25      4       35          High      Reorder now - likely stockout before replenishment
Widget-E    18      3       60          High      Reorder now - likely stockout before replenishment

=== Economic Order Quantity (EOQ) ===
Assumptions: ordering cost = $50/order, holding cost = $2/unit/year

Widget-A    EOQ = 604.2 units  (annual demand = 3650)
Widget-B    EOQ = 542.2 units  (annual demand = 2920)
...

=== Portfolio Risk Ranking (highest risk first) ===
Item          Score     Status
----------------------------------
Widget-E      0.803     Critical
Widget-C      0.754     Critical
Widget-B      0.720     Critical
Widget-D      0.612     Elevated
Widget-A      0.214     Normal
```

---

## Technologies

| Tool / Feature | Purpose |
|---|---|
| **C++17** | Core language: `std::optional`, `if`-init, structured bindings |
| **CMake 3.16+** | Cross-platform build system |
| **Google Test 1.14** | Unit and parameterised testing (fetched via `FetchContent`) |
| **`std::optional`** | Nullable shipment reference without raw pointers |
| **Strategy pattern** | Swappable forecasting algorithms |
| **Rational approximation** | Portable inverse-normal CDF without external math libs |
