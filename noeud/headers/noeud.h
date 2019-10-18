#ifndef __NOEUD_H__
#define __NOEUD_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

#define ARGS_MAX 16
#define MESS_MAX 1024
#define FIELD_MAX 32
#define DATAFILE_LINE_MAX 64

/* codes erreurs retournés au serveur relais:
 * (1) l'émetteur n'est pas le serveur relais
 * (2) la requête est incorrecte (pb lors du parse_datagram)
 * (3) le noeud ne possède pas le champ demandé
 */

enum relaisreq_types {Read, Write, Delete};
typedef struct relaisreq {
    int type;
    char args[ARGS_MAX];
} relaisreq;

typedef struct node_data {
    int sock;
    struct sockaddr_in saddr_relais;
    const char *field;
    const char *datafile;
} node_data;

int create_socket ();

int fill_node_data (int sock, const char *field, const char *datafile, const char *relais_ip, const char *relais_port, node_data *ndata);

int can_bind (const int sock);

int remember_relais_addr (const node_data *ndata);

int meet_relais (const node_data *ndata);

int send_to_relais (const int sock, const char *buf, const size_t buf_len);

ssize_t recv_from_relais (const int sock, char *buf, const size_t buf_max, struct sockaddr_in *sender_saddr);

void wait_for_request (const node_data *ndata);

int parse_datagram (const char *buf, relaisreq *rreq);

int exec_relais_request (const node_data *ndata, relaisreq *rreq);

int node_read (const node_data *ndata, const char *args);

int node_write (const node_data *ndata, const char *args);

int node_delete (const node_data *ndata, const char *args);

#endif
