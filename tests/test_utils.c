#include "utils.h"
#include <assert.h>

int main(void) {
    int port = 0;
    char command[16];
    char arg1[32];
    char arg2[64];

    assert(utils_parse_port("8080", &port) == 0);
    assert(port == 8080);
    assert(utils_parse_port("abc", &port) != 0);

    utils_parse_chat_command("/msg alice hello", command, sizeof(command), arg1, sizeof(arg1), arg2, sizeof(arg2));
    assert(strcmp(command, "/msg") == 0);
    assert(strcmp(arg1, "alice") == 0);
    assert(strcmp(arg2, "hello") == 0);
    return 0;
}
