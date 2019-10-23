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
#include <sys/time.h>
#include <stdint.h>
#include "sck.h"

#define DG_DATA_MAX 65536 // octets
#define DG_HEADER_SIZE 10
#define DG_FIRST_OCTET 0x0000 // 2 octets à 0
#define DG_DELETE_TIMEOUT 400 // si dgram reçu non complet apres 400ms, le supprimer
#define DG_RESEND_TIMEOUT 500 // si dgram envoyé non acquis apres 600ms, le réenvoyer

// requetes et réponses
// CREQ = Client -> Relais
// RREQ = Relais -> Noeud
// NREQ = Noeud -> Relais
// RRES = Relais -> Client
// NRES = Noeud -> Relais
// -------------------------
// CREQ
#define CREQ_AUTH 1
#define CREQ_READ 2
#define CREQ_WRITE 3
#define CREQ_DELETE 4
#define CREQ_LOGOUT 5
// RREQ
#define RREQ_READ 6
#define RREQ_WRITE 7
#define RREQ_DELETE 8
#define RREQ_DATA 9
#define RREQ_SYNC 10
#define RREQ_DESTROY 11
// NRES
#define NRES_READ -1
#define NRES_WRITE -2
#define NRES_DELETE -3
#define NRES_DATA -4
#define NRES_SYNC -5
// NREQ
#define NREQ_LOGOUT -6
// RRES
#define RRES_AUTH -7
#define RRES_READ -8
#define RRES_WRITE -9
#define RRES_DELETE -10
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

typedef struct dgram_s {
    // en-tête (2 premiers octets = indiquer debut en-tête)
    uint16_t id; // 2 octets
    uint8_t request; // 1 octet
    int8_t status; // 1 octet
    uint16_t data_size; // 2 octets
    uint16_t checksum; // 2 octets
    // data
    char *data; // data_len octets (max = DG_DATA_MAX)
    uint16_t data_len;
    // divers
    uint32_t addr; // format network
    in_port_t port; // format network
    struct timeval creation_time;
    struct dgram_s *next;
} dgram;
// dgram ready quand data_len == data_size !

int dgram_add_from_raw (dgram **dglist,void *raw, const size_t raw_size, const struct sockaddr_in *saddr);
dgram * dgram_del_from_ack (dgram *dglist, const uint16_t ack);
bool dgram_is_ready (dgram *dg);
int dgram_print_status (const dgram *dg);
int dgram_check_timeout_delete (dgram **dglist);
int dgram_check_timeout_resend (const int sock, dgram **dglist);
int dgram_resend (const int sock, dgram *dg);
unsigned time_ms_diff (struct timeval *tv1, struct timeval *tv2);

#endif
