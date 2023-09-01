#include "../include/matrix_test.h"
#include "../include/matrix.h"

bool matrix_new_test()
{
    size_t N = 16;
    struct matrix* M = matrix_new(N);
    if (M == NULL) {
        fprintf(stderr, "FAIL: 'matrix_new' returned NULL.\n");
        return NULL;
    }

    if (M->n != N) {
        fprintf(stderr, "FAIL: 'matrix_new' returned matrix with dimension %ld, expected %ld.\n", M->n, N);
        return NULL;
    }

    if (M->data == NULL) {
        fprintf(stderr, "FAIL: 'matrix_new' returned matrix with NULL data field.\n");
        return NULL;
    }

    matrix_free(M);

    return true;
}

bool matrix_identity_test()
{
    size_t N = 16;
    struct matrix* I = matrix_identity(N);

    for (size_t i = 0; i < I->n; i++) {
        for (size_t j = 0; j < I->n; j++) {
            if (i == j && I->data[i * I->n + j] != 1.0) {
                fprintf(stderr, "FAIL: 'matrix_identity' returned an invalid identity matrix.\n");
                return false;
            }
            if (i != j && I->data[i * I->n + j] != 0.0) {
                fprintf(stderr, "FAIL: 'matrix_identity' returned an invalid identity matrix.\n");
                return false;
            }
        }
    }

    matrix_free(I);

    return true;
}

bool matrix_inverse_test()
{
    size_t N = 16;
    struct matrix* M = matrix_random(N, 256);
    struct matrix* M_copy = matrix_copy(M);

    struct matrix* M_inv = matrix_inverse(M);
    struct matrix* I = matrix_identity(N);

    struct matrix* M_id = matrix_multiply(M_copy, M_inv);

    if (matrix_equals(M_id, I, 0.01) == false) {
        fprintf(stderr, "FAIL: 'matrix_inverse' did not correctly invert matrix.\n");
    }

    matrix_free(M);
    matrix_free(M_copy);
    matrix_free(M_inv);
    matrix_free(M_id);
    matrix_free(I);

    return true;
}
