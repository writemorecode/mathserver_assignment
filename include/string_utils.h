#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <unistd.h>

char *random_alphanumeric_string(const size_t len);
char *strip_newline_from_end(char *s);
void prepend_string(char *str, const char *prefix);
char *get_program_name(char *command);

#endif
