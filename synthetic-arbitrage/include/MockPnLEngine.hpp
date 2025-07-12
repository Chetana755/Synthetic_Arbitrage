#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <cmath>

struct SimulatedTrade {
    std::string symbol;
    double entry_price;
    double exit_price;
    std::string direction;
    double size;
    double pnl;
    std::uint64_t timestamp;
};

class ArbLogger; // Forward declaration

class MockPnLEngine {
public:
    MockPnLEngine(double max_exposure = 5000.0)
        : max_exposure(max_exposure) {}

    void recordTrade(const std::string& symbol,
                     double synthetic_price,
                     double real_price,
                     std::uint64_t timestamp,
                     const std::string& direction) {
        
        double spread = std::abs(real_price - synthetic_price);
        double edge = spread / real_price;
        double size = std::min(edge * 100.0, max_exposure);
        
        double pnl = (direction == "BUY")
            ? (real_price - synthetic_price) * (size / real_price)
            : (synthetic_price - real_price) * (size / real_price);
        
        SimulatedTrade trade = {
            symbol,
            synthetic_price,
            real_price,
            direction,
            size,
            pnl,
            timestamp
        };

        trades.push_back(trade);
        total_pnl += pnl;
        max_observed_exposure = std::max(max_observed_exposure, size);
        
        if (external_log && external_log->is_open()) {
            *external_log << trade.timestamp << "," << trade.symbol << "," << trade.direction << ","
                          << std::fixed << std::setprecision(8)
                          << trade.entry_price << "," << trade.exit_price << ","
                          << trade.size << "," << trade.pnl << "\n";
        }

        if (arb_logger) {
            arb_logger->logSimulatedTrade(
                trade.entry_price,
                trade.exit_price,
                trade.pnl,
                trade.timestamp,
                trades.size(),
                total_pnl,
                trade.direction,
                trade.size
            );
        }
        pnl_series.push_back(pnl);
    }

    double getPnL() const { return total_pnl; }
    std::size_t getCount() const { return trades.size(); }
    double getMaxExposure() const { return max_observed_exposure; }

    void attachLogger(std::ofstream* stream) {
        external_log = stream;
    }

    void attachLogger(class ArbLogger* logger) {
        arb_logger = logger;
    }
    std::vector<double> pnl_series;

double rollingStdDev(std::size_t window = 10) const {
    if (pnl_series.size() < window) return 0.0;
    double mean = std::accumulate(pnl_series.end() - window, pnl_series.end(), 0.0) / window;
    double sum = 0.0;
    for (std::size_t i = pnl_series.size() - window; i < pnl_series.size(); ++i)
        sum += std::pow(pnl_series[i] - mean, 2);
    return std::sqrt(sum / window);
}

private:
    std::vector<SimulatedTrade> trades;
    double total_pnl = 0.0;
    double max_observed_exposure = 0.0;
    double max_exposure;

    std::ofstream* external_log = nullptr;
    class ArbLogger* arb_logger = nullptr;
};