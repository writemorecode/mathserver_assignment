#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../include/matrix.h"

struct settings
{
    size_t N;
    double maxnum;
    bool fast;
    bool print;
    char *filename;
};

struct settings *read_settings(int argc, char **argv)
{
    struct settings *settings = calloc(1, sizeof(struct settings));

    settings->N = 5;
    settings->maxnum = 15.0;
    settings->fast = true;
    settings->print = true;
    settings->filename = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "n:m:I:P:o:")) != -1)
    {
        if (opt == 'n')
        {
            settings->N = atoi(optarg);
        }
        else if (opt == 'm')
        {
            settings->maxnum = atof(optarg);
        }
        else if (opt == 'P')
        {
            if (strcmp(optarg, "1") == 0)
            {
                settings->print = true;
            }
            else if (strcmp(optarg, "0") == 0)
            {
                settings->print = false;
            }
        }
        else if (opt == 'I')
        {
            if (strcmp(optarg, "fast") == 0)
            {
                settings->fast = true;
            }
            else if (strcmp(optarg, "rand") == 0)
            {
                settings->fast = false;
            }
        }
        else if (opt == 'o')
        {
            settings->filename = strdup(optarg);
        }
    }
    return settings;
}

int main(int argc, char **argv)
{
    struct settings *settings = read_settings(argc, argv);
    if (settings == NULL)
    {
        return EXIT_FAILURE;
    }

    struct matrix *A;
    if (settings->fast == true)
    {
        A = matrix_create_fast(settings->N);
    }
    else
    {
        A = matrix_random(settings->N, settings->maxnum);
    }

    FILE *fp;
    if (settings->filename == NULL)
    {
        fp = stdout;
    }
    else
    {
        fp = fopen(settings->filename, "w");
        if (fp == NULL)
        {
            perror("fopen");
            exit(EXIT_FAILURE);
        }
    }

    struct matrix *A_copy = matrix_copy(A);

    matrix_write(A);

    struct matrix *A_inv = matrix_inverse(A);

    matrix_write(A_inv);

    matrix_free(A_inv);
    matrix_free(A);
    matrix_free(A_copy);

    if (fp != stdout)
    {
        fclose(fp);
    }

    free(settings->filename);
    free(settings);
}
