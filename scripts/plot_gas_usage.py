#%%
import argparse
import json
import gzip
import logging

from pandas.io.json import json_normalize
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np



COLUMNS_TO_CAST = [
    "transaction.gas",
    "transaction.gas_for_deposit",
    "transaction.gas_refunded",
    "transaction.gas_used",
]


def load_data(filepath, start=1_000_000, stop=1_500_000):
    logging.info("loading data from %s", filepath)
    rows = []
    with gzip.open(filepath) as f:
        for i, line in enumerate(f):
            if i >= stop:
                break
            elif i >= start:
                rows.append(json.loads(line))
    df = json_normalize(rows)
    for column in COLUMNS_TO_CAST:
        df[column] = df[column].astype(np.int64)
    logging.info("finished loading data from %s", filepath)
    return df


def plot_memory_graph(df, args):
    if args.max_memory:
        df = df[df["usage.extra_memory_allocated"] < args.max_memory]
    df["memory_intensive"] = df["usage.extra_memory_allocated"] > args.memory_threshold
    with_memory_allocated = df[df["usage.extra_memory_allocated"] >= 0]
    g = sns.lmplot(x="transaction.gas_used", y="usage.extra_memory_allocated",
                   data=with_memory_allocated, hue="memory_intensive")
    g.ax.ticklabel_format(axis="both", style="sci", scilimits=(0, 5))
    plt.savefig("memory-gas-{0}-{1}.png".format(args.start, args.stop))


def plot_cpu_graph(df, args):
    without_dos = df[df["usage.clock_time"] < 1]
    ax = sns.regplot(x="transaction.gas_used", y="usage.clock_time", data=without_dos)
    ax.ticklabel_format(axis="both", style="sci", scilimits=(0, 5))
    plt.savefig("cpu-gas-{0}-{1}.png".format(args.start, args.stop))


def main():
    logging.basicConfig(level=logging.INFO)
    parser = argparse.ArgumentParser(prog="analyze-transactions")

    subparsers = parser.add_subparsers(dest="command")

    memory_parser = subparsers.add_parser("memory")
    memory_parser.add_argument("input", help="input file")
    memory_parser.add_argument("--start", help="start index", type=int, default=1_000_000)
    memory_parser.add_argument("--stop", help="stop index", type=int, default=1_500_000)
    memory_parser.add_argument("--max-memory", help="max memory", type=int)
    memory_parser.add_argument("--memory-threshold", help="threshold for high memory", type=int, default=300_000)

    cpu_parser = subparsers.add_parser("cpu")
    cpu_parser.add_argument("input", help="input file")
    cpu_parser.add_argument("--start", help="start index", type=int, default=1_000_000)
    cpu_parser.add_argument("--stop", help="stop index", type=int, default=1_500_000)

    args = parser.parse_args()

    if not args.command:
        parser.error("no command provided")

    df = load_data(args.input, start=args.start, stop=args.stop)

    if args.command == "memory":
        plot_memory_graph(df, args)
    elif args.command == "cpu":
        plot_cpu_graph(df, args)


#%%

if __name__ == "__main__":
    main()
