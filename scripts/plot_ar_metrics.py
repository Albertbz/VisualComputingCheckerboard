#!/usr/bin/env python3
"""
Simple analysis and plotting for AR measurement CSV output (ar_log.csv).
Produces summary statistics and saves several PNG plots.

Dependencies: pandas, numpy, matplotlib, seaborn (seaborn optional)

Usage:
    python scripts/plot_ar_metrics.py ar_log.csv
"""
import sys
import os
import argparse
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

try:
    import seaborn as sns

    sns.set(style="whitegrid")
except Exception:
    pass

from pathlib import Path


def summarize_series(s, name):
    return {
        "name": name,
        "count": int(s.count()),
        "mean": float(s.mean()),
        "median": float(s.median()),
        "std": float(s.std()),
        "min": float(s.min()),
        "max": float(s.max()),
        "p90": float(s.quantile(0.9)),
        "p99": float(s.quantile(0.99)),
    }


def main():
    p = argparse.ArgumentParser()
    p.add_argument("csv", help="Path to ar_log.csv")
    p.add_argument("--outdir", default="ar_analysis", help="Output directory for plots")
    args = p.parse_args()

    df = pd.read_csv(args.csv)
    outdir = Path(args.outdir)
    outdir.mkdir(parents=True, exist_ok=True)

    # Compute derived latency columns (in ms)
    df = df.sort_values(by="frame").reset_index(drop=True)
    df["lat_cap_to_swap"] = df["swap_ms"] - df["cap_ms"]
    df["lat_cap_to_pnp"] = df["pnp_ms"] - df["cap_ms"]
    df["lat_pnp_to_upload"] = df["upload_ms"] - df["pnp_ms"]
    df["lat_upload_to_swap"] = df["swap_ms"] - df["upload_ms"]

    # Print and save summary statistics
    stats = {}
    stats["cap_to_swap"] = summarize_series(df["lat_cap_to_swap"], "cap_to_swap")
    stats["cap_to_pnp"] = summarize_series(df["lat_cap_to_pnp"], "cap_to_pnp")
    stats["pnp_to_upload"] = summarize_series(df["lat_pnp_to_upload"], "pnp_to_upload")
    stats["upload_to_swap"] = summarize_series(
        df["lat_upload_to_swap"], "upload_to_swap"
    )
    stats["reproj_mean"] = summarize_series(
        df["reproj_mean"].replace(-1, np.nan).dropna(), "reproj_mean"
    )

    print("Summary statistics:\n")
    for k, v in stats.items():
        print(
            f"{k}: count={v['count']} mean={v['mean']:.3f} median={v['median']:.3f} std={v['std']:.3f} min={v['min']:.3f} max={v['max']:.3f} p90={v['p90']:.3f}"
        )

    # Save textual summary
    with open(outdir / "summary.txt", "w") as fh:
        for k, v in stats.items():
            fh.write(f"{k}: {v}\n")

    # Plot latency histogram for cap->swap
    plt.figure(figsize=(8, 4))
    plt.hist(df["lat_cap_to_swap"].dropna(), bins=80, color="tab:blue")
    plt.xlabel("Capture -> Swap (ms)")
    plt.ylabel("Frames")
    plt.title("End-to-end latency (capture to swap)")
    plt.tight_layout()
    plt.savefig(outdir / "latency_cap_to_swap_hist.png", dpi=150)
    plt.close()

    # Time series of capture->swap latency
    plt.figure(figsize=(10, 4))
    plt.plot(
        df["frame"], df["lat_cap_to_swap"], marker=".", markersize=2, linewidth=0.5
    )
    plt.xlabel("Frame")
    plt.ylabel("Latency (ms)")
    plt.title("Capture->Swap latency over time")
    plt.tight_layout()
    plt.savefig(outdir / "latency_cap_to_swap_timeseries.png", dpi=150)
    plt.close()

    # Reprojection error time series
    if "reproj_mean" in df.columns:
        plt.figure(figsize=(10, 4))
        plt.plot(
            df["frame"], df["reproj_mean"], marker=".", markersize=2, linewidth=0.5
        )
        plt.xlabel("Frame")
        plt.ylabel("Mean reprojection error (px)")
        plt.title("Reprojection error over time")
        plt.tight_layout()
        plt.savefig(outdir / "reproj_mean_timeseries.png", dpi=150)
        plt.close()

    # Translation components time series
    if all(c in df.columns for c in ["t_x", "t_y", "t_z"]):
        plt.figure(figsize=(10, 4))
        plt.plot(df["frame"], df["t_x"], label="t_x")
        plt.plot(df["frame"], df["t_y"], label="t_y")
        plt.plot(df["frame"], df["t_z"], label="t_z")
        plt.xlabel("Frame")
        plt.ylabel("Translation (m)")
        plt.legend()
        plt.title("Translation components over time")
        plt.tight_layout()
        plt.savefig(outdir / "translation_timeseries.png", dpi=150)
        plt.close()

    # Rotation vector magnitude (angle approx) time series
    if all(c in df.columns for c in ["r_x", "r_y", "r_z"]):
        rot_mag = np.sqrt(df["r_x"] ** 2 + df["r_y"] ** 2 + df["r_z"] ** 2)
        plt.figure(figsize=(10, 4))
        plt.plot(df["frame"], rot_mag, marker=".", markersize=2, linewidth=0.5)
        plt.xlabel("Frame")
        plt.ylabel("Rodrigues magnitude (rad)")
        plt.title("Rotation vector magnitude over time")
        plt.tight_layout()
        plt.savefig(outdir / "rotation_magnitude_timeseries.png", dpi=150)
        plt.close()

    print(f"Plots and summary saved to {outdir.resolve()}")


if __name__ == "__main__":
    main()
