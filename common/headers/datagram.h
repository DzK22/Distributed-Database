#ifndef __DATAGRAM_H__
#define __DATAGRAM_H__

#include "sck.h"

#define DG_DATA_MAX 65536 // octets
#define DG_HEADER_SIZE 10
#define DG_FIRST_2OCTET 0x0000 // 2 octets à 0
#define DG_DELETE_TIMEOUT 400 // si dgram reçu non complet apres 400ms, le supprimer
#define DG_RESEND_TIMEOUT 500 // si dgram envoyé non acquis apres 600ms, le réenvoyer

// requetes et réponses
// CREQ = Client -> Relais
// RREQ = Relais -> Noeud
// NREQ = Noeud -> Relais
// RRES = Relais -> Client
// NRES = Noeud -> Relais
// RNRES = Relais -> Noeud
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
#define RREQ_GETDATA 9
#define RREQ_SYNC 10
#define RREQ_DESTROY 11
// NREQ
#define NREQ_LOGOUT 12
#define NREQ_MEET 13
// NRES
#define NRES_READ 20
#define NRES_WRITE 21
#define NRES_DELETE 22
#define NRES_GETDATA 23
#define NRES_SYNC 24
// RRES
#define RRES_AUTH 25
#define RRES_READ 26
#define RRES_WRITE 27
#define RRES_DELETE 28
// RNRES
#define RNRES_MEET 30

// status
#define SUC_DELETE 40
#define SUC_WRITE 41
#define SUC_READ 42
#define SUC_AUTH 43
#define SUC_MEET 44
#define NORMAL 50
#define ERR_NOREPLY 51
#define ERR_AUTHFAILED 52
#define ERR_NOPERM 53
#define ERR_NONODE 54
#define ERR_SYNTAX 55
#define ERR_UNKNOWFIELD 56
#define ERR_WRITE 57
#define ERR_DELETE 58
#define ERR_NOTAUTH 59
#define ERR_ALREADYAUTH 60

typedef struct dgram_s {
    // en-tête (2 premiers octets = indiquer debut en-tête)
    uint16_t id; // 2 octets
    uint8_t request; // 1 octet
    int8_t status; // 1 octet
    uint16_t data_size; // 2 octets
    uint16_t checksum; // 2 octets
    // data
    char *data; // data_len octets (max = DG_DATA_MAX), a \0 is auto appended
    uint16_t data_len;
    // divers
    uint32_t addr; // format network
    in_port_t port; // format network
    struct timeval creation_time;
    struct dgram_s *next;
} dgram;
// dgram ready quand data_len == data_size !

typedef struct {
    int sck;
    dgram **dgsent;
    dgram **dgreceived;
} thread_targ;

int dgram_add_from_raw (dgram **dglist, void *raw, const size_t raw_size, dgram *curdg, const struct sockaddr_in *saddr);
bool dgram_del_from_id (dgram **dglist, const uint16_t id);
int dgram_print_status (const uint8_t request);
int dgram_check_timeout_delete (dgram **dgreceived);
int dgram_check_timeout_resend (const int sock, dgram **dgsent);
unsigned time_ms_diff (struct timeval *tv1, struct timeval *tv2);
uint16_t dgram_checksum (const dgram *dg);
int dgram_create (dgram **dgres, const uint16_t id, const uint8_t request, const uint8_t status, const uint32_t addr, const in_port_t port, const uint16_t data_size, const char *data);
dgram * dgram_add (dgram *dglist, dgram *dg);
int dgram_send (const int sck, dgram *dg, dgram **dg_sent);
void * thread_timeout_loop (void *arg);
bool dgram_is_ready (const dgram *dg);
bool dgram_verify_checksum (const dgram *dg);
void dgram_debug (const dgram *dg);
int dgram_create_send (const int sck, dgram **dgsent, dgram **dgres, const uint16_t id, const uint8_t request, const uint8_t status, const uint32_t addr, const in_port_t port, const uint16_t data_size, const char *data); // sert juste a appeler les fonctions dgram_create et dgram_send en une fonction
int dgram_process_raw (const int sck, dgram **dgsent, dgram **dgreceived, void *cb_data, int (*callback) (const dgram *, void *)); // ATTENTION ! callback est aooeké seulement si le diagramme créer / agrandi est ready !

#endif
