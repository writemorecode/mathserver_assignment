#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "../include/string_array_test.h"
#include "../include/string_array.h"

bool string_array_new_test(void) {
    const size_t testCapacity = 8;
    struct string_array *arr = string_array_new(testCapacity);
    if (arr == NULL) {
        fprintf(stderr, "FAIL: 'string_array_new' returned NULL.\n");
        return false;
    }

    if (arr->data == NULL) {
        fprintf(stderr, "FAIL: 'string_array_new' returned incorrectly allocated array.\n");
        return false;
    }

    if (arr->capacity != testCapacity) {
        fprintf(stderr, "FAIL: 'string_array_new' returned array with incorrect capacity.\n");
        return false;
    }

    if (arr->size != 0) {
        fprintf(stderr, "FAIL: 'string_array_new' returned array with incorrect size.\n");
        return false;
    }

    string_array_free(arr);

    return true;
}

bool string_array_insert_test() {
    // Initially, array has capacity 0 and size 0.
    struct string_array *arr = string_array_new(0);

    char *test_string = strdup("foo");

    string_array_insert(arr, test_string);

    // After this insert, array shall have capacity 1 and size 1.

    if (arr->capacity != 1) {
        fprintf(stderr, "FAIL: 'string_array_insert' resulted in array with incorrect capacity.\n");
        return false;
    }

    if (arr->size != 1) {
        fprintf(stderr, "FAIL: 'string_array_insert' resulted in array with incorrect size.\n");
        return false;
    }

    if (strncmp(arr->data[0], test_string, strlen(test_string)) != 0) {
        fprintf(stderr, "FAIL: String passed to 'string_array_insert' does not match string in array.\n");
        return false;
    }
         
    string_array_free(arr);

    return true;
}

bool split_string_test() {
    // The string contains 9 words 
    char *test_string = strdup("the small orange cat sat neatly on the mat");
    char *test_string_words[9] = {
        "the",
        "small",
        "orange",
        "cat",
        "sat",
        "neatly",
        "on",
        "the",
        "mat"
    };

    struct string_array *arr = split_string(test_string, ' ');
    if (arr == NULL) {
        fprintf(stderr, "FAIL: 'split_string' returned NULL.\n");
        return false;
    }

    if (arr->size != 9) {
        fprintf(stderr, "FAIL: 'split_string' returned array with incorrect number of elements.\n");
        return false;
    }

    for (int i = 0; i < 9; i++) {
        if (strncmp(arr->data[i], test_string_words[i], strlen(test_string_words[i])) != 0) {
            fprintf(stderr, "FAIL: Incorrect token found in array returned from 'split_string'.\n");
            return false;
        }
    }
    
    string_array_free(arr);
    free(test_string);

    return true;
}

