#include "packet.h"

enum tftp_mode convert_mode(char *mode) {
    if (strncasecmp(mode, "octet", MAX_MODE_SIZE) == 0) {
        return OCTET;
    }
    
    if (strncasecmp(mode, "netascii", MAX_MODE_SIZE) == 0) {
        return NETASCII;
    }
    
    return UNKNOWN;
}

void make_data(packet_t *packet, char *buffer, size_t buffer_size, uint16_t block_id) {
    packet->opcode = DATA;
    packet->data.block_id = block_id;
    packet->data.buffer_size = buffer_size;
    memcpy(packet->data.buffer, buffer, buffer_size);
}

void make_ack(packet_t *packet, uint16_t block_id) {
    packet->opcode = ACK;
    packet->ack.block_id = block_id;
}

void make_error(packet_t *packet, uint16_t error_id, char *message) {
    packet->opcode = ERROR;
    packet->error.error_id = error_id;
    strncpy(packet->error.message, message, MAX_STRING_SIZE);
}

packet_t *deserialize_packet(char *buffer, size_t buffer_size, packet_t *packet) {
    if (packet == NULL || buffer_size < 4) {
        return NULL;
    }

    memset(packet, 0, sizeof(packet_t));

    uint16_t temp = 0;
    memcpy(&temp, buffer, 2);

    switch (packet->opcode = ntohs(temp)) {
    case RRQ:
        strncpy(packet->read.file_name, buffer + 2, MAX_STRING_SIZE);
        strncpy(packet->read.mode, buffer + 2 + strlen(packet->read.file_name) + 1, MAX_MODE_SIZE);
        break;
    case WRQ:
        strncpy(packet->write.file_name, buffer + 2, MAX_STRING_SIZE);
        strncpy(packet->write.mode, buffer + 2 + strlen(packet->write.file_name) + 1, MAX_MODE_SIZE);
        break;
    case DATA:
        memcpy(&temp, buffer + 2, 2);
        packet->data.block_id = ntohs(temp);
        packet->data.buffer_size = buffer_size - 4;
        memcpy(packet->data.buffer, buffer + 4, packet->data.buffer_size);
        break;
    case ACK:
        memcpy(&temp, buffer + 2, 2);
        packet->ack.block_id = ntohs(temp);
        break;
    case ERROR:
        memcpy(&temp, buffer + 2, 2);
        packet->error.error_id = ntohs(temp);
        strncpy(packet->error.message, buffer + 4, MAX_STRING_SIZE);
        break;
    default:
        return NULL;
    }
    
    return packet;
}

size_t serialize_packet(const packet_t *packet, char *buffer) {
    if (buffer == NULL || packet == NULL) {
        return 0;
    }

    size_t total = 4;

    uint16_t temp = htons(packet->opcode);
    memcpy(buffer, &temp, 2);

    switch (packet->opcode) {
    case DATA:
        temp = htons(packet->data.block_id);
        memcpy(buffer + 2, &temp, 2);
        memcpy(buffer + 4, packet->data.buffer, packet->data.buffer_size);
        total += packet->data.buffer_size;
        break;
    case ACK:
        temp = htons(packet->ack.block_id);
        memcpy(buffer + 2, &temp, 2);
        break;
    case ERROR:
        temp = htons(packet->error.error_id);
        memcpy(buffer + 2, &temp, 2);
        strncpy(buffer + 4, packet->error.message, MAX_STRING_SIZE);
        total += strlen(buffer + 4) + 1;
        break;
    default:
        return 0;
    }

    return total;
}

void print_packet(packet_t *packet) {
    if (packet == NULL) {
        printf("Null Packet\n");
        return;
    }

    printf("-------[Packet]-------\n");
    printf("Opcode: [%u] ", packet->opcode);

    switch (packet->opcode) {
    case RRQ:
        printf("RRQ\n");
        printf("File Name = %s\n", packet->read.file_name);
        printf("Mode = %s\n", packet->read.mode);
        break;
    case WRQ:
        printf("WRQ\n");
        printf("File Name = %s\n", packet->write.file_name);
        printf("Mode = %s\n", packet->write.mode);
        break;
    case DATA:
        printf("DATA\n");
        printf("Block = %u\n", packet->data.block_id);
        printf("Data Size = %lu\n", packet->data.buffer_size);
        break;
    case ACK:
        printf("ACK\n");
        printf("Block = %u\n", packet->ack.block_id);
        break;
    case ERROR:
        printf("ERROR\n");
        printf("Error Code = %u\n", packet->error.error_id);
        printf("Error Message = %s\n", packet->error.message);
        break;
    default:
        printf("Unknown Packet Type\n");
        break;
    }

    printf("----------------------\n");
}

void print_error(packet_t *packet) {
    if (packet == NULL || packet->opcode != ERROR) {
        return;
    }

    printf("ERROR [");

    switch (packet->error.error_id) {
    case NOT_DEFINED:
        printf("Not Defined");
        break;
    case FILE_NOT_FOUND:
        printf("File Not Found");
        break;
    case ACCESS_VIOLATION:
        printf("Access Violation");
        break;
    case DISK_FULL:
        printf("Disk Full");
        break;
    case ILLEGAL_OPERATION:
        printf("Illegal Operation");
        break;
    case UNKNOWN_TRANSFER_ID:
        printf("Unknown Transfer ID");
        break;
    case FILE_ALREADY_EXISTS:
        printf("File Already Exists");
        break;
    case NO_SUCH_USER:
        printf("No Such User");
        break;
    default:
        printf("%u", packet->error.error_id);
        break;
    }

    printf("]: \"%s\"\n", packet->error.message);
}