#!/usr/bin/env python3
"""Plot latency distribution from artifacts/latency.csv."""
from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--latency", default="artifacts/latency.csv")
    parser.add_argument("--out", default="artifacts/latency_hist.png")
    args = parser.parse_args()

    latency_path = Path(args.latency)
    df = pd.read_csv(latency_path)
    print(df["total_ns"].describe(percentiles=[0.5, 0.9, 0.99, 0.999]))

    plt.figure(figsize=(10, 6))
    plt.hist(df["total_ns"], bins=100)
    plt.title("End-to-end simulated latency")
    plt.xlabel("Latency (ns)")
    plt.ylabel("Count")
    plt.tight_layout()
    plt.savefig(args.out, dpi=160)
    print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
