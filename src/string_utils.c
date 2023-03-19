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
void prepend_string(char *str, const char *prefix)
{
    if (str == NULL || prefix == NULL)
    {
        return;
    }
    const size_t str_len = strlen(str);
    const size_t prefix_len = strlen(prefix);

    for (size_t i = 0; i < str_len; i++)
    {
        for (size_t j = 0; j < prefix_len; j++)
        {
            str[str_len + j - i] = str[str_len - 1 - i];
        }
    }

    for (size_t i = 0; i < prefix_len; i++)
    {
        str[i] = prefix[i];
    }
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

/*
    Given a string containing a command, e.g.
    "matinvpar -n 4" or "kmeanspar -k 3 -f kmeans-data.txt",
    returns the name of the program, or NULL if not found.
*/
char *get_program_name(char *command)
{
    if (command == NULL)
    {
        return NULL;
    }
    size_t program_name_length = 0;
    char *first_space = strchr(command, ' ');
    if (first_space == NULL)
    {
        program_name_length = strlen(command);
    }
    else
    {
        program_name_length = first_space - command;
    }

    char *buf = calloc(program_name_length + 1, sizeof(char));
    if (buf == NULL)
    {
        return NULL;
    }
    memcpy(buf, command, program_name_length);

    return buf;
}
