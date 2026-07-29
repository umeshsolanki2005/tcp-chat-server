#include "server.h"
#include "auth.h"
#include "logger.h"
#include "utils.h"

static server_state_t *g_server_state = NULL;
static pthread_mutex_t g_shutdown_mutex = PTHREAD_MUTEX_INITIALIZER;

static void handle_signal(int signum) {
    (void)signum;
    if (g_server_state != NULL) {
        g_server_state->shutdown_requested = 1;
    }
}

static void *handle_client(void *arg) {
    int client_socket = -1;
    connected_client_t *client = NULL;
    server_state_t *state = NULL;
    chat_message_t message;
    char input_buffer[MAX_BUFFER];
    int keep_running = 1;

    if (arg == NULL) {
        pthread_exit(NULL);
    }

    client = (connected_client_t *)arg;
    state = g_server_state;
    client_socket = client->socket_fd;

    while (keep_running && state != NULL && !state->shutdown_requested) {
        ssize_t received = recv(client_socket, input_buffer, sizeof(input_buffer) - 1, 0);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recv");
            break;
        }
        if (received == 0) {
            break;
        }

        input_buffer[received] = '\0';
        if (deserialize_message(input_buffer, (size_t)received, &message) != 0) {
            logger_log("ERROR", "Malformed message from %s", client->username[0] ? client->username : "unknown");
            continue;
        }

        if (message.type == MSG_LOGOUT) {
            if (client->authenticated) {
                logger_log("INFO", "Logout: %s", client->username);
                client->authenticated = 0;
                memset(client->username, 0, sizeof(client->username));
            }
            break;
        }

        if (message.type == MSG_REGISTER) {
            int result = -1;
            pthread_mutex_lock(&state->auth_mutex);
            result = auth_register_user(USER_DB_PATH, message.sender, message.content);
            pthread_mutex_unlock(&state->auth_mutex);
            if (result == 0) {
                prepare_message(&message, MSG_REGISTER_SUCCESS, "server", client->username, "Registration successful");
            } else {
                prepare_message(&message, MSG_REGISTER_FAILURE, "server", client->username, strerror(errno));
            }
            send_all(client_socket, &message, sizeof(message));
            continue;
        }

        if (message.type == MSG_LOGIN) {
            int result = -1;
            pthread_mutex_lock(&state->auth_mutex);
            result = auth_login_user(USER_DB_PATH, message.sender, message.content);
            pthread_mutex_unlock(&state->auth_mutex);
            if (result == 0) {
                connected_client_t *existing = server_find_client_by_username(state, message.sender);
                if (existing != NULL && existing->socket_fd != client_socket) {
                    prepare_message(&message, MSG_LOGIN_FAILURE, "server", message.sender, "Account already logged in");
                    send_all(client_socket, &message, sizeof(message));
                    continue;
                }
                snprintf(client->username, sizeof(client->username), "%s", message.sender);
                client->authenticated = 1;
                prepare_message(&message, MSG_LOGIN_SUCCESS, "server", client->username, "Login successful");
                logger_log("INFO", "Login successful: %s", client->username);
            } else {
                prepare_message(&message, MSG_LOGIN_FAILURE, "server", message.sender, "Invalid credentials");
                logger_log("WARNING", "Login failed: %s", message.sender);
            }
            send_all(client_socket, &message, sizeof(message));
            continue;
        }

        if (!client->authenticated) {
            prepare_message(&message, MSG_ERROR, "server", client->username, "Authenticate first");
            send_all(client_socket, &message, sizeof(message));
            continue;
        }

        if (message.type == MSG_PUBLIC) {
            prepare_message(&message, MSG_PUBLIC, client->username, "", message.content);
            server_broadcast(state, &message, client_socket);
        } else if (message.type == MSG_PRIVATE) {
            connected_client_t *receiver = server_find_client_by_username(state, message.receiver);
            if (receiver == NULL || !receiver->authenticated) {
                prepare_message(&message, MSG_ERROR, "server", client->username, "User offline or not found");
                send_all(client_socket, &message, sizeof(message));
            } else {
                chat_message_t reply;
                prepare_message(&reply, MSG_PRIVATE, client->username, receiver->username, message.content);
                send_all(receiver->socket_fd, &reply, sizeof(reply));
                prepare_message(&message, MSG_PRIVATE, client->username, receiver->username, message.content);
                send_all(client_socket, &message, sizeof(message));
            }
        } else if (message.type == MSG_USER_LIST) {
            server_send_user_list(state, client_socket);
        } else {
            prepare_message(&message, MSG_ERROR, "server", client->username, "Unknown message type");
            send_all(client_socket, &message, sizeof(message));
        }
    }

    server_remove_client(state, client_socket);
    shutdown(client_socket, SHUT_RDWR);
    close(client_socket);
    logger_log("INFO", "Client disconnected: %s", client->username[0] ? client->username : "unknown");
    pthread_exit(NULL);
}

