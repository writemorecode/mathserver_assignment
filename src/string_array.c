#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "../include/string.h"
#include "../include/string_array.h"

struct string_array* string_array_new(size_t capacity_)
{
    struct string_array* arr = calloc(1, sizeof(struct string_array));
    arr->data = calloc(capacity_, sizeof(char*));
    arr->capacity = capacity_;
    arr->size = 0;
    return arr;
}

void string_array_free(struct string_array* arr)
{
    for (size_t i = 0; i < arr->size; i++) {
        free(arr->data[i]);
    }
    free(arr->data);
    free(arr);
}

void string_array_insert(struct string_array* arr, char* cstr)
{
    if (arr == NULL || cstr == NULL) {
        return;
    }
    if (arr->size == arr->capacity) {
        if (arr->capacity == 0) {
            arr->capacity += 1;
        } else {
            arr->capacity *= 2;
        }
        void* ret = realloc(arr->data, sizeof(char*) * arr->capacity);
        if (ret == NULL) {
            perror("realloc");
            return;
        } else {
            arr->data = ret;
        }
    }

    arr->data[arr->size] = cstr;
    arr->size += 1;
}

size_t count_char_in_string(char* str, char c)
{
    size_t count = 0;
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == c) {
            count++;
        }
    }
    return count;
}

ssize_t find_next_char(char* str, char c)
{
    size_t i;
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == c) {
            break;
        }
    }
    return i;
}

struct string_array* split_string(char* str, char delim)
{
    if (str == NULL) {
        return NULL;
    }
    size_t delim_count = count_char_in_string(str, delim);
    size_t token_count = delim_count + 1;

    struct string_array* arr = string_array_new(token_count + 1);

    size_t i = 0;
    size_t n = strlen(str);
    while (i < n) {
        // size_t next_delim_index = strcspn(str + i, &delim);
        size_t next_delim_index = find_next_char(str + i, ' ');
        char* buf = calloc(sizeof(char), next_delim_index + 1);
        memcpy(buf, str + i, next_delim_index);
        string_array_insert(arr, buf);
        i += next_delim_index + 1;
    }

    arr->data[arr->size] = NULL;
    return arr;
}
