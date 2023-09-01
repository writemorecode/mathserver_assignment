#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <unistd.h>
#include <stdbool.h>

char *random_alphanumeric_string(const size_t len);
char *strip_newline_from_end(char *s);
void prepend_string(char *str, const char *prefix);
char *get_program_name(char *command);
bool string_has_prefix(const char *str, const char *prefix);
char *append_string(char *s, char *suffix);

#endif
