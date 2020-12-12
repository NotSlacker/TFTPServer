#include "tftp.h"

int connect_socket_on(int port) {
    int sock_fd;
    struct sockaddr_in serv_addr;

    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        return -1;

    memset((char *)&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        return -1;
    
    return sock_fd;
}

void catch_alarm(int foo) {
    return;
}

int wait_for_packet(int sock_fd, struct sockaddr *client_addr, uint16_t opcode, packet_t *packet) {
    size_t n;
    size_t client_size = sizeof(client_addr);
    
    char buffer[BUFFER_SIZE];
    struct sigaction action;

    action.sa_handler = catch_alarm;
    if (sigfillset(&action.sa_mask) < 0) {
        if (DEBUG)
            printf("Can not set sighandler\n");
        return -1;
    }

    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0) {
        if (DEBUG)
            printf("Can not set sigalarm\n");
        return -1;
    }

    alarm(TFTP_ATTEMPT_TIMEOUT);

    errno = 0;
    while (1) {
        if (DEBUG)
            printf("Waiting for response... ");
        
        fflush(stdout);

        n = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, client_addr, (socklen_t *)&client_size);
        
        if (errno != 0) {
            if (errno == EINTR) {
                if (DEBUG)
                    printf("Timeout detected\n");
            } else {
                if (DEBUG)
                    printf("Error with value <%d>\n", errno);
            }

            alarm(0);
            return -1;
        }

        if (DEBUG)
            printf("Done\n");

        if (deserialize_packet(buffer, n, packet) == NULL) {
            alarm(0);
            return -1;
        }

        if (packet->opcode == opcode) {
            alarm(0);
            return n;
        } else if (packet->opcode == TFTP_OPCODE_ERROR) {
            alarm(0);
            return 0;
        }
    }
}

int send_packet(int sock_fd, struct sockaddr *sock_info, packet_t *packet) {
    char buffer[BUFFER_SIZE];
    size_t n = serialize_packet(packet, buffer);
    return sendto(sock_fd, buffer, n, 0, (struct sockaddr *)sock_info, sizeof(struct sockaddr)) >= 0;
}

int send_data(int sock_fd, struct sockaddr *sock_info, uint16_t block_num, char *data, size_t data_size) {
    packet_t packet;
    packet.opcode = TFTP_OPCODE_DATA;
    packet.data.block_num = block_num;
    packet.data.data_size = data_size;
    memcpy(packet.data.data, data, data_size);
    return send_packet(sock_fd, sock_info, &packet);
}

int send_ack(int sock_fd, struct sockaddr *sock_info, uint16_t block_num) {
    packet_t packet;
    packet.opcode = TFTP_OPCODE_ACK;
    packet.ack.block_num = block_num;
    return send_packet(sock_fd, sock_info, &packet);
}

int send_error(int sock_fd, struct sockaddr *sock_info, uint16_t ercode, char *message) {
    packet_t packet;
    packet.opcode = TFTP_OPCODE_ERROR;
    packet.error.ercode = ercode;
    strncpy(packet.error.message, message, MAX_STRING_SIZE);
    return send_packet(sock_fd, sock_info, &packet);
}

size_t read_data(char *buffer, size_t n, FILE *file_h, int convert) {
    if (convert == 0)
        return fread(buffer, 1, n, file_h);

    size_t total = 0;
    static int tail = 0;

    int curr;
    static int prev;
    
    while (total < n) {
        if (tail) {
            curr = (prev == '\n') ? '\n' : '\0';
            tail = 0;
        } else {
            curr = fgetc(file_h);
            
            if (curr == EOF)
                break;
            
            if (curr == '\n' || curr == '\r') {
                prev = curr;
                curr = '\r';
                tail = 1;
            }
        }

        buffer[total++] = (char)curr;
    }
    
    return total;
}

size_t write_data(char *buffer, size_t n, FILE *file_h, int convert) {
    if (convert == 0)
        return fwrite(buffer, 1, n, file_h);

    int pos = 0;
    int skip = 0;

    static char prev = 'p';
    
    while (pos < n) {
        if (prev == '\r') {
            if (buffer[pos] == '\n')
                fseek(file_h, -1, SEEK_CUR);
            else if (buffer[pos] == '\0')
                skip = 1;
        }
        
        if (!skip)
            fputc(buffer[pos], file_h);
        else
            skip = 0;

        prev = buffer[pos++];
    }
    
    return n;
}

int send_file(int sock_fd, struct sockaddr *client_addr, FILE *file_h, char *mode) {
    size_t n;
    char buffer[MAX_DATA_SIZE];

    packet_t packet;

    uint16_t block_num = 1;
    unsigned int attempt_counter = 0;

    do {
        n = read_data(buffer, MAX_DATA_SIZE, file_h, !strcmp(mode, "netascii"));

        if (DEBUG)
            printf("Sending DATA: [%u] ", block_num);

        if (!send_data(sock_fd, client_addr, block_num, buffer, n))
            return 0;

        if (DEBUG)
            printf("Sent\n");

        if (wait_for_packet(sock_fd, client_addr, TFTP_OPCODE_ACK, &packet) == -1) {
            if (++attempt_counter >= TFTP_MAX_ATTEMPTS) {
                printf("Attempts limit exceeded\n");
                return 0;
            }
        } else {
            if (packet.opcode == TFTP_OPCODE_ERROR) {
                print_error(&packet);
                return 0;
            }

            if (packet.ack.block_num != block_num) {
                if (DEBUG)
                    printf("Expected block [%u], but block [%u] recieved\n", block_num, packet.data.block_num);
                
                send_error(sock_fd, client_addr, TFTP_ERROR_UNKNOWN_TRANSFER_ID, "Wrong block number");
                return 0;
            }

            block_num++;
            attempt_counter = 0;
        }
    } while (n == MAX_DATA_SIZE);

    return 1;
}

int recv_file(int sock_fd, struct sockaddr *client_addr, FILE *file_h, char *mode) {
    packet_t packet;

    uint16_t block_num = 1;
    unsigned int attempt_counter = 0;

    do {
        if (wait_for_packet(sock_fd, client_addr, TFTP_OPCODE_DATA, &packet) == -1) {
            if (++attempt_counter >= TFTP_MAX_ATTEMPTS) {
                printf("Attempts limit exceeded\n");
                return 0;
            }
        } else if (packet.opcode == TFTP_OPCODE_ERROR) {
            print_error(&packet);
            return 0;
        } else {
            if (packet.data.block_num == block_num) {
                if (write_data(packet.data.data, packet.data.data_size, file_h, !strcmp(mode, "netascii"))) {
                    block_num++;
                    attempt_counter = 0;
                } else if (packet.data.data_size != 0) {
                    send_error(sock_fd, client_addr, TFTP_ERROR_NOT_DEFINED, "No data to write");
                    return 0;
                }

                if (DEBUG)
                    printf("Sending ACK: [%u] ", packet.data.block_num);

                if (!send_ack(sock_fd, client_addr, packet.data.block_num))
                    return 0;

                if (DEBUG)
                    printf("Sent\n");
            } else {
                if (DEBUG)
                    printf("Expected block [%u], but block [%u] recieved\n", block_num, packet.data.block_num);

                send_error(sock_fd, client_addr, TFTP_ERROR_UNKNOWN_TRANSFER_ID, "Wrong block number");
                return 0;
            }
        }
    } while (packet.data.data_size == MAX_DATA_SIZE);

    return 1;
}