static void *accept_loop(void *arg) {
    server_state_t *state = (server_state_t *)arg;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while (!state->shutdown_requested) {
        int client_socket = accept(state->listening_socket, (struct sockaddr *)&client_addr, &addr_len);
        if (client_socket < 0) {
            if (errno == EINTR) {
                continue;
            }
            if (!state->shutdown_requested) {
                perror("accept");
            }
            break;
        }

        logger_log("INFO", "Client connected: %s", inet_ntoa(client_addr.sin_addr));
        if (server_add_client(state, client_socket, &client_addr) != 0) {
            close(client_socket);
            continue;
        }

        pthread_t thread_id;
        connected_client_t *client = server_find_client_by_socket(state, client_socket);
        if (client == NULL) {
            close(client_socket);
            continue;
        }
        client->thread_id = thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client) != 0) {
            perror("pthread_create");
            server_remove_client(state, client_socket);
            close(client_socket);
            continue;
        }
        if (pthread_detach(thread_id) != 0) {
            perror("pthread_detach");
        }
    }

    return NULL;
}

int server_start(const char *port_str) {
    int port = 0;
    int listening_socket = -1;
    struct sockaddr_in server_addr;
    server_state_t state;
    pthread_t accept_thread;
    struct sigaction sa;

    if (utils_parse_port(port_str, &port) != 0) {
        fprintf(stderr, "Invalid port: %s\n", port_str);
        return EXIT_FAILURE;
    }

    memset(&state, 0, sizeof(state));
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&sa, 0, sizeof(sa));

    listening_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listening_socket < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    int opt = 1;
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(listening_socket);
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons((uint16_t)port);

    if (bind(listening_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(listening_socket);
        return EXIT_FAILURE;
    }

    if (listen(listening_socket, SERVER_BACKLOG) < 0) {
        perror("listen");
        close(listening_socket);
        return EXIT_FAILURE;
    }

    logger_init(LOG_PATH);
    logger_log("INFO", "Server started on port %d", port);

    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) != 0) {
        perror("sigaction");
        close(listening_socket);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    server_init_state(&state, listening_socket);
    g_server_state = &state;

    if (pthread_create(&accept_thread, NULL, accept_loop, &state) != 0) {
        perror("pthread_create");
        server_cleanup(&state);
        close(listening_socket);
        return EXIT_FAILURE;
    }

    if (pthread_join(accept_thread, NULL) != 0) {
        perror("pthread_join");
    }

    server_notify_shutdown(&state);
    server_cleanup(&state);
    close(listening_socket);
    logger_log("INFO", "Server shutdown complete");
    logger_close();
    return EXIT_SUCCESS;
}

void server_init_state(server_state_t *state, int listening_socket) {
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->listening_socket = listening_socket;
    pthread_mutex_init(&state->clients_mutex, NULL);
    pthread_mutex_init(&state->auth_mutex, NULL);
}

void server_cleanup(server_state_t *state) {
    int i;
    if (state == NULL) {
        return;
    }
    pthread_mutex_lock(&state->clients_mutex);
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (state->clients[i].socket_fd >= 0) {
            close(state->clients[i].socket_fd);
            state->clients[i].socket_fd = -1;
        }
    }
    pthread_mutex_unlock(&state->clients_mutex);
    pthread_mutex_destroy(&state->clients_mutex);
    pthread_mutex_destroy(&state->auth_mutex);
}

