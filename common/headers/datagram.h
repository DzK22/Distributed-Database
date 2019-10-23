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
#include <math.h>
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
#define ACK 0
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
// NREQ
#define NREQ_LOGOUT 12
// NRES
#define NRES_READ 20
#define NRES_WRITE 21
#define NRES_DELETE 22
#define NRES_DATA 23
#define NRES_SYNC 24
// RRES
#define RRES_AUTH 25
#define RRES_READ 26
#define RRES_WRITE 27
#define RRES_DELETE 28

// status
#define SUC_DELETE 40
#define SUC_WRITE 41
#define SUC_AUTH 42
#define NORMAL 43
#define ERR_NOREPLY 44
#define ERR_AUTHFAILED 45
#define ERR_NOPERM 46
#define ERR_NONODE 47
#define ERR_SYNTAX 48
#define ERR_UNKNOWFIELD 49
#define ERR_WRITE 50
#define ERR_DELETE 51

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
int dgram_print_status (const uint8_t request);
int dgram_check_timeout_delete (dgram **dglist);
int dgram_check_timeout_resend (const int sock, dgram **dglist);
unsigned time_ms_diff (struct timeval *tv1, struct timeval *tv2);
uint16_t dgram_checksum (const dgram *dg);
int dgram_create (dgram *dg, const uint16_t id, const uint8_t request, const uint8_t status, const uint16_t data_size, char *data);
dgram * dgram_add (dgram *dglist, dgram *dg);
int dgram_send (const int sck, dgram *dg, dgram **dg_sent);

#endif
