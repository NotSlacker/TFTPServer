#include "client.h"

client_t *cl_create(struct sockaddr_in addr) {
    client_t *cl;
    if ((cl = (client_t *)malloc(sizeof(client_t))) == NULL) {
        return NULL;
    }

    cl->addrlen = sizeof(cl->addr);
    bzero((char *)&cl->addr, cl->addrlen);

    cl->addr.sin_family = addr.sin_family;
    cl->addr.sin_port = addr.sin_port;
    cl->addr.sin_addr.s_addr = addr.sin_addr.s_addr;

    cl->is_active = 1;
    cl->is_first = 1;
    cl->is_last = 0;
    cl->state = WAIT;

    cl->file = NULL;
    cl->mode = 0;
    cl->prev = 'p';
    cl->is_tail = 0;

    cl->last = (packet_t *)malloc(sizeof(packet_t));

    cl->nrep = 0;
    cl->nout = 0;

    return cl;
}

client_t *cl_find(struct sockaddr_in addr, client_t *client[], size_t nclient) {
    for (size_t i = 0; i < nclient; i++) {
        if (client[i]->addr.sin_addr.s_addr == addr.sin_addr.s_addr && client[i]->addr.sin_port == addr.sin_port) {
            return client[i];
        }
    }

    return NULL;
}

void cl_delete(client_t *cl) {
    if (cl->file != NULL) {
        fclose(cl->file);
    }
    if (cl->last != NULL) {
        free(cl->last);
    }
    if (cl != NULL) {
        free(cl);
    }
}

void check_timeout(client_t *cl) {
    if (cl == NULL || !cl->is_active || cl->state != WAIT) {
        return;
    }

    struct timeval now;
    gettimeofday(&now, NULL);

    unsigned long elapsed = (now.tv_sec - cl->sent_at.tv_sec) * 1000u + (now.tv_sec - cl->sent_at.tv_sec) / 1000u;
    if (elapsed > TFTP_ATTEMPT_TIMEOUT * 1000ul) {
        cl->state = TIMEOUT;
    }
}