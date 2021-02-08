#include "client.h"

client_t *client_init(struct sockaddr_in addr) {
    client_t *cl = (client_t *)malloc(sizeof(client_t));

    if (cl == NULL) {
        return NULL;
    }

    cl->addrlen = sizeof(cl->addr);
    memset((char *)&cl->addr, 0, cl->addrlen);

    cl->addr.sin_family = addr.sin_family;
    cl->addr.sin_port = addr.sin_port;
    cl->addr.sin_addr.s_addr = addr.sin_addr.s_addr;

    cl->state = NEW;

    cl->file = NULL;
    cl->mode = UNKNOWN;
    cl->prev_char = '*'; // Any non-null non-control character
    cl->is_tail = 0;

    cl->last = (packet_t *)malloc(sizeof(packet_t));
    cl->is_last = 0;

    cl->nrep = 0;
    cl->nout = 0;

    return cl;
}

client_t *client_find(struct sockaddr_in addr, client_t *client[], size_t nclient) {
    for (size_t i = 0; i < nclient; i++) {
        if (client[i]->addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
            client[i]->addr.sin_port == addr.sin_port) {
            return client[i];
        }
    }

    return NULL;
}

void client_free(client_t *cl) {
    if (cl == NULL) {
        return;
    }
    
    if (cl->file != NULL) {
        fclose(cl->file);
    }

    if (cl->last != NULL) {
        free(cl->last);
    }
    
    free(cl);
}

void check_timeout(client_t *cl) {
    if (cl == NULL || cl->state != SENT) {
        return;
    }

    struct timeval now;
    gettimeofday(&now, NULL);

    // Time since the last block was sent to the client in us
    unsigned long long elapsed = (now.tv_sec - cl->sent_at.tv_sec) * 1000000ull + (now.tv_usec - cl->sent_at.tv_usec);

    if (elapsed > ATTEMPT_TIMEOUT * 1000ull) {
        cl->nrep = 0;
        cl->nout++;
        cl->state = WAIT;
    }
}