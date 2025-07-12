#pragma once
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include "../utils/funding_cache.hpp"

using ws_client = websocketpp::client<websocketpp::config::asio_tls_client>;

void start_binance_funding_ws(FundingCache& fundingCache, const std::string& symbol);