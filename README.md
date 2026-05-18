# Low-Latency Trading Engine Simulator

A technical portfolio project that simulates a simplified low-latency trading stack:

1. A market-data producer generates order-book level updates.
2. A C++20 engine consumes updates through a bounded SPSC ring buffer.
3. The order book maintains best bid/offer state.
4. A mean-reversion strategy decides whether to buy or sell.
5. A risk manager checks position, order-size, and notional limits.
6. An order manager simulates marketable limit-order fills.
7. A portfolio module tracks inventory, cash, gross notional, and mark-to-market PnL.
8. Metrics are written to CSV for latency and throughput analysis.
9. Optional Redis publishing exposes live state.
10. Python scripts summarise and plot the output.

This is a simulator, not financial advice and not a live trading system.

## Why this project is useful

It demonstrates systems-level concepts that are relevant to trading infrastructure:

- C++ hot-path processing
- integer price representation
- lock-free-style SPSC queueing
- producer/consumer threading
- order-book state management
- risk-gated order generation
- simulated execution reports
- nanosecond-resolution latency measurement
- reproducible Docker deployment
- Python-based performance analysis

## Repository layout

```text
.
├── CMakeLists.txt
├── Dockerfile
├── docker-compose.yml
├── configs/default.conf
├── include/llt
│   ├── book/order_book.hpp
│   ├── core/config.hpp
│   ├── core/metrics.hpp
│   ├── core/spsc_ring.hpp
│   ├── core/time.hpp
│   ├── core/types.hpp
│   ├── feed/simulated_feed.hpp
│   ├── feed/udp_feed.hpp
│   ├── persistence/csv_writer.hpp
│   ├── persistence/redis_client.hpp
│   ├── portfolio/portfolio.hpp
│   ├── strategy/mean_reversion_strategy.hpp
│   └── trading
│       ├── order_manager.hpp
│       └── risk_manager.hpp
├── src/main.cpp
├── tests/test_order_book.cpp
├── scripts/analyze_latency.py
├── scripts/summarize_run.py
└── resume_bullets.md
```

## Build locally

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Run

```bash
./build/llt_engine --config configs/default.conf --out artifacts --events 1000000
```

Typical outputs:

```text
artifacts/
├── book.csv
├── latency.csv
├── summary.txt
└── trades.csv
```

## Analyse results

Create a virtual environment if needed:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

Summarise the run:

```bash
python scripts/summarize_run.py --dir artifacts
```

Plot latency distribution:

```bash
python scripts/analyze_latency.py --latency artifacts/latency.csv --out artifacts/latency_hist.png
```

## Docker

Build and run the engine:

```bash
docker build -t llt-engine .
docker run --rm -v "$PWD/artifacts:/app/artifacts" llt-engine
```

Run with Redis available:

```bash
docker compose up --build
```

To enable Redis state publishing, set this in `configs/default.conf`:

```text
use_redis = true
```

## Configuration

The main config file is `configs/default.conf`.

Important parameters:

```text
lookback = 64
threshold_ticks = 3
order_qty = 25
cooldown_events = 50
max_abs_position = 500
max_order_qty = 100
max_gross_notional = 1000000
book_sample_every = 1000
```

## Current architecture

The engine uses two primary threads:

- `producer` generates market-data events and pushes them into a bounded SPSC queue.
- `engine` consumes events, updates the book, runs strategy/risk/order/PnL logic, and records metrics.

This structure keeps the hot path simple while still demonstrating the basic shape of a real-time trading pipeline.

## Implementation details

### Integer prices

Prices are represented as integer ticks through `PriceTicks`. This avoids floating-point rounding in order-book logic and execution checks.

### SPSC ring buffer

`SpscRing<T, Capacity>` uses atomic head/tail indices with acquire-release semantics. It is suitable for one producer and one consumer. It is not a general multi-producer queue.

### Order book

The order book stores bid levels in descending price order and ask levels in ascending price order. The top level of each map gives the best bid/offer.

### Strategy

The included strategy is intentionally simple. It computes a rolling mean of the mid price and trades against short-term deviations:

- if current mid is above the rolling mean by more than `threshold_ticks`, sell;
- if current mid is below the rolling mean by more than `threshold_ticks`, buy.

### Risk

The risk manager blocks orders that exceed:

- maximum order quantity;
- maximum absolute position;
- maximum gross notional.

### Metrics

Each event records:

- feed-to-engine queue delay;
- strategy decision time;
- engine hot-path processing latency after dequeue.

The summary file reports throughput and p50/p90/p99 engine hot-path latency.

## Suggested extensions

High-quality extensions for a stronger portfolio project:

1. CPU affinity and thread pinning.
2. Pre-allocated object pools for orders and reports.
3. Fixed-size symbol identifiers instead of strings.
4. pcap or CSV replay mode using real historical tick data.
5. A proper matching engine with resting orders and partial fills.
6. Multi-symbol sharding across engine threads.
7. A Prometheus metrics endpoint.
8. UDP receiver mode connected to an external feed generator process.
9. Benchmarks comparing map-based and flat-array order-book implementations.
10. Kernel-bypass discussion in the README, even if not implemented.

## Limitations

This is not a production trading system. It does not implement exchange-native protocols, real exchange connectivity, regulatory compliance checks, deterministic replay of real packets, market-impact modelling, or colocated deployment concerns.
