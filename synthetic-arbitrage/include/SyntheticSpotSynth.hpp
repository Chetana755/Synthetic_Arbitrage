#pragma once
#include <string>
#include <cstdint>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "MispricingLogger.hpp"
#include "SyntheticSnapshotLogger.hpp"
#include "ArbOpportunityEngine.hpp"
#include "MispricingBuffer.hpp"
#include "MockPnLEngine.hpp"

class SyntheticSpotSynth {
public:
    SyntheticSpotSynth(const std::string& label) : label(label) {}

    void update(const std::string& symbol, double price, std::uint64_t timestamp) {
        if (symbol == "ETHBTC") {
            ethbtc = price;
            ts_ethbtc = timestamp;
            ready_ethbtc = true;
        } else if (symbol == "BTCUSDT") {
            btcusdt = price;
            ts_btcusdt = timestamp;
            ready_btcusdt = true;
        }

        const std::uint64_t max_age = 1500; // milliseconds
        bool synced_ticks = ready_ethbtc && ready_btcusdt &&
                            std::abs((int64_t)(ts_ethbtc - ts_btcusdt)) <= max_age;

        if (synced_ticks) {
            synthetic_price = ethbtc * btcusdt;
            last_ts = timestamp;

            std::cout << "[Synthetic] " << label << " = " << synthetic_price
                      << " @ " << last_ts << "\n";

            if (mispricing_logger)
                mispricing_logger->log(synthetic_price, real_ethusdt, timestamp);
        }
    }

    void compareToReal(double real_price, std::uint64_t timestamp) {
        if (!ready_ethbtc || !ready_btcusdt) {
            std::cerr << "[warn] Synthetic pricing unavailable — missing feed(s)\n";
            return;
        }

        synthetic_price = ethbtc * btcusdt;
        real_ethusdt = real_price;
        double delta = synthetic_price - real_price;
        buffer.add(delta);

        std::cout << "\n=== Synthetic Spread Monitor ===\n"
                  << " BTCUSDT: " << btcusdt << "\n"
                  << " ETHBTC : " << ethbtc << "\n"
                  << " ETHUSDT: " << real_ethusdt << "\n"
                  << " -------------------------------\n"
                  << " Synthetic ETHUSDT = " << synthetic_price << "\n"
                  << "  (Real - Synthetic) = " << (real_ethusdt - synthetic_price) << "\n";

        if (mispricing_logger)
            mispricing_logger->log(synthetic_price, real_price, timestamp);

        if (snapshot_logger)
            snapshot_logger->write(timestamp, ethbtc, btcusdt, real_price, synthetic_price);

        if (arb_engine && buffer.isPersistentSpike(spike_threshold))
            arb_engine->evaluate(synthetic_price, real_price, timestamp);
        
        if (arb_engine &&
            std::abs(delta) >= trigger_threshold &&
            arb_engine->shouldTrigger("ETHUSDT", timestamp)) {

            std::string direction = delta > 0 ? "SELL" : "BUY";

            std::cout << "[Synthetic] ETHUSDT (Synthetic) = " << synthetic_price
                      << " @ " << timestamp << "\n";
            std::cout << "  [Mispricing Alert] ETHUSDT: Δ = " << std::fixed
                      << std::setprecision(6) << delta << "\n";

            double slippage_rate = 0.0002;
            double effective_entry = direction == "BUY" ? synthetic_price * (1 + slippage_rate)
                                                        : synthetic_price * (1 - slippage_rate);
            double effective_exit  = direction == "BUY" ? real_price * (1 - slippage_rate)
                                                        : real_price * (1 + slippage_rate);

            if (pnl_tracker)
                pnl_tracker->recordTrade("ETHUSDT", effective_entry, effective_exit, timestamp, direction);
        }
    }

    double getSynthetic() const {
        return (ready_ethbtc && ready_btcusdt) ? (ethbtc * btcusdt) : 0.0;
    }

    double getAverageMispricing() const {
        return buffer.average();
    }

    void attachLogger(MispricingLogger* l) { mispricing_logger = l; }
    void attachSnapshotLogger(SyntheticSnapshotLogger* l) { snapshot_logger = l; }
    void attachArbEngine(ArbOpportunityEngine* e) { arb_engine = e; }
    void attachMockPnL(MockPnLEngine* p) { pnl_tracker = p; }

private:
    std::string label;
    double ethbtc = 0.0;
    double btcusdt = 0.0;
    double synthetic_price = 0.0;
    double real_ethusdt = 0.0;
    std::uint64_t last_ts = 0;
    std::uint64_t ts_ethbtc = 0;
    std::uint64_t ts_btcusdt = 0;

    MispricingLogger* mispricing_logger = nullptr;
    SyntheticSnapshotLogger* snapshot_logger = nullptr;
    ArbOpportunityEngine* arb_engine = nullptr;
    MockPnLEngine* pnl_tracker = nullptr;

    MispricingBuffer buffer{2};
    bool ready_ethbtc = false;
    bool ready_btcusdt = false;

    const double trigger_threshold = 0.1;
    const double spike_threshold = 0.1;
};