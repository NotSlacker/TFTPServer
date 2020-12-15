#ifndef CLIENT_H
#define CLIENT_H

#include "packet.h"

enum state {
    WAIT,
    READY,
    ERROR,
    RETRY,
    TIMEOUT
};

typedef struct {
    int is_active;
    int is_first;
    int is_last;
    int state;
    
    struct sockaddr_in addr;
    socklen_t addrlen;

    FILE *file;
    int mode;
    char prev;
    int is_tail;
    
    packet_t *last;
    
    int nrep;
    int nout;
    struct timeval sent_at;
} client_t;

client_t *cl_create(struct sockaddr_in addr);
client_t *cl_find(struct sockaddr_in addr, client_t *client[], size_t nclient);
void cl_delete(client_t *cl);

void check_timeout(client_t *cl);

#endif