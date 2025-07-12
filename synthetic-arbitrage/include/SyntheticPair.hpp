#pragma once
#include "TradeTick.hpp"
#include "ArbLogger.hpp"
static ArbLogger logger;
#include <string>
#include <optional>
#include <iostream>
#include <cmath>
static std::uint64_t last_alert_time = 0;
class SyntheticPair {
public:
    SyntheticPair(std::string _base, double _threshold)
        : base(std::move(_base)), threshold(_threshold) {}

    // Update latest prices for each leg
    void update(const TradeTick& tick) {
        if (tick.symbol == "BTCUSDT") btcusdt = tick;
        else if (tick.symbol == "ETHBTC") ethbtc = tick;
        else if (tick.symbol == "ETHUSDT") ethusdt = tick;

        if (is_ready()) evaluate();
    }

private:
    std::string base;
    double threshold;

    std::optional<TradeTick> btcusdt, ethbtc, ethusdt;

    bool is_ready() const {
        return btcusdt && ethbtc && ethusdt;
    }

    void evaluate() {
    double synthetic = btcusdt->price * ethbtc->price;
    double actual = ethusdt->price;
    double delta = actual - synthetic;

    std::cout << "\n=== Synthetic Spread Monitor ===\n"
              << " BTCUSDT: " << btcusdt->price << "\n"
              << " ETHBTC : " << ethbtc->price << "\n"
              << " ETHUSDT: " << actual << "\n"
              << " -------------------------------\n"
              << " Synthetic ETHUSDT = " << synthetic << "\n"
              << "  (Real - Synthetic) = " << delta << "\n";

   
#include <iomanip>  
static std::uint64_t last_alert_time = 0;



if (std::abs(delta) > threshold &&
    ethusdt->timestamp - last_alert_time > 5000) {
    last_alert_time = ethusdt->timestamp;
    std::cout << "\033[1;31m"  
              << " [ALERT] Arbitrage Opportunity! Δ = "
              << std::fixed << std::setprecision(6)
              << delta
              << " exceeds ±" << threshold
              << "\033[0m"     
              << "\n";
 logger.log(base, actual, synthetic, delta);
}
}
};