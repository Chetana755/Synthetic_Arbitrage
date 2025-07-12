#pragma once
#include <fstream>
#include <string>
#include <iomanip>
#include <ctime>

class ArbLogger {
public:
    ArbLogger(const std::string& filename = "arbitrage_log.csv") {
        file.open(filename, std::ios::app);
        if (file.tellp() == 0) {
            file << "timestamp,type,real_price,synthetic_price,delta,net_pnl,count,cumulative_pnl,direction,size\n";
        }
    }

    void log(const std::string& symbol, double real, double synth, double delta) {
        const auto now = std::time(nullptr);
        file << now << "," << symbol << ","
             << std::fixed << std::setprecision(8)
             << real << "," << synth << "," << delta << ",,,,," << "\n";
    }

    void logSimulatedTrade(double synthetic, double real, double net_pnl,
                           std::uint64_t ts, int signal_count, double total_pnl,
                           const std::string& direction, double size) {
        file << ts << ",SimulatedTrade,"
             << std::fixed << std::setprecision(8)
             << real << "," << synthetic << "," << (synthetic - real)
             << "," << net_pnl << "," << signal_count << "," << total_pnl
             << "," << direction << "," << size << "\n";
    }

private:
    std::ofstream file;
};