#pragma once
#include <fstream>
#include <string>
#include <cstdint>

class SyntheticSnapshotLogger {
public:
    SyntheticSnapshotLogger(const std::string& filename)
        : out(filename, std::ios::out) {
        out << "timestamp,ethbtc,btcusdt,real_ethusdt,synthetic_ethusdt,diff\n";
    }

    void write(std::uint64_t ts, double ethbtc, double btcusdt,
               double real_eth, double synthetic_eth) {
        double diff = synthetic_eth - real_eth;
        out << ts << "," << ethbtc << "," << btcusdt << ","
            << real_eth << "," << synthetic_eth << "," << diff << "\n";
    }

private:
    std::ofstream out;
};