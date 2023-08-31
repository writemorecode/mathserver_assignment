#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <stdbool.h>

int handle_command(int socket, char *buffer);
char *receive_command(int socket);
bool validate_command(char *command);

#endif
