#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#define N 1024

int socket_create();

int candbind(int sockfd, struct sockaddr_in *addr);

ssize_t send_toclient(int sockfd, const char *msg, struct sockaddr_in *client);

bool authentification(FILE *fp, const char *recu, size_t len);

bool cmd_test(char *recu);
