#ifndef __SCK_H__
#define __SCK_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#define SCK_DATAGRAM_MAX 65546 // DG_DATA_MAX + DG_HEADER_SIZE de datagram.h

int sck_create ();
int sck_bind (const int sck, const uint16_t port);
int sck_send (const int sck, const struct sockaddr_in *saddr, const char *buf, const size_t buf_len);
ssize_t sck_recv (const int sck, char *buf, const size_t buf_max, const struct sockaddr_in *saddr);

#endif