int server_add_client(server_state_t *state, int client_socket, const struct sockaddr_in *address) {
    int i;
    if (state == NULL || address == NULL) {
        errno = EINVAL;
        return -1;
    }
    pthread_mutex_lock(&state->clients_mutex);
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (state->clients[i].socket_fd < 0) {
            memset(&state->clients[i], 0, sizeof(state->clients[i]));
            state->clients[i].socket_fd = client_socket;
            state->clients[i].authenticated = 0;
            state->clients[i].address = *address;
            state->clients[i].thread_id = 0;
            state->clients[i].username[0] = '\0';
            state->client_count++;
            pthread_mutex_unlock(&state->clients_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&state->clients_mutex);
    errno = ENOMEM;
    return -1;
}

void server_remove_client(server_state_t *state, int socket_fd) {
    int i;
    if (state == NULL) {
        return;
    }
    pthread_mutex_lock(&state->clients_mutex);
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (state->clients[i].socket_fd == socket_fd) {
            state->clients[i].socket_fd = -1;
            memset(state->clients[i].username, 0, sizeof(state->clients[i].username));
            state->clients[i].authenticated = 0;
            state->client_count = (state->client_count > 0) ? state->client_count - 1 : 0;
            break;
        }
    }
    pthread_mutex_unlock(&state->clients_mutex);
}

connected_client_t *server_find_client_by_socket(server_state_t *state, int socket_fd) {
    int i;
    connected_client_t *result = NULL;
    if (state == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&state->clients_mutex);
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (state->clients[i].socket_fd == socket_fd) {
            result = &state->clients[i];
            break;
        }
    }
    pthread_mutex_unlock(&state->clients_mutex);
    return result;
}

connected_client_t *server_find_client_by_username(server_state_t *state, const char *username) {
    int i;
    connected_client_t *result = NULL;
    if (state == NULL || username == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&state->clients_mutex);
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (state->clients[i].socket_fd >= 0 && state->clients[i].authenticated && strcmp(state->clients[i].username, username) == 0) {
            result = &state->clients[i];
            break;
        }
    }
    pthread_mutex_unlock(&state->clients_mutex);
    return result;
}

void server_broadcast(server_state_t *state, const chat_message_t *message, int exclude_socket) {
    int i;
    if (state == NULL || message == NULL) {
        return;
    }
    pthread_mutex_lock(&state->clients_mutex);
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (state->clients[i].socket_fd >= 0 && state->clients[i].authenticated && state->clients[i].socket_fd != exclude_socket) {
            int fd = state->clients[i].socket_fd;
            pthread_mutex_unlock(&state->clients_mutex);
            if (send_all(fd, message, sizeof(*message)) != 0) {
                logger_log("ERROR", "Failed to broadcast to %s", state->clients[i].username);
            }
            pthread_mutex_lock(&state->clients_mutex);
        }
    }
    pthread_mutex_unlock(&state->clients_mutex);
}

void server_send_user_list(server_state_t *state, int socket_fd) {
    int i;
    char user_list[MAX_MESSAGE];
    chat_message_t message;
    if (state == NULL) {
        return;
    }
    memset(user_list, 0, sizeof(user_list));
    pthread_mutex_lock(&state->clients_mutex);
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (state->clients[i].socket_fd >= 0 && state->clients[i].authenticated) {
            size_t offset = strlen(user_list);
            if (offset > 0) {
                snprintf(user_list + offset, sizeof(user_list) - offset, ",%s", state->clients[i].username);
            } else {
                snprintf(user_list, sizeof(user_list), "%s", state->clients[i].username);
            }
        }
    }
    pthread_mutex_unlock(&state->clients_mutex);
    prepare_message(&message, MSG_USER_LIST, "server", "", user_list);
    send_all(socket_fd, &message, sizeof(message));
}

void server_notify_shutdown(server_state_t *state) {
    int i;
    if (state == NULL) {
        return;
    }
    pthread_mutex_lock(&state->clients_mutex);
    for (i = 0; i < MAX_CLIENTS; ++i) {
        if (state->clients[i].socket_fd >= 0) {
            int fd = state->clients[i].socket_fd;
            pthread_mutex_unlock(&state->clients_mutex);
            send_all(fd, "shutdown", 8);
            shutdown(fd, SHUT_RDWR);
            close(fd);
            pthread_mutex_lock(&state->clients_mutex);
        }
    }
    pthread_mutex_unlock(&state->clients_mutex);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    return server_start(argv[1]);
}
