#include "transfer.h"

size_t read_data(char *buffer, size_t n, client_t *cl) {
    bzero(buffer, n);

    if (cl->mode == 0)
        return fread(buffer, 1, n, cl->file);

    int curr;
    size_t total = 0;
    
    while (total < n) {
        if (cl->is_tail) {
            curr = cl->prev == '\n' ? '\n' : '\0';
            cl->is_tail = 0;
        } else {
            curr = fgetc(cl->file);
            
            if (curr == EOF)
                break;
            
            if (curr == '\n' || curr == '\r') {
                cl->prev = curr;
                curr = '\r';
                cl->is_tail = 1;
            }
        }

        buffer[total++] = (char)curr;
    }
    
    return total;
}

size_t write_data(char *buffer, size_t n, client_t *cl) {
    if (cl->mode == 0)
        return fwrite(buffer, 1, n, cl->file);

    size_t pos = 0;
    int skip = 0;

    while (pos < n) {
        if (cl->prev == '\r') {
            if (buffer[pos] == '\n')
                fseek(cl->file, -1, SEEK_CUR);
            else if (buffer[pos] == '\0')
                skip = 1;
        }
        
        if (!skip)
            fputc(buffer[pos], cl->file);
        else
            skip = 0;

        cl->prev = buffer[pos++];
    }
    
    return pos;
}

void handle_recv(int sock_fd, client_t *client[], size_t *nclient) {
    printf("receiving... ");

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    size_t nbuffer = 0;
    char buffer[BUFFER_SIZE];

    int nrecv;
    if ((nrecv = recvfrom(sock_fd, buffer, BUFFER_SIZE, MSG_DONTWAIT, (struct sockaddr *)&addr, &addrlen)) < 0) {
        perror("handle_recv: recvfrom");
        return;
    }

    client_t *cl;
    if ((cl = cl_find(addr, client, *nclient)) == NULL) {
        if (*nclient < TFTP_MAX_CLIENTS) {
            if ((cl = cl_create(addr)) != NULL) {
                client[*nclient] = cl;
                *nclient += 1;
                printf("added active client\n");
                printf("%lu active clients so far\n", *nclient);
            }
        }
    }

    packet_t req;
    nbuffer = nrecv;

    if (deserialize_packet(buffer, nbuffer, &req) == NULL) {
        return;
    }

    if (req.opcode == TFTP_OPCODE_RRQ && cl->is_first) {
        if (strncasecmp(req.read.mode, TFTP_MODE_OCTET, MAX_MODE_SIZE) != 0 &&
            strncasecmp(req.read.mode, TFTP_MODE_NETASCII, MAX_MODE_SIZE) != 0) {
            make_error(cl->last, TFTP_ERROR_ILLEGAL_OPERATION, "unsupported mode");
            cl->state = ERROR;
            return;
        }

        if (access(req.read.filename, F_OK) == 0) {
            if ((cl->file = fopen(req.read.filename, "rb")) == NULL) {
                if (errno == ENOMEM) {
                    make_error(cl->last, TFTP_ERROR_DISK_FULL, strerror(errno));
                } else {
                    make_error(cl->last, TFTP_ERROR_ACCESS_VIOLATION, strerror(errno));
                }
                cl->state = ERROR;
                return;
            }
        } else {
            make_error(cl->last, TFTP_ERROR_FILE_NOT_FOUND, "file not found");
            cl->state = ERROR;
            return;
        }

        cl->mode = !strcmp(req.read.mode, "netascii");
        nbuffer = read_data(buffer, MAX_DATA_SIZE, cl);

        make_data(cl->last, buffer, nbuffer, 1);
        cl->nrep = 0;
        cl->nout = 0;
        cl->is_first = 0;
        cl->state = READY;
    } else if (req.opcode == TFTP_OPCODE_WRQ && cl->is_first) {
        if (strncasecmp(req.write.mode, TFTP_MODE_OCTET, MAX_MODE_SIZE) != 0 &&
            strncasecmp(req.write.mode, TFTP_MODE_NETASCII, MAX_MODE_SIZE) != 0) {
            make_error(cl->last, TFTP_ERROR_ILLEGAL_OPERATION, "unsupported mode");
            cl->state = ERROR;
            return;
        }
        if (access(req.write.filename, F_OK) != 0) {
            if ((cl->file = fopen(req.write.filename, "wb")) == NULL) {
                if (errno == ENOMEM) {
                    make_error(cl->last, TFTP_ERROR_DISK_FULL, strerror(errno));
                } else {
                    make_error(cl->last, TFTP_ERROR_ACCESS_VIOLATION, strerror(errno));
                }
                cl->state = ERROR;
                return;
            }
        } else {
            make_error(cl->last, TFTP_ERROR_FILE_ALREADY_EXISTS, "file already exists");
            cl->state = ERROR;
            return;
        }

        cl->mode = !strcmp(req.write.mode, "netascii");

        make_ack(cl->last, 0);
        cl->nrep = 0;
        cl->nout = 0;
        cl->is_first = 0;
        cl->state = READY;
    } else if (req.opcode == TFTP_OPCODE_DATA && !cl->is_first) {
        printf("received DATA block [%hu] of %lu bytes\n", req.data.nblock, req.data.nbuffer);
        if (req.data.nblock == cl->last->ack.nblock + 1) {
            nbuffer = write_data(req.data.buffer, req.data.nbuffer, cl);
            cl->is_last = req.data.nbuffer < MAX_DATA_SIZE;
            make_ack(cl->last, req.data.nblock);
            cl->nrep = 0;
            cl->nout = 0;
            cl->state = READY;
        } else if (req.data.nblock == cl->last->ack.nblock) {
            cl->nout = 0;
            cl->state = RETRY;
        } else {
            make_error(cl->last, TFTP_ERROR_UNKNOWN_TRANSFER_ID, "wrong block number");
            cl->state = ERROR;
            return;
        }
    } else if (req.opcode == TFTP_OPCODE_ACK && !cl->is_first) {
        printf("received ACK block [%hu]\n", req.ack.nblock);
        if (cl->is_last) {
            cl->is_active = 0;
            return;
        }
        if (req.ack.nblock == cl->last->data.nblock) {
            nbuffer = read_data(buffer, MAX_DATA_SIZE, cl);
            cl->is_last = nbuffer < MAX_DATA_SIZE;
            make_data(cl->last, buffer, nbuffer, req.ack.nblock + 1);
            cl->nrep = 0;
            cl->nout = 0;
            cl->state = READY;
        } else if (req.ack.nblock + 1 == cl->last->data.nblock) {
            cl->nout = 0;
            cl->state = RETRY;
        } else {
            make_error(cl->last, TFTP_ERROR_UNKNOWN_TRANSFER_ID, "wrong block number");
            cl->state = ERROR;
            return;
        }
    } else if (req.opcode == TFTP_OPCODE_ERROR && !cl->is_first) {
        printf("received ERROR code [%hu]\n", req.error.nerror);
        print_error(&req);
        cl->is_active = 0;
        return;
    } else {
        make_error(cl->last, TFTP_ERROR_ILLEGAL_OPERATION, "unknown operation");
        cl->state = ERROR;
        return;
    }
}

