#ifndef TRANSFER_H
#define TRANSFER_H

#include "client.h"

size_t read_data(char *buffer, size_t n, client_t *cl);
size_t write_data(char *buffer, size_t n, client_t *cl);

void handle_recv(int sock_fd, client_t *client[], size_t *nclients);
void handle_send(int sock_fd, client_t *client);

#endif