#pragma once
#include <string>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <cmath>

class MispricingLogger {
public:
    MispricingLogger(const std::string& name, double threshold = 0.2)
        : name(name), threshold(threshold), logfile("mispricing_" + name + ".csv") {
        logfile << "timestamp,synthetic_price,real_price,diff\n";
    }

    void log(double synthetic_price, double real_price, std::uint64_t ts) {
        double diff = synthetic_price - real_price;
        logfile << ts << "," << synthetic_price << "," << real_price << "," << diff << "\n";

        if (std::abs(diff) >= threshold) {
            std::cout << "  [Mispricing Alert] " << name << ": ∆ = " << diff << "\n";
        }
    }

private:
    std::string name;
    double threshold;
    std::ofstream logfile;
};