void handle_send(int sock_fd, client_t *cl) {
    if (cl->is_active == 0 || cl->state == WAIT) {
        return;
    }

    printf("sending... ");

    size_t nbuffer = 0;
    char buffer[BUFFER_SIZE];

    nbuffer = serialize_packet(cl->last, buffer);
    if (sendto(sock_fd, buffer, nbuffer, 0, (struct sockaddr *)&cl->addr, cl->addrlen) < 0) {
        perror("handle_send: sendto");
        return;
    }

    if (cl->last->opcode == TFTP_OPCODE_DATA) {
        printf("sent DATA block [%hu] of %lu bytes\n", cl->last->data.nblock, cl->last->data.nbuffer);
    } else if (cl->last->opcode == TFTP_OPCODE_ACK) {
        printf("sent ACK block [%hu]\n", cl->last->ack.nblock);
        if (cl->is_last) {
            cl->is_active = 0;
        }
    } else if (cl->last->opcode == TFTP_OPCODE_ERROR) {
        printf("sent ERROR code [%hu]\n", cl->last->error.nerror);
    }

    if (cl->state == ERROR) {
        cl->is_active = 0;
        return;
    }

    if (cl->state == RETRY) {
        cl->nrep++;
        if (cl->nrep >= TFTP_MAX_ATTEMPTS) {
            printf("Attempts limit exceeded\n");
            cl->is_active = 0;
        }
    }

    if (cl->state == TIMEOUT) {
        cl->nout++;
        if (cl->nout >= TFTP_MAX_ATTEMPTS) {
            printf("Attempts limit exceeded\n");
            cl->is_active = 0;
        }
    }

    gettimeofday(&cl->sent_at, NULL);
    cl->state = WAIT;
}