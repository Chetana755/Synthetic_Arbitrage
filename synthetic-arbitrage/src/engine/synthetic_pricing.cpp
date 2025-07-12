#include "synthetic_pricing.hpp"

double compute_synthetic_price(double perp_price, const std::string& symbol, const FundingCache& funding) {
    auto info = funding.get(symbol);
    return perp_price * (1.0 - info.rate); // Adjust for funding (positive rate means cost to hold long)
}