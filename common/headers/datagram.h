#ifndef __DATAGRAM_H__
#define __DATAGRAM_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define DG_DATA_MAX 65536 // octets

// requetes et réponses
// CREQ = Client -> Relais
// RREQ = Relais -> Noeud
// RRES = Relais -> Client
// NRES = Noeud -> Relais
// -------------------------
// CREQ
#define CREQ_AUTH 1
#define CREQ_READ 2
#define CREQ_WRITE 3
#define CREQ_DELETE 4
// RREQ
#define RREQ_READ 5
#define RREQ_WRITE 6
#define RREQ_DELETE 7
#define RREQ_DATA 8
#define RREQ_SYNC 9
#define RREQ_DESTROY 10
// NRES
#define NRES_READ -1
#define NRES_WRITE -2
#define NRES_DELETE -3
#define NRES_DATA -4
#define NRES_SYNC -5
// RRES
#define RRES_AUTH -6
#define RRES_READ -7
#define RRES_WRITE -8
#define RRES_DELETE -9
// ACK
#define ACK 0

// status
#define SUC_DELETE 3
#define SUC_WRITE 2
#define SUC_AUTH 1
#define NORMAL 0
#define ERR_NOREPLY -1
#define ERR_AUTHFAILED -2
#define ERR_NOPERM -3
#define ERR_NONODE -4
#define ERR_SYNTAX -5
#define ERR_UNKNOWFIELD -6
#define ERR_WRITE -7
#define ERR_DELETE -8

typedef struct {
    // en-tête
    u_int16_t id; // 2 octets
    u_int8_t request; // 1 octet
    int8_t status; // 1 octet
    u_int16_t data_size; // 2 octets
    u_int16_t checksum; // 2 octets
    // data
    char *data; // data_len octets (max = DG_DATA_MAX)
    u_int16_t data_len;
    // divers
    u_int32_t addr;
    in_port_t port;
    dgram *next;
} dgram;
// dgram ready quand data_len == data_size !

int dgram_add_from_raw (dgram *dglist, dgram **newdg, void *raw, const size_t raw_size, const struct sockaddr_in *saddr);
dgram * dgram_del_from_ack (dgram *dglist, const u_int16_t ack);
void dgra
bool dgram_is_ready (dgram *dg);
int dgram_print_status (const dgram *dg);

#endif
