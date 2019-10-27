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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <math.h>

#define SCK_DATAGRAM_MAX 65546 // DG_DATA_MAX + DG_HEADER_SIZE de datagram.h

int sck_create ();
int sck_bind (const int sck, const uint16_t port);
ssize_t sck_send (const int sck, const struct sockaddr_in *saddr, const void *buf, const size_t buf_len);
ssize_t sck_recv (const int sck, void *buf, const size_t buf_max, const struct sockaddr_in *saddr);
int sck_create_saddr (struct sockaddr_in *saddr, const char *addr, const char *port);
int sck_wait_for_request (const int sck, const time_t delay, const bool use_stdin, void *cb_data, int (*callback) (int, void *));

#endif
