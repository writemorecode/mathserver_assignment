import numpy as np
import threading

def matrix_inverse_seq(A):
    n = A.shape[0]
    I = np.eye(n)

    for pivot_row in range(n):
        pivot_value = A[pivot_row, pivot_row]
        A[pivot_row] /= pivot_value
        I[pivot_row] /= pivot_value

        for row in range(n):
            multiplier = A[row, pivot_row]
            if row == pivot_row: continue
            A[row] -= A[pivot_row] * multiplier
            I[row] -= I[pivot_row] * multiplier
    return I




def main():
    A = np.array([[3,6,9],[2,8,15],[7,3,8]],dtype=np.float64)
    # print(A)
    A_inv = np.linalg.inv(A)
    # print(A_inv)
    M = matrix_inverse_seq(A)
    # print(M)
    if np.allclose(A_inv, M):
        print("TEST PASS")
    else:
        print("TEST FAIL")


if __name__ == "__main__":
    main()
