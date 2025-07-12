#pragma once
#include <unordered_map>
#include <string>
#include <cstdint>
#include <iostream> // Needed for output during evaluation
#include "MockPnLEngine.hpp"
class ArbOpportunityEngine {
public:
    // cooldown_ms: time between signals per symbol (default 5000 ms)
    ArbOpportunityEngine(std::uint64_t cooldown_ms = 5000)
        : cooldown(cooldown_ms) {}

    // Returns true if signal can fire (i.e., cooldown passed)
    bool shouldTrigger(const std::string& symbol, std::uint64_t timestamp) {
        auto it = last_trigger.find(symbol);
        if (it == last_trigger.end() || timestamp - it->second > cooldown) {
            last_trigger[symbol] = timestamp;
            return true;
        }
        return false;
    }

    // Resets signal timestamp manually (optional)
    void reset(const std::string& symbol) {
        last_trigger.erase(symbol);
    }

    // NEW: Evaluate a potential arbitrage signal
    void evaluate(double synthetic_price, double real_price, std::uint64_t timestamp) {
        double delta = synthetic_price - real_price;
        std::cout << "[ArbEngine::Evaluate] Δ = " << delta << " @ " << timestamp << "\n";
        // Additional logic can be added later (e.g. triggering alerts, logging signals)
    }
    void attachMockPnL(MockPnLEngine* p) { mockPnL = p; }

private:
    std::uint64_t cooldown; // cooldown duration in milliseconds
    std::unordered_map<std::string, std::uint64_t> last_trigger; 
    MockPnLEngine* mockPnL = nullptr;
};