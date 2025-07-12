
# Synthetic Arbitrage System – Architecture Overview

## 1. System Purpose

The system is designed to detect mispricing opportunities between real and synthetic instruments across multiple exchanges in real time. It simulates arbitrage trades, tracks performance metrics, and logs structured data for analysis and submission.

---

## 2. System Components

### Core Modules (C++)

| Module                   | Description                                                  |
|--------------------------|--------------------------------------------------------------|
| `main.cpp`               | Entry point; manages lifecycle, feed initiation, logging     |
| `SyntheticEngine.cpp`    | Computes synthetic prices from paired instrument feeds       |
| `CrossExchangePair.cpp`  | Detects delta-based arbitrage opportunities                  |
| `bybit_clean.cpp`        | Cleans and adapts Bybit feeds to internal format             |
| `binance_funding_ws.cpp` | Ingests funding data via WebSocket                           |

### Header Definitions (`include/`)

- `SyntheticPair.hpp` – abstraction of synthetic price components  
- `ArbOpportunityEngine.hpp` – encapsulates arbitrage detection logic  
- `MockPnLEngine.hpp` – manages PnL and cumulative metrics  
- `ArbLogger.hpp`, `SyntheticSnapshotLogger.hpp` – trade recording  
- `TradeTick.hpp` – encapsulates individual price snapshots or trade data  

---

## 3. External Libraries

- **ASIO**: Manages the networking stack and asynchronous timers  
- **WebSocket++**: Used for connecting to exchange feeds (e.g., Binance, Bybit)  
- **OpenSSL**: Enables secure data transmission over WebSockets  
- **nlohmann/json**: Parses and serializes structured feed data

---

## 4. File & Data Flow

```text
[ Exchange Feeds ] → binance_funding_ws.cpp / bybit_clean.cpp
        ↓
[ SyntheticEngine ] → Generates synthetic price
        ↓
[ CrossExchangePair ] → Compares synthetic vs. real price
        ↓
[ ArbOpportunityEngine ] → Signals trade if threshold met
        ↓
[ MockPnLEngine ] → Calculates net and cumulative PnL
        ↓
[ SyntheticSnapshotLogger ] → Writes structured entry to arbitrage_log.csv
```

---

## 5. Output Artifacts

- `arbitrage_log.csv` — primary trade snapshot  
- `eth_synthetic_log.csv` — synthetic price timeline  
- `mispricing_ETHUSDT.csv` — detected spread records  

Each written to the `build/` directory. Optional zipped reports are placed in `reports/YYYYMMDD_HHMMSS_report.zip`.

---

## 6. Architectural Highlights

- Modular component design with header encapsulation for reuse  
- Event-driven data processing pipeline using ASIO and WebSocket++  
- Latency-aware synthetic pricing and timestamped logging  
- Configurable detection thresholds and feed validation  
- Clear separation between simulation logic and file IO

---

## 7. Extensibility

The system is designed to support:

- Multiple synthetic pair models  
- Additional exchange feeds via modular WebSocket clients  
- Enhanced pricing using weightings, volatility filters, or statistical edges  
- Plug-in strategy modules through header definitions  
- Optional live dashboard or RPC control layer

```

---

