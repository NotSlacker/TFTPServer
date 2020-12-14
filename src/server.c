#include "tftp.h"

int server_send(int sock_fd, struct sockaddr *client_addr, packet_t *packet) {
    FILE *file_h;

    if (strncmp(packet->read.mode, TFTP_MODE_OCTET, MAX_MODE_SIZE) != 0 &&
        strncmp(packet->read.mode, TFTP_MODE_NETASCII, MAX_MODE_SIZE) != 0) {
        send_error(sock_fd, client_addr, TFTP_ERROR_ILLEGAL_OPERATION, "Unsupported mode");
        return 0;
    }

    if (access(packet->read.filename, F_OK) == 0) {
        if ((file_h = fopen(packet->read.filename, "rb")) == NULL) {
            send_error(sock_fd, client_addr, TFTP_ERROR_ACCESS_VIOLATION, strerror(errno));
            return 0;
        }
    } else {
        send_error(sock_fd, client_addr, TFTP_ERROR_FILE_NOT_FOUND, "File not found");
        return 0;
    }

    int result = send_file(sock_fd, client_addr, file_h, packet->read.mode);

    fclose(file_h);
    return result;
}

int server_recieve(int sock_fd, struct sockaddr *client_addr, packet_t *packet) {
    FILE *file_h;

    if (strncmp(packet->write.mode, TFTP_MODE_OCTET, MAX_MODE_SIZE) != 0 &&
        strncmp(packet->write.mode, TFTP_MODE_NETASCII, MAX_MODE_SIZE) != 0) {
        send_error(sock_fd, client_addr, TFTP_ERROR_ILLEGAL_OPERATION, "Unsupported mode");
        return 0;
    }

    if (access(packet->write.filename, F_OK) != 0) {
        if ((file_h = fopen(packet->read.filename, "wb")) == NULL) {
            if (errno == ENOMEM)
                send_error(sock_fd, client_addr, TFTP_ERROR_DISK_FULL, strerror(errno));
            else
                send_error(sock_fd, client_addr, TFTP_ERROR_ACCESS_VIOLATION, strerror(errno));
            return 0;
        }
    } else {
        send_error(sock_fd, client_addr, TFTP_ERROR_FILE_ALREADY_EXISTS, "File already exists");
        return 0;
    }

    send_ack(sock_fd, client_addr, 0);

    int result = recv_file(sock_fd, client_addr, file_h, packet->write.mode);
    
    fclose(file_h);
    return result;
}

static unsigned int child_counter = 0;

void process_child(struct sockaddr client_addr, packet_t *packet) {
    if (packet == NULL)
        return;

    int is_success = 1;
    int child_sock_fd = connect_socket_on(0);

    switch (packet->opcode) {
    case TFTP_OPCODE_RRQ:
        printf("Processng RRQ request for \"%s\"...\n", packet->read.filename);
        is_success = server_send(child_sock_fd, &client_addr, packet);
        break;
    case TFTP_OPCODE_WRQ:
        printf("Processng WRQ request for \"%s\"...\n", packet->write.filename);
        is_success = server_recieve(child_sock_fd, &client_addr, packet);
        break;
    default:
        is_success = send_error(child_sock_fd, &client_addr, TFTP_ERROR_ILLEGAL_OPERATION, "Unexpected Packet");
        break;
    }

    if (is_success == 0)
        printf("Failed to handle request\n");
    else
        printf("Done processing request\n");
}

void transfer_loop(int sock_fd) {
    int recv_size;
    char buffer[BUFFER_SIZE];

    struct sockaddr client_addr;
    size_t client_size = sizeof(client_addr);
    
    packet_t packet;
    
    pid_t fork_id;

    printf("Server is running\n");

    struct pollfd pfd[1] = { { sock_fd, POLLIN, 0 } };

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);

        if (poll(pfd, 1, -1) > 0) {
            recv_size = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, &client_addr, (socklen_t *)&client_size);

            if (recv_size < 0) {
                perror("Error in recvfrom:");
                exit(1);
            }

            if (recv_size == 0) {
                printf("Socket closed\n");
                exit(0);
            }

            if (child_counter < TFTP_MAX_CLIENTS) {
                deserialize_packet(buffer, recv_size, &packet);
                fork_id = fork();
            } else {
                send_error(sock_fd, &client_addr, TFTP_ERROR_NOT_DEFINED, "Too many clients");
                continue;
            }

            if (fork_id < 0) {
                send_error(sock_fd, &client_addr, TFTP_ERROR_NOT_DEFINED, "Unable to handle request");
                exit(1);
            } else if (fork_id == 0) {
                close(sock_fd);
                
                if (DEBUG)
                    print_packet(&packet);

                child_counter++;
                process_child(client_addr, &packet);
                child_counter--;

                exit(0);
            }
        } else {
            perror("Error in poll");
            exit(1);
        }
    }

    close(sock_fd);
}

int main(int argc, char *argv[]) {
    int port = 1;
    
    if (argc == 1)
        port = SERVER_DEFAULT_PORT;

    if (argc == 2)
        port = atoi(argv[1]);
    
    if (argc > 2) {
        printf("Usage: %s [port]\n", argv[0]);
        return 0;
    }

    printf("Using port: %d\n", port);

    int sock_fd = connect_socket_on(port);
    transfer_loop(sock_fd);

    return 0;
}