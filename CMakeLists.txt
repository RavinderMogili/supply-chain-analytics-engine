cmake_minimum_required(VERSION 3.16)
project(SupplyChainAnalyticsEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Main library ──────────────────────────────────────────────────────────────
add_library(sca_lib
    src/algorithms/SMAForecaster.cpp
    src/algorithms/EMAForecaster.cpp
    src/algorithms/HoltForecaster.cpp
    src/algorithms/SignalAwareForecaster.cpp
    src/planner/EOQCalculator.cpp
    src/planner/SafetyStockCalculator.cpp
    src/planner/InventoryPlanner.cpp
    src/planner/RiskScorer.cpp
    src/planner/PlannerCache.cpp
    src/planner/BatchPlanner.cpp
    src/planner/PlannerPipeline.cpp
    src/loader/CSVLoader.cpp
    src/loader/JSONExporter.cpp
    src/simulation/SupplyChainSimulator.cpp
    src/simulation/DisruptionAnalyzer.cpp
    src/simulation/ScenarioRunner.cpp
)

target_include_directories(sca_lib PUBLIC include)

find_package(Threads REQUIRED)
target_link_libraries(sca_lib PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(sca_lib PRIVATE /W4)
else()
    target_compile_options(sca_lib PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ── Demo executable ───────────────────────────────────────────────────────────
add_executable(sca_demo demo/main.cpp)
target_link_libraries(sca_demo PRIVATE sca_lib)

# ── Benchmark executable ─────────────────────────────────────────────────────
add_executable(sca_benchmark demo/benchmark.cpp)
target_link_libraries(sca_benchmark PRIVATE sca_lib)

# ── Google Test (fetched at configure time) ───────────────────────────────────
include(FetchContent)

# Required on Windows/MSVC: link against shared CRT
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_subdirectory(tests)
