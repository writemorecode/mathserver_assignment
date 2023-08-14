#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../include/string_array_test.h"
#include "../include/string_array.h"

int main() {
    bool pass = true;

    pass = string_array_new_test();
    pass = string_array_insert_test();
    pass = split_string_test();

    if (pass) {
        fprintf(stdout, "PASS\n");
    }

}
