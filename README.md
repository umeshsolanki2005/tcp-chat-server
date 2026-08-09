# TCP Chat Server in C  

## Overview
This project implements a production-style multi-client TCP chat server and terminal client in C using Linux sockets, POSIX threads, mutexes, and file-based authentication. It demonstrates modular C design, network programming, multithreading, logging, validation, and graceful resource cleanup....
.
## Features
- Multi-client TCP chat server
- User registration and login
- Public broadcast messaging
- Private one-to-one messaging
- Connected-user listing
- Thread-safe client management
- Structured protocol with fixed-size messages
- File-based user storage
- Thread-safe logging
- Graceful shutdown and cleanup

## Architecture
The project is organized into networking, authentication, logging, protocol, and utility modules.

## Directory Structure
```text
tcp-chat-server/
├── Makefile
├── README.md
├── include/
│   ├── common.h
│   ├── protocol.h
│   ├── server.h
│   ├── client.h
│   ├── auth.h
│   ├── logger.h
│   └── utils.h
├── src/
│   ├── server.c
│   ├── client.c
│   ├── auth.c
│   ├── logger.c
│   ├── protocol.c
│   └── utils.c
├── data/
│   └── users.txt
├── logs/
│   └── server.log
└── tests/
    ├── test_auth.c
    ├── test_protocol.c
    └── test_utils.c
```

## Build and Run
```bash
git clone <repository-url>
cd tcp-chat-server
make
./server 8080
```
Run clients in separate terminals:
```bash
./client 127.0.0.1 8080
```

## Client Commands
- /help
- /users
- /msg <username> <message>
- /all <message>
- /logout
- /quit

## Testing
```bash
make tests
valgrind --leak-check=full --show-leak-kinds=all ./server 8080
```

## Security Notes
The custom password hash used here is educational and not production-grade. Traffic is not encrypted. For production, use TLS and a strong password-hashing library such as Argon2 or bcrypt.

## Resume Description
Developed a multi-client TCP chat application in C using Linux socket programming and POSIX threads. Implemented concurrent client handling, file-based authentication, public and private messaging, protocol-based communication, thread-safe logging, graceful disconnection, and signal-based server shutdown.

## Resume Bullet Points
- Developed a multi-client TCP chat server in C using Linux sockets and POSIX threads, supporting concurrent user connections, authentication, and real-time messaging.
- Implemented public broadcasting, private messaging, connected-user tracking, and a structured TCP communication protocol with partial-send and partial-receive handling.
- Added thread-safe client management and logging using mutexes, along with graceful shutdown, signal handling, error recovery, and resource cleanup.
- Tested the application using multiple clients, unit tests, GCC warnings, GDB, and Valgrind.
