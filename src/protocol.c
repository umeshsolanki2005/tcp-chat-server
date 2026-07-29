#include "protocol.h"

static size_t message_size(const chat_message_t *message) {
    return sizeof(uint32_t) + sizeof(char) * MAX_USERNAME + sizeof(char) * MAX_USERNAME + sizeof(char) * MAX_MESSAGE;
}

int send_all(int fd, const void *buffer, size_t length) {
    const char *ptr = (const char *)buffer;
    size_t total_sent = 0;

    while (total_sent < length) {
        ssize_t sent = send(fd, ptr + total_sent, length - total_sent, 0);
        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (sent == 0) {
            return -1;
        }
        total_sent += (size_t)sent;
    }

    return 0;
}

int recv_all(int fd, void *buffer, size_t length) {
    char *ptr = (char *)buffer;
    size_t total_received = 0;

    while (total_received < length) {
        ssize_t received = recv(fd, ptr + total_received, length - total_received, 0);
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return -1;
        }
        if (received == 0) {
            return 0;
        }
        total_received += (size_t)received;
    }

    return 1;
}

void prepare_message(chat_message_t *message, uint32_t type, const char *sender, const char *receiver, const char *content) {
    if (message == NULL) {
        return;
    }

    memset(message, 0, sizeof(*message));
    message->type = type;
    if (sender != NULL) {
        snprintf(message->sender, sizeof(message->sender), "%s", sender);
    }
    if (receiver != NULL) {
        snprintf(message->receiver, sizeof(message->receiver), "%s", receiver);
    }
    if (content != NULL) {
        snprintf(message->content, sizeof(message->content), "%s", content);
    }
}

int serialize_message(const chat_message_t *message, char *buffer, size_t buffer_size, size_t *written) {
    uint32_t network_type;
    size_t needed = message_size(message);

    if (message == NULL || buffer == NULL || written == NULL || buffer_size < needed) {
        errno = EINVAL;
        return -1;
    }

    memset(buffer, 0, buffer_size);
    network_type = htonl(message->type);
    memcpy(buffer, &network_type, sizeof(network_type));
    memcpy(buffer + sizeof(network_type), message->sender, MAX_USERNAME);
    memcpy(buffer + sizeof(network_type) + MAX_USERNAME, message->receiver, MAX_USERNAME);
    memcpy(buffer + sizeof(network_type) + MAX_USERNAME + MAX_USERNAME, message->content, MAX_MESSAGE);
    *written = needed;
    return 0;
}

int deserialize_message(const char *buffer, size_t buffer_size, chat_message_t *message) {
    uint32_t network_type;
    size_t needed = sizeof(uint32_t) + MAX_USERNAME + MAX_USERNAME + MAX_MESSAGE;

    if (buffer == NULL || message == NULL || buffer_size < needed) {
        errno = EINVAL;
        return -1;
    }

    memset(message, 0, sizeof(*message));
    memcpy(&network_type, buffer, sizeof(network_type));
    message->type = ntohl(network_type);
    if (!is_valid_message_type(message->type)) {
        errno = EINVAL;
        return -1;
    }

    memcpy(message->sender, buffer + sizeof(network_type), MAX_USERNAME - 1);
    memcpy(message->receiver, buffer + sizeof(network_type) + MAX_USERNAME, MAX_USERNAME - 1);
    memcpy(message->content, buffer + sizeof(network_type) + MAX_USERNAME + MAX_USERNAME, MAX_MESSAGE - 1);
    message->sender[MAX_USERNAME - 1] = '\0';
    message->receiver[MAX_USERNAME - 1] = '\0';
    message->content[MAX_MESSAGE - 1] = '\0';
    return 0;
}

bool is_valid_message_type(uint32_t type) {
    switch (type) {
        case MSG_REGISTER:
        case MSG_LOGIN:
        case MSG_LOGIN_SUCCESS:
        case MSG_LOGIN_FAILURE:
        case MSG_REGISTER_SUCCESS:
        case MSG_REGISTER_FAILURE:
        case MSG_PUBLIC:
        case MSG_PRIVATE:
        case MSG_USER_LIST:
        case MSG_LOGOUT:
        case MSG_DISCONNECT:
        case MSG_INFO:
        case MSG_ERROR:
            return true;
        default:
            return false;
    }
}
