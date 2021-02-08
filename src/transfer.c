#include "transfer.h"

size_t read_data(char *buffer, size_t n, client_t *cl) {
    if (cl->mode == OCTET) {
        return fread(buffer, 1, n, cl->file);
    }

    // Replace "\n" with "\r\n" and "\r" with "\r\0"
    if (cl->mode == NETASCII) {
        int cur;
        size_t pos = 0;
        
        while (pos < n) {
            if (cl->is_tail) {
                cur = (cl->prev_char == '\n') ? '\n' : '\0';
                cl->is_tail = 0;
            } else {
                cur = fgetc(cl->file);
                
                if (cur == EOF)
                    break;
                
                if (cur == '\n' || cur == '\r') {
                    cl->prev_char = cur;
                    cur = '\r';
                    cl->is_tail = 1;
                }
            }

            buffer[pos++] = (char)cur;
        }
        
        return pos;
    }

    return 0;
}

size_t write_data(char *buffer, size_t n, client_t *cl) {
    if (cl->mode == OCTET) {
        return fwrite(buffer, 1, n, cl->file);
    }

    // Replace "\r\n" with "\n" and "\r\0" with "\r"
    if (cl->mode == NETASCII) {
        size_t pos = 0;
        
        while (pos < n) {
            if (cl->prev_char == '\r' && buffer[pos] == '\n') {
                fseek(cl->file, -1, SEEK_CUR);
            }
            
            if (cl->prev_char != '\r' || buffer[pos] != '\0') {
                fputc(buffer[pos], cl->file);
            }

            cl->prev_char = buffer[pos++];
        }

        return pos;
    }

    return 0;
}

void handle_recv(int sock_fd, client_t *client[], size_t *nclients) {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    size_t buffer_size = 0;
    char buffer[BUFFER_SIZE];

    int nrecv = recvfrom(sock_fd, buffer, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr *)&addr, &addrlen);
    if (nrecv < 0) {
        perror("handle_recv: recvfrom");
        return;
    }

    client_t *cl = client_find(addr, client, *nclients);

    // Handling a new client
    if (cl == NULL) {
        if (*nclients < MAX_CLIENTS) {
            if ((cl = client_init(addr)) != NULL) {
                client[*nclients] = cl;
                *nclients += 1;
                printf("added new client: %lu active in total\n", *nclients);
            }
        } else {
            printf("limit of active clients reached\n");
            return;
        }
    }

    buffer_size = nrecv;

    packet_t req;
    if (deserialize_packet(buffer, buffer_size, &req) == NULL) {
        return;
    }

    if (req.opcode == RRQ && cl->state == NEW) {
        printf("received READ request for file [%s]\n", req.read.file_name);
        cl->state = WAIT;

        if ((cl->mode = convert_mode(req.read.mode)) == UNKNOWN) {
            make_error(cl->last, ILLEGAL_OPERATION, "unsupported mode");
            return;
        }

        if (access(req.read.file_name, F_OK) != 0) {
            make_error(cl->last, FILE_NOT_FOUND, "file not found");
            return;
        }

        if ((cl->file = fopen(req.read.file_name, "rb")) == NULL) {
            make_error(cl->last, ACCESS_VIOLATION, strerror(errno));
            return;
        }

        buffer_size = read_data(buffer, MAX_DATA_SIZE, cl);
        make_data(cl->last, buffer, buffer_size, 1);
        cl->is_last = buffer_size < MAX_DATA_SIZE;
    } else if (req.opcode == WRQ && cl->state == NEW) {
        printf("received WRITE request for file [%s]\n", req.read.file_name);
        cl->state = WAIT;

        if ((cl->mode = convert_mode(req.write.mode)) == UNKNOWN) {
            make_error(cl->last, ILLEGAL_OPERATION, "unsupported mode");
            return;
        }

        if (access(req.write.file_name, F_OK) == 0) {
            make_error(cl->last, FILE_ALREADY_EXISTS, "file already exists");
            return;
        }

        if ((cl->file = fopen(req.write.file_name, "wb")) == NULL) {
            make_error(cl->last, errno == ENOMEM ? DISK_FULL : ACCESS_VIOLATION, strerror(errno));
            return;
        }

        make_ack(cl->last, 0);
    } else if (req.opcode == DATA && cl->state != NEW) {
        printf("received DATA block [%hu] of %lu bytes\n", req.data.block_id, req.data.buffer_size);
        cl->state = WAIT;

        if (req.data.block_id == cl->last->ack.block_id + 1) {
            buffer_size = write_data(req.data.buffer, req.data.buffer_size, cl);
            make_ack(cl->last, req.data.block_id);
            cl->is_last = req.data.buffer_size < MAX_DATA_SIZE;
            cl->nrep = cl->nout = 0;
        } else if (req.data.block_id == cl->last->ack.block_id) {
            cl->nrep++;
            cl->nout = 0;
        } else {
            make_error(cl->last, UNKNOWN_TRANSFER_ID, "wrong block number");
        }
    } else if (req.opcode == ACK && cl->state != NEW) {
        printf("received ACK block [%hu]\n", req.ack.block_id);
        cl->state = WAIT;

        if (cl->is_last) {
            cl->state = INACTIVE;
            return;
        }

        if (req.ack.block_id == cl->last->data.block_id) {
            buffer_size = read_data(buffer, MAX_DATA_SIZE, cl);
            make_data(cl->last, buffer, buffer_size, req.ack.block_id + 1);
            cl->is_last = buffer_size < MAX_DATA_SIZE;
            cl->nrep = cl->nout = 0;
        } else if (req.ack.block_id + 1 == cl->last->data.block_id) {
            cl->nrep++;
            cl->nout = 0;
        } else {
            make_error(cl->last, UNKNOWN_TRANSFER_ID, "wrong block number");
        }
    } else if (req.opcode == ERROR && cl->state != NEW) {
        printf("received ");
        print_error(&req);
        cl->state = INACTIVE;
    } else {
        cl->state = WAIT;
        make_error(cl->last, ILLEGAL_OPERATION, "unknown operation");
    }
}

void handle_send(int sock_fd, client_t *cl) {
    if (cl->state != WAIT) {
        return;
    }
    
    if (cl->nrep >= MAX_ATTEMPTS || cl->nout >= MAX_ATTEMPTS) {
        printf("attempts limit exceeded\n");
        cl->state = INACTIVE;
        return;
    }

    size_t buffer_size = 0;
    char buffer[BUFFER_SIZE];

    buffer_size = serialize_packet(cl->last, buffer);
    if (sendto(sock_fd, buffer, buffer_size, 0, (struct sockaddr *)&cl->addr, cl->addrlen) < 0) {
        perror("handle_send: sendto");
        return;
    }

    if (cl->last->opcode == DATA) {
        printf("sent DATA block [%hu] of %lu bytes\n", cl->last->data.block_id, cl->last->data.buffer_size);
    } else if (cl->last->opcode == ACK) {
        printf("sent ACK block [%hu]\n", cl->last->ack.block_id);
        if (cl->is_last) {
            cl->state = INACTIVE;
            return;
        }
    } else if (cl->last->opcode == ERROR) {
        printf("sent ERROR code [%hu]\n", cl->last->error.error_id);
        cl->state = INACTIVE;
        return;
    }

    gettimeofday(&cl->sent_at, NULL);
    cl->state = SENT;
}