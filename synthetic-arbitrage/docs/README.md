
## Build Instructions

### Requirements

- C++17 compliant compiler  
- CMake ≥ 3.16  
- OpenSSL libraries (`libssl`, `libcrypto`)  
- Header-only libraries: ASIO, WebSocket++, nlohmann/json  

### Compilation

```bash
mkdir build
cd build
cmake ..
make
```

Executable generated:

```
build/SyntheticArbitrage.exe
```

---

## Project Structure Overview

```
synthetic_arbitrage/
├── CMakeLists.txt
├── src/
│   ├── main.cpp
│   ├── SyntheticEngine.cpp
│   ├── CrossExchangePair.cpp
│   ├── bybit_clean.cpp
│   └── feeds/binance_funding_ws.cpp
├── include/
│   ├── ArbLogger.hpp
│   ├── SyntheticPair.hpp
│   ├── ArbOpportunityEngine.hpp
│   ├── SyntheticSnapshotLogger.hpp
│   ├── MockPnLEngine.hpp
│   └── TradeTick.hpp
├── libs/                  # ASIO, WebSocket++, json
├── build/
│   ├── SyntheticArbitrage.exe
│   ├── arbitrage_log.csv
│   ├── eth_synthetic_log.csv
│   └── mispricing_ETHUSDT.csv
├── data/                  # Example feed and log inputs
├── docs/                  # architecture.md, performance_report.md
└── reports/               # Output snapshots and ZIPs
```

---

## Usage Guide

### Executing the Engine

```bash
./SyntheticArbitrage.exe
```

Upon startup:
- Initializes synthetic pricing engine  
- Connects to WebSocket feeds (Binance, Bybit)  
- Runs arbitrage logic and logs trades to CSV  

### Output Format: `arbitrage_log.csv`

```
timestamp, symbol, real_price, synthetic_price, delta,
net_pnl, cumulative_pnl, direction, latency, size
```

Logging is managed by `SyntheticSnapshotLogger` and `ArbLogger`.

---

## Module Overview

### Synthetic Pricing Logic

Defined in `SyntheticEngine.cpp` and `SyntheticPair.hpp`:

```cpp
double synthetic_price = (priceA + priceB) / 2.0;
```

Includes latency correction using timestamp gaps.

---

### Arbitrage Detection

Implemented in `CrossExchangePair.cpp`:

```cpp
double delta = synthetic_price - real_price;
if (abs(delta) > threshold) {
    // record arbitrage opportunity
}
```

Direction tagged via gap polarity.

---

### Logging System

Headers:
- `ArbLogger.hpp`
- `SyntheticSnapshotLogger.hpp`

Functions:
- `writeHeaderOnce()` — single header row  
- `appendTradeRow()` — per-trade logging  
- Validates missing fields, prevents format mismatches

---

## Networking and Feed Input

- ASIO manages timers, sockets, and loops  
- WebSocket++ connects to funding rate endpoints  
- Exchange data processed in:
  - `feeds/binance_funding_ws.cpp`  
  - `bybit_clean.cpp`

---

## Performance Benchmarks

Measured in local test environment:

| Component           | Metric              |
|--------------------|---------------------|
| Trade throughput    | ~480 trades/sec     |
| Log latency         | ~1.2 ms per trade   |
| Memory usage        | ~74–80 MB peak      |
| Feed connection     | Multi-threaded WS   |

---

## Reporting and Packaging

Generated logs and assets are stored in:
```
reports/YYYYMMDD_HHMMSS/
```

Packaged into:
```
report_YYYYMMDD_HHMMSS.zip
```

Contains:
- CSV output  
- Time-stamped PnL charts  
- Metadata and system status  

---

## Documentation Set

Located in `docs/`:

- `architecture.md` — system design breakdown  
- `performance_report.md` — latency, throughput, memory  
- `finance_report.md` — pricing model, arbitrage logic, risk controls

These documents should be bundled alongside the executable and trade logs.

---

## Licensing Notes

Used libraries:

- ASIO (Boost License)  
- WebSocket++ (BSD)  
- nlohmann/json (MIT)  


---

