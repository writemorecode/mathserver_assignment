#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/matrix_test.h"
#include "../include/string_array_test.h"
#include "../include/string_utils_test.h"

typedef bool (*test_case)();

test_case tests[] = {
    string_array_new_test,
    string_array_insert_test,
    append_string_test,
    split_string_test,
    matrix_new_test,
    matrix_identity_test,
    matrix_inverse_test,
};


int main()
{
    const size_t N = sizeof(tests) / sizeof(test_case);

    for (size_t i = 0; i < N; i++) {
        if (tests[i]()) {
            printf("PASS\n");
        } else {
            printf("FAIL\n");
            break;
        }
    }

}
