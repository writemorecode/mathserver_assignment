from matplotlib import pyplot as plt
import polars as po
import sys


def main():
    if len(sys.argv) != 3:
        print("usage: %s [data filename] [plot filename]".format(sys.argv[0]))
        return
    filename = sys.argv[1]
    plot_filename = sys.argv[2]
    df = po.read_csv(filename, separator=" ", has_header=False, new_columns=["x", "y", "cluster"])
    plt.scatter(df["x"], df["y"], c=df["cluster"])
    plt.savefig(plot_filename)


if __name__ == "__main__":
    main()
