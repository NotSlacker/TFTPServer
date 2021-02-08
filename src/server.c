#include "transfer.h"

void listen_loop(int port) {
    size_t off = 0;
    size_t nclients = 0;
    client_t *client[MAX_CLIENTS] = { NULL };

    int sv_fd;
    struct sockaddr_in sv_addr;

    if ((sv_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) < 0) {
        perror("listen_loop: socket");
        return;
    }

    memset((char *)&sv_addr, 0, sizeof(sv_addr));

    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = htons(port);
    sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sv_fd, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) < 0) {
        perror("listen_loop: bind");
        return;
    }

    printf("server listening...\n");

    struct pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));

    pfd.fd = sv_fd;

    while (1) {        
        pfd.events = nclients == 0 ? POLLIN : POLLIN | POLLOUT;

        if (poll(&pfd, 1, -1) < 0) {
            perror("listen_loop: poll");
            break;
        }

        if (pfd.revents & POLLIN) {
            handle_recv(sv_fd, client, &nclients);
        }
        
        if (pfd.revents & POLLOUT) {
            for (size_t i = 0; i < nclients; i++) {
                handle_send(sv_fd, client[i]);
            }
        }
        
        for (size_t i = 0; i < nclients; i++) {
            check_timeout(client[i]);
        }

        off = 0;
        for (size_t i = 0; i < nclients; i++) {
            if (client[i]->state == INACTIVE) {
                client_free(client[i]);
                off++;
            } else if (off > 0) {
                client[i - off] = client[i];
            }
        }
        
        if (off > 0) {
            nclients -= off;
            printf("removed inactive clients: %lu active in total\n", nclients);
        }
    }

    close(sv_fd);
}

int main(int argc, char *argv[]) {
    int port = 1;
    
    if (argc == 1) {
        port = SERVER_DEFAULT_PORT;
    }

    if (argc == 2) {
        port = atoi(argv[1]);
    }
    
    if (argc > 2) {
        printf("usage: %s [port]\n", argv[0]);
        return 0;
    }

    printf("started on port: %d\n", port);

    listen_loop(port);

    return 0;
}