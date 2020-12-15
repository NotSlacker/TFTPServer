#include "packet.h"

void make_data(packet_t *packet, char *buffer, size_t nbuffer, uint16_t nblock) {
    packet->opcode = TFTP_OPCODE_DATA;
    packet->data.nblock = nblock;
    packet->data.nbuffer = nbuffer;
    memcpy(packet->data.buffer, buffer, nbuffer);
}

void make_ack(packet_t *packet, uint16_t nblock) {
    packet->opcode = TFTP_OPCODE_ACK;
    packet->ack.nblock = nblock;
}

void make_error(packet_t *packet, uint16_t nerror, char *message) {
    packet->opcode = TFTP_OPCODE_ERROR;
    packet->error.nerror = nerror;
    strncpy(packet->error.message, message, MAX_STRING_SIZE);
}


packet_t *deserialize_packet(char *buffer, size_t buffer_size, packet_t *packet) {
    if (packet == NULL || buffer_size < 4)
        return NULL;

    memset(packet, 0, sizeof(packet_t));

    uint16_t temp = 0;
    memcpy(&temp, buffer, 2);

    switch (packet->opcode = ntohs(temp)) {
    case TFTP_OPCODE_RRQ:
        strncpy(packet->read.filename, buffer + 2, MAX_STRING_SIZE);
        strncpy(packet->read.mode, buffer + 2 + strlen(packet->read.filename) + 1, MAX_MODE_SIZE);
        break;
    case TFTP_OPCODE_WRQ:
        strncpy(packet->write.filename, buffer + 2, MAX_STRING_SIZE);
        strncpy(packet->write.mode, buffer + 2 + strlen(packet->write.filename) + 1, MAX_MODE_SIZE);
        break;
    case TFTP_OPCODE_DATA:
        memcpy(&temp, buffer + 2, 2);
        packet->data.nblock = ntohs(temp);
        packet->data.nbuffer = buffer_size - 4;
        memcpy(packet->data.buffer, buffer + 4, packet->data.nbuffer);
        break;
    case TFTP_OPCODE_ACK:
        memcpy(&temp, buffer + 2, 2);
        packet->ack.nblock = ntohs(temp);
        break;
    case TFTP_OPCODE_ERROR:
        memcpy(&temp, buffer + 2, 2);
        packet->error.nerror = ntohs(temp);
        strncpy(packet->error.message, buffer + 4, MAX_STRING_SIZE);
        break;
    default:
        return NULL;
    }
    
    return packet;
}

size_t serialize_packet(const packet_t *packet, char *buffer) {
    if (buffer == NULL || packet == NULL)
        return 0;

    size_t n = 4;

    uint16_t temp = htons(packet->opcode);
    memcpy(buffer, &temp, 2);

    switch (packet->opcode) {
    case TFTP_OPCODE_DATA:
        temp = htons(packet->data.nblock);
        memcpy(buffer + 2, &temp, 2);
        memcpy(buffer + 4, packet->data.buffer, packet->data.nbuffer);
        n += packet->data.nbuffer;
        break;
    case TFTP_OPCODE_ACK:
        temp = htons(packet->ack.nblock);
        memcpy(buffer + 2, &temp, 2);
        break;
    case TFTP_OPCODE_ERROR:
        temp = htons(packet->error.nerror);
        memcpy(buffer + 2, &temp, 2);
        strncpy(buffer + 4, packet->error.message, MAX_STRING_SIZE);
        n += strlen(buffer + 4) + 1;
        break;
    default:
        return 0;
    }

    return n;
}

void print_packet(packet_t *packet) {
    if (packet == NULL) {
        printf("Null Packet\n");
        return;
    }

    printf("-------[Packet]-------\n");
    printf("Opcode: [%u] ", packet->opcode);

    switch (packet->opcode) {
    case TFTP_OPCODE_RRQ:
        printf("RRQ\n");
        printf("Filename = %s\n", packet->read.filename);
        printf("Mode = %s\n", packet->read.mode);
        break;
    case TFTP_OPCODE_WRQ:
        printf("WRQ\n");
        printf("Filename = %s\n", packet->write.filename);
        printf("Mode = %s\n", packet->write.mode);
        break;
    case TFTP_OPCODE_DATA:
        printf("DATA\n");
        printf("Block = %u\n", packet->data.nblock);
        printf("DataSize = %lu\n", packet->data.nbuffer);
        break;
    case TFTP_OPCODE_ACK:
        printf("ACK\n");
        printf("Block = %u\n", packet->ack.nblock);
        break;
    case TFTP_OPCODE_ERROR:
        printf("ERROR\n");
        printf("ErrorCode = %u\n", packet->error.nerror);
        printf("ErrorMessage = %s\n", packet->error.message);
        break;
    default:
        printf("Unknown Packet Type\n");
        break;
    }

    printf("----------------------\n");
}

void print_error(packet_t *packet) {
    if (packet->opcode != TFTP_OPCODE_ERROR)
        return;

    printf("Error: [");

    switch (packet->error.nerror) {
    case TFTP_ERROR_NOT_DEFINED:
        printf("Not Defined");
        break;
    case TFTP_ERROR_FILE_NOT_FOUND:
        printf("File Not Found");
        break;
    case TFTP_ERROR_ACCESS_VIOLATION:
        printf("Access Violation");
        break;
    case TFTP_ERROR_DISK_FULL:
        printf("Disk Full");
        break;
    case TFTP_ERROR_ILLEGAL_OPERATION:
        printf("Illegal Operation");
        break;
    case TFTP_ERROR_UNKNOWN_TRANSFER_ID:
        printf("Unknown Transfer ID");
        break;
    case TFTP_ERROR_FILE_ALREADY_EXISTS:
        printf("File Already Exists");
        break;
    case TFTP_ERROR_NO_SUCH_USER:
        printf("No Such User");
        break;
    default:
        printf("%u", packet->error.nerror);
        break;
    }

    printf("] Message: %s\n", packet->error.message);
}