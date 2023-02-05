#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <unistd.h>

char *random_alphanumeric_string(const size_t len);
char *strip_newline_from_end(char *s);
char *prepend_string(char *str, const char *prefix);

#endif
