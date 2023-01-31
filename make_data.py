from sklearn.datasets import make_blobs

X, y = make_blobs(n_samples = 3000, n_features=2, centers=[(-5,5),(5,5),(-5,-5)], random_state=42)

with open("kmeans-data.txt", "w") as fh:
    for sample in X:
        x,y = sample
        fh.write(f"{x}\t{y}\n")


