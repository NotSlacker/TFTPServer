#ifndef PACKET_H
#define PACKET_H

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
    char filename[MAX_STRING_SIZE + 1];
    char mode[MAX_MODE_SIZE + 1];
} RRQ, WRQ;

typedef struct {
    uint16_t block_num;
    size_t data_size;
    char data[MAX_DATA_SIZE];
} DATA;

typedef struct {
    uint16_t block_num;
} ACK;

typedef struct {
    uint16_t ercode;
    char message[MAX_STRING_SIZE + 1];
} ERROR;

typedef struct {
    uint16_t opcode;
    union {
        RRQ read;
        WRQ write;
        DATA data;
        ACK ack;
        ERROR error;
    };
} packet_t;

packet_t *deserialize_packet(char *buffer, size_t buffer_size, packet_t *packet);
size_t serialize_packet(const packet_t *packet, char *buffer);

void print_packet(packet_t *packet);
void print_error(packet_t *packet);

#endif