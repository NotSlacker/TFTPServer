#ifndef PACKET_H
#define PACKET_H

#include "tftp.h"

#define MAX_MODE_SIZE 8
#define MAX_STRING_SIZE 512
#define MAX_DATA_SIZE 512

#define TFTP_OPCODE_RRQ 1
#define TFTP_OPCODE_WRQ 2
#define TFTP_OPCODE_DATA 3
#define TFTP_OPCODE_ACK 4
#define TFTP_OPCODE_ERROR 5

#define TFTP_ERROR_NOT_DEFINED 0
#define TFTP_ERROR_FILE_NOT_FOUND 1
#define TFTP_ERROR_ACCESS_VIOLATION 2
#define TFTP_ERROR_DISK_FULL 3
#define TFTP_ERROR_ILLEGAL_OPERATION 4
#define TFTP_ERROR_UNKNOWN_TRANSFER_ID 5
#define TFTP_ERROR_FILE_ALREADY_EXISTS 6
#define TFTP_ERROR_NO_SUCH_USER 7

typedef struct {
    char filename[MAX_STRING_SIZE];
    char mode[MAX_MODE_SIZE];
} rrq_t, wrq_t;

typedef struct {
    uint16_t nblock;
    size_t nbuffer;
    char buffer[MAX_DATA_SIZE];
} data_t;

typedef struct {
    uint16_t nblock;
} ack_t;

typedef struct {
    uint16_t nerror;
    char message[MAX_STRING_SIZE];
} error_t;

typedef struct {
    uint16_t opcode;
    union {
        rrq_t read;
        wrq_t write;
        data_t data;
        ack_t ack;
        error_t error;
    };
} packet_t;

void make_data(packet_t *packet, char *buffer, size_t nbuffer, uint16_t nblock);
void make_ack(packet_t *packet, uint16_t nblock);
void make_error(packet_t *packet, uint16_t nerror, char *message);

packet_t *deserialize_packet(char *buffer, size_t buffer_size, packet_t *packet);
size_t serialize_packet(const packet_t *packet, char *buffer);

void print_packet(packet_t *packet);
void print_error(packet_t *packet);

#endif