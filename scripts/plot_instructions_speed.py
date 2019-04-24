import argparse
import json
import pandas as pd



def load_data(input_path):
    with open(input_path) as f:
        data = json.load(f)
        return pd.DataFrame.from_dict(data["stats"], orient="index")


def plot_data(df, instructions, output):
    ax = df.loc[instructions].plot.bar(y="mean", yerr="stdev", legend=None)
    ax.set_xlabel("Instruction")
    ax.set_ylabel("Mean time (ns)")
    fig = ax.get_figure()
    fig.subplots_adjust(bottom=0.2)
    fig.savefig(output)


def main():
    parser = argparse.ArgumentParser(prog="plot-inst")
    parser.add_argument("input", help="input-file")
    parser.add_argument("-i", "--instruction", required=True, nargs="+", help="instructions to plot")
    parser.add_argument("-o", "--output", required=True, help="output path")
    args = parser.parse_args()

    df = load_data(args.input)
    plot_data(df, args.instruction, args.output)
    print(df.loc[args.instruction])


if __name__ == "__main__":
    main()
