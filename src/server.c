#include "transfer.h"

void listen_loop(int port) {
    size_t off = 0;
    size_t nclient = 0;
    client_t *client[TFTP_MAX_CLIENTS] = { NULL };

    int sv_fd;
    struct sockaddr_in sv_addr;

    if ((sv_fd = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) < 0) {
        perror("listen_loop: socket");
        return;
    }

    bzero((char *)&sv_addr, sizeof(sv_addr));

    sv_addr.sin_family = AF_INET;
    sv_addr.sin_port = htons(port);
    sv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sv_fd, (struct sockaddr *)&sv_addr, sizeof(sv_addr)) < 0) {
        perror("listen_loop: bind");
        return;
    }

    printf("server listening...\n");

    int rpoll = 0;

    struct pollfd pfd[1];
    bzero(pfd, sizeof(pfd));

    pfd[0].fd = sv_fd;

    while (1) {        
        if (nclient == 0) {
            pfd[0].events = POLLIN;
        } else {
            pfd[0].events = POLLIN | POLLOUT;
        }

        if ((rpoll = poll(pfd, 1, -1)) < 0) {
            perror("listen_loop: poll");
            break;
        }

        if (pfd[0].revents & POLLIN) {
            handle_recv(sv_fd, client, &nclient);
        }
        
        if (pfd[0].revents & POLLOUT) {
            for (size_t i = 0; i < nclient; i++) {
                handle_send(sv_fd, client[i]);
            }
        }
        
        for (size_t i = 0; i < nclient; i++) {
            check_timeout(client[i]);
        }

        off = 0;
        for (size_t i = 0; i < nclient; i++) {
            if (!client[i]->is_active) {
                cl_delete(client[i]);
                printf("removed inactive client\n");
                off++;
            } else if (off > 0) {
                client[i - off] = client[i];
            }
        }
        nclient -= off;
        if (off) {
            printf("%lu active clients so far\n", nclient);
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