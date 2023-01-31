#ifndef PARMATINV_H
#define PARMATINV_H

#include <stddef.h>
#include <stdio.h>

struct matrix
{
    // Only handling square matrices
    size_t n;
    double *data;
};

struct thread_arg
{
    struct matrix *M;
    struct matrix *I;
    size_t r;
    size_t p;
    size_t count;
};

struct matrix *matrix_new(size_t n_);
struct matrix *matrix_identity(size_t n_);
struct matrix *matrix_create_fast(size_t n_);
struct matrix *matrix_copy(struct matrix *mat);
void matrix_free(struct matrix *mat);
void matrix_write(struct matrix *mat, FILE *fp);
struct matrix *matrix_random(size_t n_, size_t max);
struct matrix *matrix_multiply(struct matrix *A, struct matrix *B);

struct matrix *matrix_inverse(struct matrix *mat);
struct matrix *matrix_inverse_parallel(struct matrix *mat);

// Functions for Gauss-Jordan elimination
void *divide_row(void *arg);
void *eliminate_forward(void *arg);
void *eliminate_backward(void *arg);

#endif
