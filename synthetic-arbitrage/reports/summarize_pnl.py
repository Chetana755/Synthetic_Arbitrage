import argparse
import pandas as pd
import matplotlib.pyplot as plt
import plotly.express as px
import os
import zipfile
from datetime import datetime

def parse_args():
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Analyze arbitrage trading performance.")
    parser.add_argument('--file', default="arbitrage_log.csv", help='Path to arbitrage CSV log')
    parser.add_argument('--rolling', type=int, default=10, help='Rolling window size for metrics')
    return parser.parse_args()

def main():
    args = parse_args()

    # Validate input file
    if not os.path.exists(args.file):
        print(f"File not found: {args.file}")
        return

    # Create output directory with timestamp
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    report_dir = os.path.join("reports", timestamp)
    os.makedirs(report_dir, exist_ok=True)

    # Load CSV log into DataFrame
    df = pd.read_csv(args.file)

    # Print basic diagnostics
    print("\n=== Trade Log Snapshot ===")
    print(f"Rows loaded         : {len(df)}")
    print(f"Columns detected    : {list(df.columns)}")
    print("First few rows:")
    print(df.head())

    # Remove accidental header duplicates
    df = df[df['timestamp'] != 'timestamp']

    # Coerce numeric fields
    numeric_fields = ['timestamp', 'synthetic_price', 'real_price', 'net_pnl']
    for col in numeric_fields:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors='coerce')
        else:
            print(f"Missing required column: {col}")
            return

    # Handle optional 'size' column
    if 'size' in df.columns:
        df['size'] = pd.to_numeric(df['size'], errors='coerce')
    else:
        print("'size' column missing — skipping per-dollar PnL calculation.")

    # Convert timestamp to datetime and set index
    df['datetime'] = pd.to_datetime(df['timestamp'], unit='ms', errors='coerce')
    df = df.dropna(subset=['datetime'])
    df.set_index('datetime', inplace=True)
    trades = df.copy()

    # Compute derived metrics
    trades['cumulative_pnl'] = trades['net_pnl'].cumsum()
    trades['spread'] = trades['synthetic_price'] - trades['real_price']
    if 'size' in trades.columns:
        trades['pnl_per_dollar'] = trades['net_pnl'] / trades['size']
    trades['rolling_win'] = trades['net_pnl'] > 0
    trades['rolling_winrate'] = trades['rolling_win'].rolling(window=args.rolling).mean() * 100
    trades['rolling_avg_pnl'] = trades['net_pnl'].rolling(window=args.rolling).mean()
    trades['gap'] = trades.index.to_series().diff().dt.total_seconds()

    # Save cleaned snapshot
    trades.to_csv(os.path.join(report_dir, "trades_snapshot.csv"))

    # Plot cumulative PnL
    plt.figure(figsize=(10, 5))
    plt.plot(trades['cumulative_pnl'], marker='o', linewidth=2)
    plt.title("Cumulative PnL Over Time")
    plt.xlabel("Time")
    plt.ylabel("PnL (USD)")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(report_dir, "cumulative_pnl.png"))

    # Summary statistics
    win_trades = trades[trades['net_pnl'] > 0]
    loss_trades = trades[trades['net_pnl'] <= 0]
    max_drawdown = (trades['cumulative_pnl'].cummax() - trades['cumulative_pnl']).max()

    print("\n=== Trade Performance Summary ===")
    print(f"Total Trades        : {len(trades)}")
    print(f"Winning Trades      : {len(win_trades)}")
    print(f"Losing Trades       : {len(loss_trades)}")
    print(f"Win Rate            : {len(win_trades) / len(trades) * 100:.2f}%")
    print(f"Average PnL/Trade   : {trades['net_pnl'].mean():.4f} USD")
    print(f"Maximum Drawdown    : {max_drawdown:.4f} USD")

    # Directional breakdown
    print("\n=== PnL by Trade Direction ===")
    if 'direction' in trades.columns:
        for direction, group in trades.groupby('direction'):
            print(f"{direction}")
            print(f"  Trades    : {len(group)}")
            print(f"  Win Rate  : {(group['net_pnl'] > 0).mean() * 100:.2f}%")
            print(f"  Avg PnL   : {group['net_pnl'].mean():.4f} USD")
            print(f"  Total PnL : {group['net_pnl'].sum():.4f} USD")

        # Plot cumulative PnL by direction
        plt.figure(figsize=(12, 6))
        for direction, group in trades.groupby('direction'):
            plt.plot(group.index, group['cumulative_pnl'], label=direction)
        plt.title("Cumulative PnL by Trade Direction")
        plt.xlabel("Time")
        plt.ylabel("PnL (USD)")
        plt.legend()
        plt.grid(True)
        plt.tight_layout()
        plt.savefig(os.path.join(report_dir, "directional_pnl.png"))

    # Plot rolling win rate
    plt.figure(figsize=(12, 4))
    plt.plot(trades.index, trades['rolling_winrate'], color='blue')
    plt.title(f"Rolling Win Rate ({args.rolling} trades)")
    plt.xlabel("Time")
    plt.ylabel("Win Rate (%)")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(report_dir, "rolling_winrate.png"))

    # Plot rolling average PnL
    plt.figure(figsize=(12, 4))
    plt.plot(trades.index, trades['rolling_avg_pnl'], color='purple')
    plt.title(f"Rolling Avg PnL ({args.rolling} trades)")
    plt.xlabel("Time")
    plt.ylabel("Avg PnL (USD)")
    plt.grid(True)
    plt.tight_layout()
    plt.savefig(os.path.join(report_dir, "rolling_avg_pnl.png"))

    # Plot histogram of trade timing gaps
    plt.figure(figsize=(8, 4))
    plt.hist(trades['gap'].dropna(), bins=40, color='gray', alpha=0.7)
    plt.title("Time Between Trades")
    plt.xlabel("Seconds")
    plt.ylabel("Frequency")
    plt.tight_layout()
    plt.savefig(os.path.join(report_dir, "trade_gaps.png"))

    # Generate interactive PnL chart with Plotly
    fig = px.line(trades, x=trades.index, y="cumulative_pnl", title="Cumulative PnL (Interactive)")
    fig.write_html(os.path.join(report_dir, "pnl_dashboard.html"))

    # Package all report assets into a ZIP file
    zip_path = os.path.join("reports", f"{timestamp}_report.zip")
    with zipfile.ZipFile(zip_path, "w") as zf:
        for file in os.listdir(report_dir):
            zf.write(os.path.join(report_dir, file), arcname=file)

    print(f"\nReport saved to: {report_dir}")
    print(f"ZIP archive created: {zip_path}")

if __name__ == "__main__":
    main()
