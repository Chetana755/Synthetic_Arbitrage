#pragma once
#include "../utils/funding_cache.hpp"

double compute_synthetic_price(double perp_price, const std::string& symbol, const FundingCache& funding);