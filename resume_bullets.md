# Resume Bullets

Use these only after you have run the project, understood the code, and can explain the architecture and trade-offs in an interview.

- Built a C++20 low-latency trading engine simulator with a simulated market-data feed, order-book updates, mean-reversion strategy logic, pre-trade risk checks, execution simulation, PnL tracking, and nanosecond-resolution latency instrumentation.
- Implemented a single-producer/single-consumer ring buffer and multi-threaded producer/engine pipeline to separate market-data ingestion from strategy execution and reduce contention in the hot path.
- Added CSV-based observability and Python analysis scripts for p50/p90/p99 latency, throughput, trade logs, final inventory, and mark-to-market PnL.
- Integrated optional Redis state publishing and Docker-based reproducible deployment for local performance experiments.

# Interview Discussion Points

- Why integer price ticks are used instead of floating point prices in the engine.
- Why the SPSC queue avoids locks under one producer and one consumer.
- What the current simulator simplifies compared with a production HFT system, including exchange connectivity, packet loss handling, colocated deployment, kernel bypass, exchange-specific order states, and regulatory controls.
- How to extend the project with a true limit-order matching engine, pcap replay, FIX/OUCH protocol simulation, or CPU affinity pinning.
