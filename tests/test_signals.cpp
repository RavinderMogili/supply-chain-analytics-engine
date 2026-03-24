#include <gtest/gtest.h>
#include "algorithms/SignalAwareForecaster.h"
#include "algorithms/SMAForecaster.h"
#include "algorithms/EMAForecaster.h"
#include "algorithms/HoltForecaster.h"

#include <memory>
#include <cmath>

using namespace sca;

// ── Helpers ───────────────────────────────────────────────────────────────────

namespace {

const std::vector<double> history = {80, 85, 90, 88, 92, 95, 91, 89};

std::unique_ptr<SMAForecaster> make_sma() {
    return std::make_unique<SMAForecaster>(3);
}

DemandSignal additive(double v, const std::string& src = "POS") {
    return DemandSignal{src, v, DemandSignal::Type::Additive, "test add"};
}

DemandSignal multiplicative(double v, const std::string& src = "MARKET") {
    return DemandSignal{src, v, DemandSignal::Type::Multiplicative, "test mult"};
}

} // anonymous namespace

// ── Construction ──────────────────────────────────────────────────────────────

TEST(SignalAwareForecaster, NullBaseThrows) {
    EXPECT_THROW(
        SignalAwareForecaster(nullptr),
        std::invalid_argument);
}

TEST(SignalAwareForecaster, ConstructsWithNoSignals) {
    SignalAwareForecaster f(make_sma());
    EXPECT_TRUE(f.signals().empty());
}

TEST(SignalAwareForecaster, ConstructsWithInitialSignals) {
    std::vector<DemandSignal> sigs = {additive(10), multiplicative(1.1)};
    SignalAwareForecaster f(make_sma(), std::move(sigs));
    EXPECT_EQ(f.signals().size(), 2u);
}

// ── No signals: output equals base ───────────────────────────────────────────

TEST(SignalAwareForecaster, NoSignalsMatchesBaseOutput) {
    SMAForecaster base_copy(3);
    SignalAwareForecaster f(make_sma());

    auto base_result = base_copy.forecast(history, 3);
    auto wrapped     = f.forecast(history, 3);

    ASSERT_EQ(base_result.size(), wrapped.size());
    for (std::size_t i = 0; i < base_result.size(); ++i)
        EXPECT_NEAR(wrapped[i], base_result[i], 1e-9);
}

// ── Additive signal ───────────────────────────────────────────────────────────

TEST(SignalAwareForecaster, AdditiveSingleSignalShiftsAllPeriods) {
    SMAForecaster base_copy(3);
    SignalAwareForecaster f(make_sma());
    f.add_signal(additive(20.0));

    auto base_result = base_copy.forecast(history, 3);
    auto result      = f.forecast(history, 3);

    ASSERT_EQ(result.size(), 3u);
    for (std::size_t i = 0; i < result.size(); ++i)
        EXPECT_NEAR(result[i], base_result[i] + 20.0, 1e-9);
}

TEST(SignalAwareForecaster, MultipleAdditiveSignalsAreAccumulated) {
    SMAForecaster base_copy(3);
    SignalAwareForecaster f(make_sma());
    f.add_signal(additive(10.0, "POS"));
    f.add_signal(additive(5.0,  "PO_ACTIVITY"));

    auto base_result = base_copy.forecast(history, 3);
    auto result      = f.forecast(history, 3);

    for (std::size_t i = 0; i < result.size(); ++i)
        EXPECT_NEAR(result[i], base_result[i] + 15.0, 1e-9);
}

TEST(SignalAwareForecaster, NegativeAdditiveClampedToZero) {
    // Huge negative signal → forecast clamped to 0, not negative
    SignalAwareForecaster f(make_sma());
    f.add_signal(additive(-99999.0));

    auto result = f.forecast(history, 3);
    for (double v : result)
        EXPECT_GE(v, 0.0);
}

// ── Multiplicative signal ─────────────────────────────────────────────────────

TEST(SignalAwareForecaster, MultiplicativeScalesAllPeriods) {
    SMAForecaster base_copy(3);
    SignalAwareForecaster f(make_sma());
    f.add_signal(multiplicative(1.15)); // +15% competitor stockout

    auto base_result = base_copy.forecast(history, 3);
    auto result      = f.forecast(history, 3);

    for (std::size_t i = 0; i < result.size(); ++i)
        EXPECT_NEAR(result[i], base_result[i] * 1.15, 1e-9);
}

