#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include "protocol.h"

typedef struct {
    int socket_fd;
    pthread_t thread_id;
    char username[MAX_USERNAME];
    int authenticated;
    struct sockaddr_in address;
} connected_client_t;

typedef struct {
    int listening_socket;
    volatile sig_atomic_t shutdown_requested;
    connected_client_t clients[MAX_CLIENTS];
    pthread_mutex_t clients_mutex;
    pthread_mutex_t auth_mutex;
    int client_count;
} server_state_t;

void server_init_state(server_state_t *state, int listening_socket);
void server_cleanup(server_state_t *state);
int server_start(const char *port_str);
int server_add_client(server_state_t *state, int client_socket, const struct sockaddr_in *address);
void server_remove_client(server_state_t *state, int socket_fd);
connected_client_t *server_find_client_by_socket(server_state_t *state, int socket_fd);
connected_client_t *server_find_client_by_username(server_state_t *state, const char *username);
void server_broadcast(server_state_t *state, const chat_message_t *message, int exclude_socket);
void server_send_user_list(server_state_t *state, int socket_fd);
void server_notify_shutdown(server_state_t *state);

#endif
