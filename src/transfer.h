#ifndef TRANSFER_H
#define TRANSFER_H

#include <signal.h>

int connect_socket_on(int port);

int wait_for_packet(int sock_fd, struct sockaddr* client_addr, uint16_t opcode, packet_t* packet);
int send_packet(int sock_fd, struct sockaddr* sock_info, packet_t* packet);

int send_data(int sock_fd, struct sockaddr* sock_info, uint16_t block_num, char* data, size_t data_size);
int send_ack(int sock_fd, struct sockaddr* sock_info, uint16_t block_num);
int send_error(int sock_fd, struct sockaddr* sock_info, uint16_t ercode, char* message);

size_t read_data(char *buffer, size_t n, FILE *file_h, int convert);
size_t write_data(char *buffer, size_t n, FILE *file_h, int convert);

int send_file(int sock_fd, struct sockaddr *client_addr, FILE *file_h, char *mode);
int recv_file(int sock_fd, struct sockaddr *client_addr, FILE *file_h, char *mode);

#endif