#ifndef PARMATINV_H
#define PARMATINV_H

#include <pthread.h>
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
    size_t row_start;
    size_t row_end;
    size_t thread_id;
    pthread_barrier_t *barrier;
};

struct matrix *matrix_new(size_t n_);
struct matrix *matrix_identity(size_t n_);
struct matrix *matrix_create_fast(size_t n_);
struct matrix *matrix_copy(struct matrix *mat);
void matrix_free(struct matrix *mat);
void matrix_write(struct matrix *mat);
struct matrix *matrix_random(size_t n_, size_t max);
struct matrix *matrix_multiply(struct matrix *A, struct matrix *B);

struct matrix *foo(struct matrix *M);
struct matrix *matrix_inverse(struct matrix *mat);
struct matrix *matrix_inverse_parallel(struct matrix *mat);

#endif
