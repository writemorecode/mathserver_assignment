#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "../include/string.h"
#include "../include/string_array.h"

struct string_array *string_array_new(size_t capacity_)
{
    struct string_array *arr = calloc(1, sizeof(struct string_array));
    arr->data = calloc(capacity_, sizeof(char *));
    arr->capacity = capacity_;
    arr->size = 0;
    return arr;
}

void string_array_free(struct string_array *arr)
{
    for (size_t i = 0; i < arr->size; i++)
    {
        free(arr->data[i]);
    }
    // Free the terminating NULL pointer at end of array.
    free(arr->data[arr->size]);
    free(arr->data);
    free(arr);
}

void string_array_insert(struct string_array *arr, char *cstr)
{
    if (arr == NULL || cstr == NULL)
    {
        return;
    }
    if (arr->size == arr->capacity)
    {
        if (arr->capacity == 0)
        {
            arr->capacity += 1;
        }
        else
        {
            arr->capacity *= 2;
        }
        void *ret = realloc(arr->data, sizeof(char *) * (arr->capacity + 1));
        if (ret == NULL)
        {
            perror("realloc");
            return;
        }
        else
        {
            arr->data = ret;
        }
    }

    size_t cstr_len = strlen(cstr);
    arr->data[arr->size] = calloc(cstr_len + 1, sizeof(char));
    memcpy(arr->data[arr->size], cstr, cstr_len);
    arr->data[arr->size][cstr_len] = 0;
    arr->size += 1;
}

/*
    Constructs a string_array from tokens in "str" that are delimited by "delim".
*/
struct string_array *string_array_from_string(char *str, char *delim)
{
    if (str == NULL)
    {
        return NULL;
    }
    char *endptr;
    char *token = strtok_r(str, delim, &endptr);
    if (token == NULL)
    {
        return NULL;
    }
    struct string_array *arr = string_array_new(1);
    string_array_insert(arr, token);
    while (token != NULL)
    {
        token = strtok_r(NULL, delim, &endptr);
        if (token != NULL)
        {
            string_array_insert(arr, token);
        }
    }

    // Add a NULL pointer to end of array
    void *p = realloc(arr->data, sizeof(char *) * (arr->capacity + 1));
    if (p == NULL)
    {
        string_array_free(arr);
        perror("realloc");
        return NULL;
    }
    arr->data = p;
    arr->data[arr->size] = NULL;

    return arr;
}
