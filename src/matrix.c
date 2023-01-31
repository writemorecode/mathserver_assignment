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

#define MAX_THREAD_COUNT 4

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
void matrix_write(struct matrix *mat, FILE *fp)
{
    for (size_t i = 0; i < mat->n; i++)
    {
        for (size_t j = 0; j < mat->n; j++)
        {
            fprintf(fp, "%0.4f", mat->data[i * mat->n + j]);

            if (j < mat->n)
            {
                fprintf(fp, "\t");
            }
        }
        if (i < mat->n)
        {
            fprintf(fp, "\n");
        }
    }
    fprintf(fp, "\n");
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

    unsigned int s;
    FILE *fp = fopen("/dev/urandom", "r");
    fread(&s, sizeof(s), 1, fp);
    fclose(fp);
    srand(s);

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
            if (row != p)
            {
                for (col = 0; col < n; col++)
                {
                    mat->data[row * n + col] -= mat->data[p * n + col] * multiplier;
                    id->data[row * n + col] -= id->data[p * n + col] * multiplier;
                }
            }
        }
    }
    return id;
}

/*
void *divide_row(void *arg)
{
    struct thread_arg *p = (struct thread_arg *)arg;
    size_t n = p->M->n;
    double pv;
    size_t index;
    for (size_t k = 0; k < p->count; k++)
    {
        pv = p->M->data[(p->r + k) * n + p->p];
        for (size_t c = 0; c < n; c++)
        {
            index = (p->r + k) * n + c;
            p->M->data[index] /= pv;
            p->I->data[index] /= pv;
        }
    }
}


void *eliminate_forward(void *arg)
{
    struct thread_arg *p = (struct thread_arg *)arg;
    size_t n = p->M->n;
    double multiplier;
    for (size_t k = 0; k < p->count; k++)
    {
        multiplier = p->M->data[(p->r + k) * n + p->p];
        for (size_t c = 0; c < n; c++)
        {
            p->M->data[(k + p->r) * n + c] -= p->M->data[p->p * n + c] * multiplier;
            p->I->data[(k + p->r) * n + c] -= p->I->data[p->p * n + c] * multiplier;
        }
    }
}

void *eliminate_backward(void *arg)
{
    struct thread_arg *p = (struct thread_arg *)arg;
    size_t n = p->M->n;
    double multiplier;
    for (size_t k = 0; k < p->count; k++)
    {
        multiplier = p->M->data[(p->r - k) * n + p->p];
        for (size_t c = 0; c < n; c++)
        {
            p->M->data[(p->r - k) * n + c] -= p->M->data[p->p * n + c] * multiplier;
            p->I->data[(p->r - k) * n + c] -= p->I->data[p->p * n + c] * multiplier;
        }
    }
}

struct matrix *matrix_inverse_parallel(struct matrix *M)
{
    const size_t n = M->n;
    struct matrix *I = matrix_identity(n);

    matrix_write(M, stdout);

    pthread_t threads[MAX_THREAD_COUNT] = {0};
    struct thread_arg args[MAX_THREAD_COUNT];

    for (size_t col = 0; col < n; col++)
    {
        size_t row_count = n - col;
        size_t rows_per_thread = n / MAX_THREAD_COUNT;
        size_t remainder = n % MAX_THREAD_COUNT;
        for (size_t i = 0; i < MAX_THREAD_COUNT; i++)
        {
            args[i].M = M;
            args[i].I = I;
            args[i].count = rows_per_thread;
            args[i].r = col + i * rows_per_thread;
            args[i].p = col;
        }
        if (remainder > 0)
        {
            args[MAX_THREAD_COUNT - 1].count += remainder;
        }

        for (size_t i = 0; i < MAX_THREAD_COUNT; i++)
        {
            pthread_create(&threads[i], NULL, divide_row, &args[i]);
        }

        for (size_t i = 0; i < MAX_THREAD_COUNT; i++)
        {
            pthread_join(threads[i], NULL);
        }

        row_count -= 1;
        if (row_count == 0)
        {
            break;
        }
        rows_per_thread = n / MAX_THREAD_COUNT;
        remainder = n % MAX_THREAD_COUNT;

        for (size_t i = 0; i < MAX_THREAD_COUNT; i++)
        {
            args[i].count = rows_per_thread;
            args[i].r = i * rows_per_thread + col + 1;
            args[i].p = col;
        }
        if (remainder > 0)
        {
            args[0].count += remainder;
        }

        for (size_t i = 0; i < MAX_THREAD_COUNT; i++)
        {
            pthread_create(&threads[i], NULL, eliminate_forward, &args[i]);
        }

        for (size_t i = 0; i < MAX_THREAD_COUNT; i++)
        {
            pthread_join(threads[i], NULL);
        }
    }

    size_t current_row = n - 2;
    for (size_t i = 1; i < n; i++)
    {
        for (size_t j = 1; j < i + 1; j++)
        {
            args[0].r = current_row;
            args[0].p = current_row + j;
            args[0].count = 1;
            eliminate_backward(&args[0]);
        }
        current_row -= 1;
    }

    return I;
}
*/