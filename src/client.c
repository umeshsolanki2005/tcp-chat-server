#include "client.h"
#include "logger.h"
#include "utils.h"

static void print_help(void) {
    printf("Supported commands:\n");
    printf("/help\n/users\n/msg <username> <message>\n/all <message>\n/logout\n/quit\n");
}

void *client_receive_loop(void *arg) {
    client_state_t *state = (client_state_t *)arg;
    chat_message_t message;
    char buffer[MAX_BUFFER];

    while (state != NULL && state->running) {
        ssize_t received = recv(state->socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("recv");
            break;
        }
        if (received == 0) {
            printf("Server disconnected.\n");
            break;
        }

        buffer[received] = '\0';
        if (deserialize_message(buffer, (size_t)received, &message) != 0) {
            printf("Received malformed message.\n");
            continue;
        }

        if (message.type == MSG_PUBLIC) {
            printf("[Public][%s]: %s\n", message.sender, message.content);
        } else if (message.type == MSG_PRIVATE) {
            printf("[Private from %s]: %s\n", message.sender, message.content);
        } else if (message.type == MSG_USER_LIST) {
            printf("Connected users: %s\n", message.content);
        } else if (message.type == MSG_LOGIN_SUCCESS || message.type == MSG_REGISTER_SUCCESS) {
            printf("%s\n", message.content);
            state->authenticated = true;
        } else if (message.type == MSG_LOGIN_FAILURE || message.type == MSG_REGISTER_FAILURE || message.type == MSG_ERROR) {
            printf("%s\n", message.content);
        } else if (message.type == MSG_INFO) {
            printf("%s\n", message.content);
        }
    }

    state->running = false;
    return NULL;
}

int client_start(const char *server_ip, const char *port_str) {
    int port = 0;
    int sockfd = -1;
    struct sockaddr_in server_addr;
    client_state_t state;
    pthread_t thread;
    char input[MAX_BUFFER];

    if (utils_parse_port(port_str, &port) != 0) {
        fprintf(stderr, "Invalid port: %s\n", port_str);
        return EXIT_FAILURE;
    }

    memset(&state, 0, sizeof(state));
    memset(&server_addr, 0, sizeof(server_addr));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return EXIT_FAILURE;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return EXIT_FAILURE;
    }

    state.socket_fd = sockfd;
    state.running = true;
    if (pthread_create(&thread, NULL, client_receive_loop, &state) != 0) {
        perror("pthread_create");
        close(sockfd);
        return EXIT_FAILURE;
    }

    while (state.running) {
        printf("Enter command: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }
        utils_trim_newline(input);
        if (strcmp(input, "") == 0) {
            continue;
        }

        if (strcmp(input, "/help") == 0) {
            print_help();
            continue;
        }

        if (strcmp(input, "/quit") == 0) {
            break;
        }

        if (strcmp(input, "/logout") == 0) {
            chat_message_t message;
            prepare_message(&message, MSG_LOGOUT, state.username, "", "");
            send_all(sockfd, &message, sizeof(message));
            state.authenticated = false;
            state.username[0] = '\0';
            continue;
        }

        if (!state.authenticated) {
            char username[MAX_USERNAME];
            char password[MAX_MESSAGE];
            chat_message_t message;
            if (strncmp(input, "/register ", 10) == 0) {
                char *rest = input + 10;
                char *space = strchr(rest, ' ');
                if (space == NULL) {
                    printf("Usage: /register <username> <password>\n");
                    continue;
                }
                *space = '\0';
                snprintf(username, sizeof(username), "%s", rest);
                snprintf(password, sizeof(password), "%s", space + 1);
                prepare_message(&message, MSG_REGISTER, username, "", password);
                send_all(sockfd, &message, sizeof(message));
                continue;
            }
            if (strncmp(input, "/login ", 7) == 0) {
                char *rest = input + 7;
                char *space = strchr(rest, ' ');
                if (space == NULL) {
                    printf("Usage: /login <username> <password>\n");
                    continue;
                }
                *space = '\0';
                snprintf(username, sizeof(username), "%s", rest);
                snprintf(password, sizeof(password), "%s", space + 1);
                prepare_message(&message, MSG_LOGIN, username, "", password);
                send_all(sockfd, &message, sizeof(message));
                continue;
            }
            printf("Please login or register first.\n");
            continue;
        }

        if (strncmp(input, "/msg ", 5) == 0) {
            char command[MAX_USERNAME];
            char arg1[MAX_USERNAME];
            char arg2[MAX_MESSAGE];
            chat_message_t message;
            if (utils_parse_chat_command(input, command, sizeof(command), arg1, sizeof(arg1), arg2, sizeof(arg2)) != 0) {
                printf("Invalid command format.\n");
                continue;
            }
            prepare_message(&message, MSG_PRIVATE, state.username, arg1, arg2);
            send_all(sockfd, &message, sizeof(message));
            continue;
        }

        if (strncmp(input, "/all ", 5) == 0) {
            chat_message_t message;
            prepare_message(&message, MSG_PUBLIC, state.username, "", input + 5);
            send_all(sockfd, &message, sizeof(message));
            continue;
        }

        if (strcmp(input, "/users") == 0) {
            chat_message_t message;
            prepare_message(&message, MSG_USER_LIST, state.username, "", "");
            send_all(sockfd, &message, sizeof(message));
            continue;
        }

        printf("Unknown command. Use /help to view available commands.\n");
    }

    close(sockfd);
    return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }
    return client_start(argv[1], argv[2]);
}
