#define ASIO_STANDALONE
#define WEBSOCKETPP_ASIO_STANDALONE

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>
#include "TradeTick.hpp"

#include <functional>
#include <iostream>

using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_tls_client> bybit_client;

void run_bybit_stream(std::function<void(const TradeTick&)> callback) {
    bybit_client client;
    client.init_asio();

    client.set_tls_init_handler([](websocketpp::connection_hdl) {
        namespace ssl = websocketpp::lib::asio::ssl;
        auto ctx = websocketpp::lib::make_shared<ssl::context>(ssl::context::tlsv12_client);
        ctx->set_verify_mode(ssl::verify_none);
        return ctx;
    });

    client.set_open_handler([&client](websocketpp::connection_hdl hdl) {
        std::cout << "[Bybit]  Connected\n";
        json sub = {
            {"op", "subscribe"},
            {"args", { "publicTrade.BTCUSDT", "publicTrade.ETHUSDT" }}
        };
        client.send(hdl, sub.dump(), websocketpp::frame::opcode::text);
    });

    client.set_message_handler([callback](websocketpp::connection_hdl, bybit_client::message_ptr msg) {
        try {
            auto payload = json::parse(msg->get_payload());
            if (!payload.contains("data")) return;

            for (const auto& entry : payload["data"]) {
                TradeTick tick;
                tick.symbol = entry["s"].get<std::string>();

if (entry["p"].is_string())
    tick.price = std::stod(entry["p"].get<std::string>());
else
    tick.price = entry["p"].get<double>();

if (entry["v"].is_string())
    tick.quantity = std::stod(entry["v"].get<std::string>());
else
    tick.quantity = entry["v"].get<double>();

if (entry["T"].is_string())
    tick.timestamp = std::stoull(entry["T"].get<std::string>());
else
    tick.timestamp = entry["T"].get<std::uint64_t>();
            }
        } catch (const std::exception& e) {
            std::cerr << "[Bybit parse error] " << e.what() << "\n";
        }
    });

    websocketpp::lib::error_code ec;
    auto con = client.get_connection("wss://stream.bybit.com/v5/public/linear", ec);
    if (ec) {
        std::cerr << "[Bybit connect error] " << ec.message() << "\n";
        return;
    }

    client.connect(con);
    client.run();
}