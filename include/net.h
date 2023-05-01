#ifndef NET_H
#define NET_H

#include <stddef.h>

char *read_full(int fd, size_t length);
int write_full(int fd, char *buffer, size_t length);

#endif
