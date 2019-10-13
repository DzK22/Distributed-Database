#ifndef __NOEUDS_H__
#define __NOEUDS_H__

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

int candbind(const int sockfd, struct sockaddr_in *addr, const char *port);

ssize_t send_toclient(const int sockfd, const char *msg, struct sockaddr_in *client);

bool authentification(FILE *fp, const char *recu, const size_t len);

bool cmd_test(char *recu, FILE *fp, const char *user);

#endif
