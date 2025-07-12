#pragma once
#include <string>
#include <iostream>
#include <cmath>
#include <cstdint>
#include <optional>
#include <iomanip>
#include <chrono>

class SyntheticPerpEngine {
public:
    SyntheticPerpEngine(std::string _symbol, double _threshold)
        : symbol(std::move(_symbol)), threshold(_threshold) {}

    void updateSpot(double price) {
        spot = price;
        ready_spot = true;
    }

    void updateFunding(double rate, std::uint64_t next_ts) {
        funding_rate = rate;
        next_funding = next_ts;
        ready_funding = true;
    }

    void updateMark(double mark_price, std::uint64_t timestamp) {
        last_mark_price = mark_price;
        ready_mark = true;

        if (!ready_spot || !ready_funding || !next_funding) return;

        double hours = (*next_funding - timestamp) / 3600000.0;
        double synthetic = *spot + (*funding_rate * (*spot) * hours);
        double delta = mark_price - synthetic;

        std::cout << "\n=== Synthetic Perp Monitor ===\n"
                  << " Spot Price      : " << *spot << "\n"
                  << " Funding Rate    : " << *funding_rate << "\n"
                  << " Hours Remaining : " << hours << "\n"
                  << " Synthetic Perp  : " << synthetic << "\n"
                  << " Real Mark Price : " << mark_price << "\n"
                  << " Δ (Real - Synth): " << delta << "\n";

        static std::uint64_t last_alert = 0;
        if (std::abs(delta) > threshold &&
            timestamp - last_alert > 5000)
        {
            last_alert = timestamp;
            std::cout << "\033[1;35m[ALERT]\033[0m Synthetic Perp mispricing detected! Δ = "
                      << std::fixed << std::setprecision(6)
                      << delta << " exceeds ±" << threshold << "\n";
        }
    }

    double getSynthetic() const {
        if (!ready_spot || !ready_funding || !next_funding) return 0.0;

        double now_ms = getTimeNowMs();
        double hours = (*next_funding - now_ms) / 3600000.0;
        return *spot + (*funding_rate * (*spot) * hours);
    }

    double getMark() const {
        return last_mark_price;
    }

    double getFundingRate() const {
        return funding_rate.value_or(0.0);
    }

    double getSpot() const {
        return spot.value_or(0.0);
    }

    bool isReady() const {
        return ready_spot && ready_funding && ready_mark && next_funding.has_value();
    }

private:
    std::string symbol;
    double threshold;

    std::optional<double> spot;
    std::optional<double> funding_rate;
    std::optional<std::uint64_t> next_funding;

    double last_mark_price = 0.0;

    bool ready_spot = false;
    bool ready_funding = false;
    bool ready_mark = false;

    double getTimeNowMs() const {
        return static_cast<double>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
    }
};