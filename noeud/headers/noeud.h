#ifndef __NOEUD_H__
#define __NOEUD_H__

#include "../../common/headers/datagram.h"
#include "../../common/headers/sck.h"

#define N 1024
#define ARGS_MAX 16
#define FIELD_MAX 32
#define DATAFILE_LINE_MAX 64

typedef struct {
    int sck;
    const char *field;
    const char *datafile;
    struct sockaddr_in relais_saddr;
    dgram *dgsent;
    dgram *dgreceived;
    unsigned id_counter;
    bool meet_success;
} nodedata;

int fd_can_read (int fd, void *data);
int read_sck (nodedata *ndata);
int exec_dg (const dgram *dg, nodedata *ndata);

int exec_rnres_meet (const dgram *dg, nodedata *ndata);
int exec_rreq_read (const dgram *dg, nodedata *ndata);
int exec_rreq_write (const dgram *dg, nodedata *ndata);
int exec_rreq_delete (const dgram *dg, nodedata *ndata);
int exec_rreq_getdata (const dgram *dg, nodedata *ndata);
int exec_rreq_sync (const dgram *dg, nodedata *ndata);
int exec_rreq_destroy (const dgram *dg, nodedata *ndata);

int send_meet (nodedata *ndata);
bool is_relais (const struct sockaddr_in *saddr, const nodedata *ndata);
int delete_user_file_line (const char *username, const nodedata *ndata);

#endif
