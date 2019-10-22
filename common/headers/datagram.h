#ifndef __DATAGRAM_H__
#define __DATAGRAM_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#define DG_DATA_MAX 4096

// requetes et réponses
// CREQ = Client -> Relais
// RREQ = Relais -> Noeud
// RRES = Relais -> Client
// NRES = Noeud -> Relais

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
    unsigned id;
    unsigned short request;
    char *data[DG_DATA_MAX];
    short status;
    uint32_t addr;
    in_port_t port;
    bool ready;
} dgram;

dgram * dgram_add_from_raw (dgram *dglist, const char *raw);
void dgram_del_from_ack (dgram *dglist, unsigned ack);
int dgram_print_status (dgram *dg);

#endif
