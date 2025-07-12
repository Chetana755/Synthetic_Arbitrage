#define ASIO_STANDALONE
#define BOOST_ASIO_DISABLE_BOOST
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include "binance_funding_ws.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>

using json = nlohmann::json;

std::string ws_url(const std::string& sym) {
    std::string lower = sym;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return "wss://fstream.binance.com/ws/" + lower + "@funding@1s";
}

void start_binance_funding_ws(FundingCache& fundingCache, const std::string& symbol) {
    ws_client client;
    client.init_asio();

    client.set_message_handler([&](websocketpp::connection_hdl, ws_client::message_ptr msg) {
        try {
            auto j = json::parse(msg->get_payload());
            double rate = std::stod(j.value("r", "0"));
            int64_t timestamp = j.value("T", 0);
            fundingCache.update(symbol, rate, std::chrono::milliseconds(timestamp));
            std::cout << "[Funding] " << symbol << ": " << rate << " @ " << timestamp << "\n";
        } catch (...) {
            std::cerr << "[Funding] Parse error\n";
        }
    });

    websocketpp::lib::error_code ec;
    auto con = client.get_connection(ws_url(symbol), ec);
    if (ec) {
        std::cerr << "Connection error: " << ec.message() << "\n";
        return;
    }

    client.connect(con);
    client.run();
}