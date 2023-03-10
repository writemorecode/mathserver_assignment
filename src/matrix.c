#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include "../include/matrix.h"

#define THREAD_COUNT 4

struct matrix *matrix_new(size_t n_)
{
    struct matrix *mat = calloc(1, sizeof(struct matrix));
    mat->n = n_;
    mat->data = calloc(n_ * n_, sizeof(double));
    return mat;
}

struct matrix *matrix_copy(struct matrix *mat)
{
    if (mat == NULL)
    {
        return NULL;
    }
    size_t n = mat->n;
    struct matrix *copy = matrix_new(n);
    memcpy(copy->data, mat->data, sizeof(double) * n * n);
    return copy;
}

void matrix_free(struct matrix *mat)
{
    if (mat == NULL)
    {
        return;
    }
    free(mat->data);
    free(mat);
}

/*
 * Writes the matrix to file descriptor fp.
 * Set fp to stdout to write matrix to screen.
 */
void matrix_write(struct matrix *mat)
{
    for (size_t i = 0; i < mat->n; i++)
    {
        for (size_t j = 0; j < mat->n; j++)
        {
            fprintf(stdout, "%0.4f", mat->data[i * mat->n + j]);

            if (j < mat->n)
            {
                fprintf(stdout, "    ");
            }
        }
        if (i < mat->n)
        {
            fprintf(stdout, "\n");
        }
    }
    fprintf(stdout, "\n");
}

/*
 * Returns the nxn identity matrix.
 */
struct matrix *matrix_identity(size_t n_)
{
    struct matrix *mat = matrix_new(n_);
    if (mat == NULL)
    {
        return NULL;
    }

    // calloc in matrix_new will initialize matrix to zero
    for (size_t i = 0; i < mat->n; i++)
    {
        mat->data[i * mat->n + i] = 1;
    }

    return mat;
}

struct matrix *matrix_random(size_t n_, size_t max)
{
    struct matrix *mat = matrix_new(n_);
    if (mat == NULL)
    {
        return NULL;
    }

    time_t t;
    srand((unsigned) time(&t));

    for (size_t i = 0; i < mat->n; i++)
    {
        for (size_t j = 0; j < mat->n; j++)
        {
            if (i == j) /* diagonal dominance */
            {
                mat->data[i * mat->n + j] = (double)(rand() % max) + 5.0;
            }
            else
            {
                mat->data[i * mat->n + j] = (double)(rand() % max) + 1.0;
            }
        }
    }

    return mat;
}

struct matrix *matrix_create_fast(size_t n_)
{
    struct matrix *mat = matrix_new(n_);
    if (mat == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < mat->n; i++)
    {
        for (size_t j = 0; j < mat->n; j++)
        {
            if (i == j) /* diagonal dominance */
            {
                mat->data[i * mat->n + j] = 5.0;
            }
            else
            {
                mat->data[i * mat->n + j] = 2.0;
            }
        }
    }

    return mat;
}

struct matrix *matrix_multiply(struct matrix *A, struct matrix *B)
{
    if (A->n != B->n)
    {
        return NULL;
    }
    size_t n = A->n;
    struct matrix *C = matrix_new(A->n);
    for (size_t i = 0; i < n; i++)
    {
        for (size_t j = 0; j < n; j++)
        {
            float sum = 0;
            for (size_t k = 0; k < n; k++)
            {
                sum += (A->data[i * n + k] * B->data[k * n + j]);
            }
            C->data[i * n + j] = sum;
        }
    }
    return C;
}

struct matrix *matrix_inverse(struct matrix *mat)
{
    size_t row, col, p;
    size_t n = mat->n;
    double pv, multiplier;
    struct matrix *id = matrix_identity(n);

    for (p = 0; p < n; p++)
    {
        pv = mat->data[p * n + p];
        for (col = 0; col < n; col++)
        {
            mat->data[p * n + col] /= pv;
            id->data[p * n + col] /= pv;
        }
        assert(mat->data[p * n + p] == 1.0);

        for (row = 0; row < n; row++)
        {
            multiplier = mat->data[row * n + p];
            if (row == p)
            {
                continue;
            }
            for (col = 0; col < n; col++)
            {
                mat->data[row * n + col] -= mat->data[p * n + col] * multiplier;
                id->data[row * n + col] -= id->data[p * n + col] * multiplier;
            }
        }
    }
    return id;
}


void *worker(void *thr_arg)
{
    return NULL;
}

struct matrix *matrix_inverse_parallel(struct matrix *M)
{
    const size_t n = M->n;
    struct matrix *I = matrix_identity(n);

    pthread_t threads[THREAD_COUNT] = {0};
    struct thread_arg args[THREAD_COUNT];

    size_t rows_per_thread = n / THREAD_COUNT;
    size_t remainder = n % THREAD_COUNT;

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, THREAD_COUNT);

    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        args[i].M = M;
        args[i].I = I;
        args[i].row_start = rows_per_thread * i;
        args[i].row_end = rows_per_thread * (i + 1);
        args[i].thread_id = i;
        args[i].barrier = &barrier;
    }
    if (remainder > 0)
    {
        args[THREAD_COUNT - 1].row_end += remainder;
    }

    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        pthread_create(&threads[i], NULL, worker, &args[i]);
    }

    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return I;
}