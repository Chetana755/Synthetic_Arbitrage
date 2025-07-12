#pragma once
#include <unordered_map>
#include <string>
#include <mutex>
#include <chrono>

struct FundingInfo {
    double rate = 0.0;
    std::chrono::milliseconds nextFundingTime{0};
};

class FundingCache {
public:
    void update(const std::string& symbol, double rate, std::chrono::milliseconds next_time) {
        std::lock_guard<std::mutex> lock(mutex_);
        rates_[symbol] = {rate, next_time};
    }

    FundingInfo get(const std::string& symbol) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = rates_.find(symbol);
        return it != rates_.end() ? it->second : FundingInfo{};
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, FundingInfo> rates_;
};