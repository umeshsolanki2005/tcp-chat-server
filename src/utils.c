#include "utils.h"

int utils_parse_port(const char *value, int *port) {
    char *end = NULL;
    long parsed = 0;

    if (value == NULL || port == NULL) {
        errno = EINVAL;
        return -1;
    }

    errno = 0;
    parsed = strtol(value, &end, 10);
    if (errno != 0 || end == value || *end != '\0' || parsed < 1 || parsed > 65535) {
        errno = EINVAL;
        return -1;
    }

    *port = (int)parsed;
    return 0;
}

void utils_trim_newline(char *text) {
    size_t length;

    if (text == NULL) {
        return;
    }

    length = strlen(text);
    while (length > 0 && (text[length - 1] == '\n' || text[length - 1] == '\r')) {
        text[length - 1] = '\0';
        length--;
    }
}

int utils_parse_chat_command(const char *input, char *command, size_t command_size, char *arg1, size_t arg1_size, char *arg2, size_t arg2_size) {
    char buffer[MAX_BUFFER];
    char *token;
    int token_index = 0;
    char *save_ptr = NULL;

    if (input == NULL || command == NULL || arg1 == NULL || arg2 == NULL) {
        errno = EINVAL;
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));
    strncpy(buffer, input, sizeof(buffer) - 1);
    utils_trim_newline(buffer);

    token = strtok_r(buffer, " ", &save_ptr);
    while (token != NULL && token_index < 3) {
        if (token_index == 0) {
            snprintf(command, command_size, "%s", token);
        } else if (token_index == 1) {
            snprintf(arg1, arg1_size, "%s", token);
        } else if (token_index == 2) {
            snprintf(arg2, arg2_size, "%s", token);
        }
        token = strtok_r(NULL, " ", &save_ptr);
        token_index++;
    }

    return 0;
}
