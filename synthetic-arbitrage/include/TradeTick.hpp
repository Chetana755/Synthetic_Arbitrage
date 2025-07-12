#pragma once

#include <string>
#include <cstdint>

struct TradeTick {
    std::string symbol;
    double price;
    double quantity;
    std::uint64_t timestamp;
};