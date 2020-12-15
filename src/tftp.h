#ifndef TFTP_H
#define TFTP_H

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>

#define SERVER_DEFAULT_PORT 11111

#define TFTP_MAX_CLIENTS 100
#define TFTP_MAX_ATTEMPTS 5
#define TFTP_ATTEMPT_TIMEOUT 3

#define TFTP_MODE_OCTET "octet"
#define TFTP_MODE_NETASCII "netascii"

#define BUFFER_SIZE 1024

#define DEBUG 1

#endif