#ifndef UTILS_H
#define UTILS_H

#include "common.h"

int utils_parse_port(const char *value, int *port);
void utils_trim_newline(char *text);
int utils_parse_chat_command(const char *input, char *command, size_t command_size, char *arg1, size_t arg1_size, char *arg2, size_t arg2_size);

#endif
