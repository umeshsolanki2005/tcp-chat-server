CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Werror -std=c11 -D_POSIX_C_SOURCE=200809L -pthread

INCLUDES = -Iinclude

SRCS = src/server.c src/client.c src/auth.c src/logger.c src/protocol.c src/utils.c
OBJS = $(SRCS:.c=.o)

all: server client

server: src/server.o src/auth.o src/logger.o src/protocol.o src/utils.o
	$(CC) $(CFLAGS) $(INCLUDES) -o server src/server.o src/auth.o src/logger.o src/protocol.o src/utils.o

client: src/client.o src/auth.o src/logger.o src/protocol.o src/utils.o
	$(CC) $(CFLAGS) $(INCLUDES) -o client src/client.o src/auth.o src/logger.o src/protocol.o src/utils.o

src/%.o: src/%.c include/%.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

.PHONY: tests run-server clean

tests: test_auth test_protocol test_utils
	test_auth
	test_protocol
	test_utils

test_auth: src/auth.o src/utils.o tests/test_auth.c
	$(CC) $(CFLAGS) $(INCLUDES) -o tests/test_auth tests/test_auth.c src/auth.o src/utils.o

test_protocol: src/protocol.o src/utils.o tests/test_protocol.c
	$(CC) $(CFLAGS) $(INCLUDES) -o tests/test_protocol tests/test_protocol.c src/protocol.o src/utils.o

test_utils: src/utils.o tests/test_utils.c
	$(CC) $(CFLAGS) $(INCLUDES) -o tests/test_utils tests/test_utils.c src/utils.o

run-server:
	./server 8080

clean:
	rm -f server client src/*.o tests/test_auth tests/test_protocol tests/test_utils
