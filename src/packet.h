#ifndef PACKET_H
#define PACKET_H

#include "tftp.h"

struct rrq {
    char     file_name[MAX_STRING_SIZE];
    char     mode[MAX_MODE_SIZE];
};

struct wrq {
    char     file_name[MAX_STRING_SIZE];
    char     mode[MAX_MODE_SIZE];
};

struct data {
    uint16_t block_id;
    size_t   buffer_size;
    char     buffer[MAX_DATA_SIZE];
};

struct ack {
    uint16_t block_id;
};

struct error {
    uint16_t error_id;
    char     message[MAX_STRING_SIZE];
};

typedef struct {
    uint16_t opcode;
    union {
        struct rrq   read;
        struct wrq   write;
        struct data  data;
        struct ack   ack;
        struct error error;
    };
} packet_t;

enum tftp_mode convert_mode(char *mode);

void make_data(packet_t *packet, char *buffer, size_t buffer_size, uint16_t block_id);
void make_ack(packet_t *packet, uint16_t block_id);
void make_error(packet_t *packet, uint16_t nerror, char *message);

packet_t *deserialize_packet(char *buffer, size_t buffer_size, packet_t *packet);
size_t    serialize_packet(const packet_t *packet, char *buffer);

void print_packet(packet_t *packet);
void print_error(packet_t *packet);

#endif