TEST(SignalAwareForecaster, MultipleMultiplicativeSignalsAreMultiplied) {
    SMAForecaster base_copy(3);
    SignalAwareForecaster f(make_sma());
    f.add_signal(multiplicative(1.10, "MARKET"));
    f.add_signal(multiplicative(1.05, "PROMO"));

    auto base_result = base_copy.forecast(history, 3);
    auto result      = f.forecast(history, 3);

    for (std::size_t i = 0; i < result.size(); ++i)
        EXPECT_NEAR(result[i], base_result[i] * 1.10 * 1.05, 1e-6);
}

// ── Combined additive + multiplicative ────────────────────────────────────────

TEST(SignalAwareForecaster, AdditiveAppliedBeforeMultiplicative) {
    // Order: (base + additive) * multiplicative
    SMAForecaster base_copy(3);
    SignalAwareForecaster f(make_sma());
    f.add_signal(additive(10.0));
    f.add_signal(multiplicative(2.0));

    auto base_result = base_copy.forecast(history, 3);
    auto result      = f.forecast(history, 3);

    for (std::size_t i = 0; i < result.size(); ++i)
        EXPECT_NEAR(result[i], (base_result[i] + 10.0) * 2.0, 1e-9);
}

// ── add_signal / clear_signals ────────────────────────────────────────────────

TEST(SignalAwareForecaster, AddSignalIncreaseCount) {
    SignalAwareForecaster f(make_sma());
    EXPECT_EQ(f.signals().size(), 0u);
    f.add_signal(additive(5.0));
    EXPECT_EQ(f.signals().size(), 1u);
    f.add_signal(multiplicative(1.1));
    EXPECT_EQ(f.signals().size(), 2u);
}

TEST(SignalAwareForecaster, ClearSignalsResetsToBase) {
    SMAForecaster base_copy(3);
    SignalAwareForecaster f(make_sma());
    f.add_signal(additive(50.0));

    f.clear_signals();
    EXPECT_TRUE(f.signals().empty());

    auto base_result = base_copy.forecast(history, 3);
    auto result      = f.forecast(history, 3);

    for (std::size_t i = 0; i < result.size(); ++i)
        EXPECT_NEAR(result[i], base_result[i], 1e-9);
}

// ── Works with all base forecasters ──────────────────────────────────────────

TEST(SignalAwareForecaster, WorksWithEMABase) {
    SignalAwareForecaster f(std::make_unique<EMAForecaster>(0.3));
    f.add_signal(additive(10.0, "POS"));
    auto result = f.forecast(history, 3);
    EXPECT_EQ(result.size(), 3u);
    for (double v : result) EXPECT_GT(v, 0.0);
}

TEST(SignalAwareForecaster, WorksWithHoltBase) {
    SignalAwareForecaster f(std::make_unique<HoltForecaster>(0.3, 0.2));
    f.add_signal(multiplicative(1.20, "MARKET")); // competitor out-of-stock +20%
    auto result = f.forecast(history, 3);
    EXPECT_EQ(result.size(), 3u);
    for (double v : result) EXPECT_GT(v, 0.0);
}

// ── name() ────────────────────────────────────────────────────────────────────

TEST(SignalAwareForecaster, NameContainsBaseNameAndSources) {
    SignalAwareForecaster f(make_sma());
    f.add_signal(additive(5.0, "POS"));
    f.add_signal(multiplicative(1.1, "MARKET"));

    const auto n = f.name();
    EXPECT_NE(n.find("SignalAware"), std::string::npos);
    EXPECT_NE(n.find("POS"),         std::string::npos);
    EXPECT_NE(n.find("MARKET"),      std::string::npos);
}

TEST(SignalAwareForecaster, NameWithNoSignalsHasNoSources) {
    SignalAwareForecaster f(make_sma());
    const auto n = f.name();
    EXPECT_NE(n.find("SignalAware"), std::string::npos);
    EXPECT_EQ(n.find("["),           std::string::npos);
}

// ── Empty history / zero periods ─────────────────────────────────────────────

TEST(SignalAwareForecaster, EmptyHistoryReturnsEmpty) {
    SignalAwareForecaster f(make_sma());
    f.add_signal(additive(10.0));
    EXPECT_TRUE(f.forecast({}, 3).empty());
}

TEST(SignalAwareForecaster, ZeroPeriodsReturnsEmpty) {
    SignalAwareForecaster f(make_sma());
    f.add_signal(additive(10.0));
    EXPECT_TRUE(f.forecast(history, 0).empty());
}
