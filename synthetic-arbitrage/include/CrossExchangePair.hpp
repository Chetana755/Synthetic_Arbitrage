#pragma once
#include <string>
#include <cstdint>

class CrossExchangePair {
public:
    CrossExchangePair(const std::string& symbol, double threshold)
        : symbol(symbol), threshold(threshold), binanceReady(false), bybitReady(false) {}

    void update(const std::string& venue, double price, std::uint64_t ts) {
        if (venue == "Binance") {
            binancePrice = price;
            binanceReady = true;
        } else if (venue == "Bybit") {
            bybitPrice = price;
            bybitReady = true;
        }
    }

    void printSpread() const;
    double getPrice(const std::string& venue) const;

private:
    std::string symbol;
    double threshold;

    double binancePrice = 0.0;
    double bybitPrice = 0.0;
    bool binanceReady;
    bool bybitReady;
};