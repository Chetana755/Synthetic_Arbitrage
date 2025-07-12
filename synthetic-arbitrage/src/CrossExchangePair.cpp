#include "CrossExchangePair.hpp"
#include <iostream>
#include <cmath>
#include <cstdint>

void CrossExchangePair::printSpread() const {
    if (!binanceReady || !bybitReady) return;

    double spread = binancePrice - bybitPrice;
    std::cout << "[Δ " << symbol << "] Binance: " << binancePrice
              << " | Bybit: " << bybitPrice
              << " | Δ = " << (spread > 0 ? "+" : "") << spread << "\n";

    if (std::abs(spread) >= threshold) {
        std::cout << "  [ALERT] Cross-exchange spread exceeds threshold: " << spread << "\n";
    }
}

double CrossExchangePair::getPrice(const std::string& venue) const {
    if (venue == "Binance" && binanceReady) return binancePrice;
    if (venue == "Bybit" && bybitReady) return bybitPrice;
    return 0.0;
}