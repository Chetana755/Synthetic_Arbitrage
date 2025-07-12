#define ASIO_STANDALONE
#define WEBSOCKETPP_STD_THREAD
#define WEBSOCKETPP_ASIO_STANDALONE
#define WEBSOCKETPP_IO_SERVICE_ASIO asio::io_service

#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <nlohmann/json.hpp>

#include "TradeTick.hpp"
#include "SyntheticPair.hpp"
#include "SyntheticPerpEngine.hpp"
#include "CrossExchangePair.hpp"
#include "bybit_client.hpp"
#include "SyntheticSpotSynth.hpp"
#include "MispricingLogger.hpp"
#include "SyntheticSnapshotLogger.hpp"
#include "MockPnLEngine.hpp"
#include "OptionPricer.hpp"

#include <iostream>
#include <string>
#include <cstdint>
#include <unordered_map>
#include <deque>
#include <chrono>
#include <thread>
#include <functional>
#include <csignal>
#include <atomic>
#include <mutex>
#include <fstream>
CrossExchangePair ethCross("ETHUSDT", 0.2);
    SyntheticPair arb("ETHUSDT", 1.0);
    SyntheticPerpEngine perp("BTCUSDT", 1.0);
    SyntheticSpotSynth ethSynth("ETHUSDT (Synthetic)");
    SyntheticSnapshotLogger ethSnapshotLogger("eth_synthetic_log.csv");
    MockPnLEngine pnlTracker;
    MispricingLogger ethLogger("ETHUSDT", 0.2);
    ArbOpportunityEngine ethArbEngine(0.2);
    ArbLogger arbLogger("arbitrage_log.csv");
using json = nlohmann::json;
typedef websocketpp::client<websocketpp::config::asio_tls_client> ws_client;

std::mutex log_mutex;
constexpr std::size_t max_buffer_size = 20;
std::atomic<bool> running = true;
std::unordered_map<std::string, std::deque<TradeTick>> trade_buffers;

void signal_handler(int) { running = false; }

void on_message(ws_client* c, websocketpp::connection_hdl, ws_client::message_ptr msg) {
    try {
        auto payload = json::parse(msg->get_payload());
        auto data = payload.contains("data") ? payload["data"] : payload;
        std::string stream = payload.value("stream", "");

        TradeTick tick;

        if (payload.contains("data") && payload["data"].is_array()) {
            for (const auto& entry : payload["data"]) {
                tick.symbol    = entry["s"];
                tick.price     = entry["p"].is_string() ? std::stod(entry["p"].get<std::string>()) : entry["p"].get<double>();
                tick.quantity  = entry["v"].is_string() ? std::stod(entry["v"].get<std::string>()) : entry["v"].get<double>();
                tick.timestamp = entry["T"].is_string() ? std::stoull(entry["T"].get<std::string>()) : entry["T"].get<std::uint64_t>();

                {
                    std::lock_guard<std::mutex> lock(log_mutex);
                    std::cout << "[tick] " << tick.symbol << " " << tick.price
                              << " x " << tick.quantity << " @ " << tick.timestamp << "\n";
                }

                // 🔄 Update engines
                if (tick.symbol == "BTCUSDT") {
                    perp.updateSpot(tick.price);
                    ethSynth.update("BTCUSDT", tick.price, tick.timestamp);
                }
                if (tick.symbol == "ETHBTC") {
                    ethSynth.update("ETHBTC", tick.price, tick.timestamp);
                }
                if (tick.symbol == "ETHUSDT") {
                    ethCross.update("Binance", tick.price, tick.timestamp);
                }

                trade_buffers[tick.symbol].push_back(tick);
                if (trade_buffers[tick.symbol].size() > max_buffer_size)
                    trade_buffers[tick.symbol].pop_front();
            }
        } else {
            tick.symbol    = data["s"];
            tick.price     = data["p"].is_string() ? std::stod(data["p"].get<std::string>()) : data["p"].get<double>();
            tick.quantity  = data["q"].is_string() ? std::stod(data["q"].get<std::string>()) : data["q"].get<double>();
            tick.timestamp = data["T"].is_string() ? std::stoull(data["T"].get<std::string>()) : data["T"].get<std::uint64_t>();

            if (stream == "btcusdt@fundingRate") {
                double fundingRate = data["r"].is_string() ? std::stod(data["r"].get<std::string>()) : data["r"].get<double>();
                std::uint64_t ts = data["T"].is_string() ? std::stoull(data["T"].get<std::string>()) : data["T"].get<std::uint64_t>();
                perp.updateFunding(fundingRate, ts);
                std::cout << "[Funding] BTCUSDT = " << fundingRate << " @ " << ts << "\n";

            }

            if (stream == "btcusdt@markPrice") {
                double markPrice = data["p"].is_string() ? std::stod(data["p"].get<std::string>()) : data["p"].get<double>();
                std::uint64_t ts = data["T"].is_string() ? std::stoull(data["T"].get<std::string>()) : data["T"].get<std::uint64_t>();
                perp.updateMark(markPrice, ts);
                    std::cout << "[Mark] BTCUSDT = " << markPrice << " @ " << ts << "\n";

            }

            {
                std::lock_guard<std::mutex> lock(log_mutex);
                std::cout << "[tick] " << tick.symbol << " " << tick.price
                          << " x " << tick.quantity << " @ " << tick.timestamp << "\n";
            }

            // 🔄 Update engines
            if (tick.symbol == "BTCUSDT") {
                perp.updateSpot(tick.price);
                ethSynth.update("BTCUSDT", tick.price, tick.timestamp);
            }
            if (tick.symbol == "ETHBTC") {
                ethSynth.update("ETHBTC", tick.price, tick.timestamp);
            }
            if (tick.symbol == "ETHUSDT") {
                ethCross.update("Binance", tick.price, tick.timestamp);
                ethSynth.compareToReal(tick.price, tick.timestamp);
            }

            trade_buffers[tick.symbol].push_back(tick);
            if (trade_buffers[tick.symbol].size() > max_buffer_size)
                trade_buffers[tick.symbol].pop_front();
        }
    } catch (const std::exception& e) {
        std::cerr << "[parse error] " << e.what() << "\n";
    }
}

