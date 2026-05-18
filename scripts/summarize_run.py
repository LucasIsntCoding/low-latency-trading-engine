#!/usr/bin/env python3
"""Generate human-readable performance and trading summary from the engine outputs."""
from __future__ import annotations

import argparse
from pathlib import Path

import pandas as pd


def pct(series: pd.Series, q: float) -> float:
    return float(series.quantile(q)) if len(series) else 0.0


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dir", default="artifacts")
    args = parser.parse_args()
    root = Path(args.dir)
    lat = pd.read_csv(root / "latency.csv")
    trades = pd.read_csv(root / "trades.csv")

    print("Latency")
    print(f"  events: {len(lat):,}")
    print(f"  mean ns: {lat.total_ns.mean():,.1f}")
    print(f"  p50 / p90 / p99 ns: {pct(lat.total_ns, .50):,.0f} / {pct(lat.total_ns, .90):,.0f} / {pct(lat.total_ns, .99):,.0f}")
    print()
    print("Trading")
    print(f"  fills: {len(trades):,}")
    if len(trades):
        print(f"  final position: {int(trades.position_after.iloc[-1]):,}")
        print(f"  final PnL: {float(trades.pnl_after.iloc[-1]):,.2f}")
        print(f"  avg fill price: {trades.fill_price.mean():,.4f}")


if __name__ == "__main__":
    main()
