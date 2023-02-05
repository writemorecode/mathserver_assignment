#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Returns a pseudo-random alphanumeric string of length "len". */
char *random_alphanumeric_string(const size_t len)
{
    srand(time(NULL));
    const char *a = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t a_len = strlen(a);
    size_t index;
    char *s = calloc(len + 1, sizeof(char));
    for (size_t i = 0; i < len; i++)
    {
        index = rand() % a_len;
        s[i] = a[index];
    }
    return s;
}

/*
    Returns a newly allocated string containing
    the string "str" prepended with "prefix".
*/
char *prepend_string(char *str, const char *prefix)
{
    if (str == NULL || prefix == NULL)
    {
        return NULL;
    }
    const size_t str_len = strlen(str);
    const size_t prefix_len = strlen(prefix);
    const size_t total_len = str_len + prefix_len;

    char *ret = realloc(str, total_len + 1);
    if(ret == NULL)
    {
        return NULL;
    }
    str = ret;
    ret = NULL;

    for(size_t i = 0; i < str_len; i++)
    {
        str[total_len - 1 - i] = str[str_len - 1 - i];
    }

    for(size_t i = 0; i < prefix_len; i++)
    {
        str[i] = prefix[i];
    }

    str[total_len] = 0;
    return str;
}

/*
    Removes a newline character from the end of "s".
    Does nothing if newline not found.
*/
void strip_newline_from_end(char *s)
{
    if (s == NULL)
    {
        return;
    }
    size_t s_len = strlen(s);
    if (s[s_len - 1] == '\n')
    {
        s[s_len - 1] = 0;
    }
}