int main() {
    // Create CSV file and write header for arbitrage logs
    std::ofstream log_file("arbitrage_log.csv");
    log_file << "timestamp,symbol,direction,entry_price,exit_price,size,pnl\n";

    // Attach core modules to synthetic strategy engine
    ethSynth.attachSnapshotLogger(&ethSnapshotLogger);
    ethSynth.attachLogger(&ethLogger);
    ethSynth.attachArbEngine(&ethArbEngine);

    // Chain logging and PnL modules for triggered trades
    ethArbEngine.attachMockPnL(&pnlTracker);
    pnlTracker.attachLogger(&log_file);
    pnlTracker.attachLogger(&arbLogger);

    // Register SIGINT handler to cleanly exit
    std::signal(SIGINT, signal_handler);

    // Thread to periodically display engine diagnostics
    std::thread stats_overlay([&] {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(2));

            double real = ethCross.getPrice("Binance");
            double synth = ethSynth.getSynthetic();
            double avgDelta = ethSynth.getAverageMispricing();
            double fundingAdjusted = perp.getSynthetic();

            std::lock_guard<std::mutex> lock(log_mutex);
            std::cout << "\n=== Arbitrage Engine Snapshot ===\n";
            std::cout << " Synthetic ETHUSDT        : " << synth << "\n";
            std::cout << " BTC Funding-Adj Synthetic: " << fundingAdjusted << "\n";
            std::cout << " Real ETHUSDT             : " << real << "\n";
            std::cout << " Spread                   : " << synth - real << "\n";
            std::cout << " Rolling Avg              : " << avgDelta << "\n";
            std::cout << " BTC Mark Price           : " << perp.getMark() << "\n";
            std::cout << " Funding Rate             : " << perp.getFundingRate() << "\n";
            std::cout << " Signals Triggered        : " << pnlTracker.getCount() << "\n";
            std::cout << " Simulated PnL            : " << pnlTracker.getPnL() << "\n";
            std::cout << "===============================\n";
        }
    });

    // Thread to consume Bybit feed and update price streams
    std::thread bybit_thread([&] {
        run_bybit_stream([&](const TradeTick& tick) {
            if (tick.symbol == "ETHUSDT") {
                ethCross.update("Bybit", tick.price, tick.timestamp);
                ethSynth.compareToReal(tick.price, tick.timestamp);
            }
            if (tick.symbol == "BTCUSDT") {
                perp.updateMark(tick.price, tick.timestamp);
            }
        });
    });

    // Thread to monitor real-time spread between Binance and Bybit
    std::thread spread_monitor([&] {
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(3));

            double bpx = ethCross.getPrice("Binance");
            double ypx = ethCross.getPrice("Bybit");

            if (bpx > 0 && ypx > 0) {
                double diff = bpx - ypx;
                std::cout << "[Δ ETHUSDT] Binance: " << bpx
                          << " | Bybit: " << ypx
                          << " | Δ = " << (diff > 0 ? "+" : "") << diff << "\n";

                if (std::abs(diff) >= 0.2)
                    std::cout << "  [ALERT] ETHUSDT spread exceeds 20 cents\n";
            }
        }
    });

    // Log option pricing sample using Black-Scholes model
    std::cout << "[Options] Synthetic Call Price = " 
              << OptionPricer::blackScholesCall(2800, 2750, 0.07, 0.01, 0.6)
              << "\n";

    // Binance WebSocket URL with composite streams
    const std::string ws_url =
        "wss://stream.binance.com:9443/stream?streams="
        "btcusdt@trade/ethusdt@trade/ethbtc@trade/"
        "btcusdt@fundingRate/btcusdt@markPrice";

reconnect:
    ws_client client;

    try {
        client.clear_access_channels(websocketpp::log::alevel::all);
        client.init_asio();

        // TLS initialization for WebSocket connection (without peer verification)
        client.set_tls_init_handler([](websocketpp::connection_hdl) {
            namespace ssl = websocketpp::lib::asio::ssl;
            auto ctx = websocketpp::lib::make_shared<ssl::context>(ssl::context::tlsv12_client);
            ctx->set_verify_mode(ssl::verify_none);
            return ctx;
        });

        // Bind message handler to incoming WebSocket ticks
        client.set_message_handler(std::bind(&on_message, &client,
                                             std::placeholders::_1, std::placeholders::_2));

        // On unexpected close, force reconnect logic via exit code
        client.set_close_handler([](websocketpp::connection_hdl) {
            std::cerr << "[info] Binance WS closed. Reconnecting...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::exit(42);
        });

        // Initialize connection and attach stream
        websocketpp::lib::error_code ec;
        auto con = client.get_connection(ws_url, ec);
        if (ec) {
            std::cerr << "[connect error] " << ec.message() << "\n";
            return 1;
        }

        client.connect(con);
        client.run();
        
    } catch (const std::exception& e) {
        std::cerr << "[fatal] " << e.what() << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        goto reconnect;
    }

    // Join threads before clean exit
    if (bybit_thread.joinable())      bybit_thread.join();
    if (spread_monitor.joinable())    spread_monitor.join();
    if (stats_overlay.joinable())     stats_overlay.join();

    return 0;
}
