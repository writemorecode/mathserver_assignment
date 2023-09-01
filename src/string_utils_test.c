#include "../include/string_utils.h"
#include "../include/string_utils_test.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

bool append_string_test() {
    char *str1 = strdup("hello ");
    char *suffix1 = strdup("world");
    char *app1 = append_string(str1, suffix1); 
    char *app1_correct = "hello world";

    if (strcmp(app1, app1_correct) != 0) {
        return false;
    }

    free(suffix1);
    free(app1);

    return true;
}
