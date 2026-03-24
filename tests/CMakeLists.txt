add_executable(sca_tests
    test_forecasters.cpp
    test_eoq.cpp
    test_safety_stock.cpp
    test_planner.cpp
    test_risk_scorer.cpp
    test_cache.cpp
    test_holt.cpp
    test_simulation.cpp
    test_signals.cpp
)

target_link_libraries(sca_tests
    PRIVATE
        sca_lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(sca_tests)
