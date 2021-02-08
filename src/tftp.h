#ifndef TFTP_H
#define TFTP_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>

#define SERVER_DEFAULT_PORT 11111

#define MAX_STRING_SIZE 512
#define MAX_DATA_SIZE 512
#define MAX_MODE_SIZE 16
#define BUFFER_SIZE 1024

#define MAX_CLIENTS 100
#define MAX_ATTEMPTS 5

#define ATTEMPT_TIMEOUT 3000ull

#define STR_MODE_OCTET "octet"
#define STR_MODE_NETASCII "netascii"

enum tftp_mode {
    UNKNOWN,
    OCTET,
    NETASCII
};

enum tftp_opcode {
    RRQ = 1,
    WRQ,
    DATA,
    ACK,
    ERROR
};

enum tftp_error {
    NOT_DEFINED,
    FILE_NOT_FOUND,
    ACCESS_VIOLATION,
    DISK_FULL,
    ILLEGAL_OPERATION,
    UNKNOWN_TRANSFER_ID,
    FILE_ALREADY_EXISTS,
    NO_SUCH_USER
};

#endif