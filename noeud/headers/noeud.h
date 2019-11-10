#ifndef __NOEUD_H__
#define __NOEUD_H__

#include "../../common/headers/datagram.h"
#include "../../common/headers/sck.h"

#define N 1024
#define ARGS_MAX 16
#define FIELD_MAX 32
#define DATAFILE_LINE_MAX 64

typedef struct {
    char login[FIELD_MAX];
    char value[FIELD_MAX];
} user_info;

typedef struct {
    int sck;
    const char *field;
    user_info *datas;
    size_t nb_infos;
    struct sockaddr_in relais_saddr;
    dgram *dgsent;
    dgram *dgreceived;
    unsigned id_counter;
    bool meet_success;
    sem_t gsem;
} nodedata;

int sck_can_read (const int sck, void *data);
int exec_dg (const dgram *dg, void *data);

int exec_rnres_meet (const dgram *dg, nodedata *ndata);
int exec_rreq_read (const dgram *dg, nodedata *ndata);
int exec_rreq_write (const dgram *dg, nodedata *ndata);
int exec_rreq_delete (const dgram *dg, nodedata *ndata);
int exec_rreq_getdata (const dgram *dg, nodedata *ndata);
int exec_rreq_sync (const dgram *dg, nodedata *ndata);

int send_meet (nodedata *ndata);
bool is_relais (const uint32_t addr, const in_port_t port, const nodedata *ndata);
bool meet_timeout (const dgram *dg);

#endif
