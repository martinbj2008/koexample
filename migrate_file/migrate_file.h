#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int send_fd(int fd, int fd_to_send);
int recv_fd(int fd);
void test_tcp_sock(int fd);
