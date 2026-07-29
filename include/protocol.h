#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "common.h"

typedef enum {
    MSG_REGISTER = 1,
    MSG_LOGIN,
    MSG_LOGIN_SUCCESS,
    MSG_LOGIN_FAILURE,
    MSG_REGISTER_SUCCESS,
    MSG_REGISTER_FAILURE,
    MSG_PUBLIC,
    MSG_PRIVATE,
    MSG_USER_LIST,
    MSG_LOGOUT,
    MSG_DISCONNECT,
    MSG_INFO,
    MSG_ERROR
} message_type_t;

typedef struct {
    uint32_t type;
    char sender[MAX_USERNAME];
    char receiver[MAX_USERNAME];
    char content[MAX_MESSAGE];
} chat_message_t;

int send_all(int fd, const void *buffer, size_t length);
int recv_all(int fd, void *buffer, size_t length);
void prepare_message(chat_message_t *message, uint32_t type, const char *sender, const char *receiver, const char *content);
int serialize_message(const chat_message_t *message, char *buffer, size_t buffer_size, size_t *written);
int deserialize_message(const char *buffer, size_t buffer_size, chat_message_t *message);
bool is_valid_message_type(uint32_t type);

#endif
