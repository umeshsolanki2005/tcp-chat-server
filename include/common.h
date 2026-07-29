#ifndef COMMON_H
#define COMMON_H

#define _POSIX_C_SOURCE 200809L

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAX_USERNAME 21
#define MAX_MESSAGE 1024
#define MAX_CLIENTS 100
#define MAX_BUFFER 2048
#define SERVER_BACKLOG 10
#define DEFAULT_PORT 8080
#define USER_DB_PATH "data/users.txt"
#define LOG_PATH "logs/server.log"
#define MAX_PASSWORD_HASH 128
#define MAX_LINE_LENGTH 1024

#endif
