from sklearn.datasets import make_blobs
import random


def main():
    X, y = make_blobs(
        n_samples=1500,
        n_features=2,
        centers=[(-5, 5), (5, 5), (-5, -5)],
        random_state=random.randint(0, 100),
    )
    with open("kmeans-data.txt", "w") as fh:
        for sample in X:
            x, y = sample
            fh.write(f"{x:.3f}\t{y:.3f}\n")


if __name__ == "__main__":
    main()
