#ifndef STRING_ARRAY_H
#define STRING_ARRAY_H

#include <stddef.h>
#include "string.h"

struct string_array
{
    size_t size;
    size_t capacity;
    char **data;
};

struct string_array *string_array_new(size_t capacity_);
void string_array_free(struct string_array *arr);
void string_array_insert(struct string_array *arr, char *str);
struct string_array *string_array_from_string(char *str, char *delim);

struct string_array *split_string(char *str, char delim);

#endif
