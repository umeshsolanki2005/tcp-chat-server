#ifndef CLIENT_H
#define CLIENT_H

#include "common.h"
#include "protocol.h"

typedef struct {
    int socket_fd;
    pthread_t receive_thread;
    bool running;
    bool authenticated;
    char username[MAX_USERNAME];
} client_state_t;

int client_start(const char *server_ip, const char *port_str);
void client_cleanup(client_state_t *state);
void *client_receive_loop(void *arg);

#endif
