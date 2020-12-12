#include "tftp.h"

packet_t *deserialize_packet(char *buffer, size_t buffer_size, packet_t *packet) {
    if (packet == NULL || buffer_size < 4)
        return NULL;

    memset(packet, 0, sizeof(packet_t));

    switch (packet->opcode = ntohs(*(uint16_t *)buffer)) {
    case TFTP_OPCODE_RRQ:
        strncpy(packet->read.filename, buffer + 2, MAX_STRING_SIZE);
        strncpy(packet->read.mode, buffer + 2 + strlen(packet->read.filename) + 1, MAX_MODE_SIZE);
        break;
    case TFTP_OPCODE_WRQ:
        strncpy(packet->write.filename, buffer + 2, MAX_STRING_SIZE);
        strncpy(packet->write.mode, buffer + 2 + strlen(packet->write.filename) + 1, MAX_MODE_SIZE);
        break;
    case TFTP_OPCODE_DATA:
        packet->data.block_num = ntohs(*((uint16_t *)buffer + 1));
        packet->data.data_size = buffer_size - 4;
        memcpy(packet->data.data, buffer + 4, packet->data.data_size);
        break;
    case TFTP_OPCODE_ACK:
        packet->ack.block_num = ntohs(*((uint16_t *)buffer + 1));
        break;
    case TFTP_OPCODE_ERROR:
        packet->error.ercode = ntohs(*((uint16_t *)buffer + 1));
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
	*(uint16_t *)buffer = htons(packet->opcode);

    switch (packet->opcode) {
    case TFTP_OPCODE_DATA:
	    *((uint16_t *)buffer + 1) = htons(packet->data.block_num);
        memcpy(buffer + 4, packet->data.data, packet->data.data_size);
        n += packet->data.data_size;
        break;
    case TFTP_OPCODE_ACK:
	    *((uint16_t *)buffer + 1) = htons(packet->ack.block_num);
        break;
    case TFTP_OPCODE_ERROR:
	    *((uint16_t *)buffer + 1) = htons(packet->error.ercode);
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
        printf("Block = %u\n", packet->data.block_num);
        printf("DataSize = %lu\n", packet->data.data_size);
        break;
    case TFTP_OPCODE_ACK:
        printf("ACK\n");
        printf("Block = %u\n", packet->ack.block_num);
        break;
    case TFTP_OPCODE_ERROR:
        printf("ERROR\n");
        printf("ErrorCode = %u\n", packet->error.ercode);
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

    switch (packet->error.ercode) {
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
        printf("%u", packet->error.ercode);
        break;
    }

    printf("] Message: %s\n", packet->error.message);
}