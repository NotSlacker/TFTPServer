#ifndef CLIENT_H
#define CLIENT_H

#include "packet.h"

enum client_state {
    NEW,
    SENT,
    WAIT,
    INACTIVE
};

typedef struct {
    socklen_t addrlen;
    struct sockaddr_in addr;

    enum client_state state;

    FILE *file;
    enum tftp_mode mode;
    char prev_char;
    int8_t is_tail;

    packet_t *last;
    int8_t is_last;

    int nrep; // Number of retries
    int nout; // Number of timeouts
    struct timeval sent_at;
} client_t;

client_t *client_init(struct sockaddr_in addr);
client_t *client_find(struct sockaddr_in addr, client_t *client[], size_t nclient);
void      client_free(client_t *cl);

void check_timeout(client_t *cl);

#endif