/**
* \file clients.h
* \brief Déclarations des fonctions de gestion de clients
* \author François et Danyl
*/

#ifndef __CLIENTS_H__
#define __CLIENTS_H__

#include "../../common/headers/datagram.h"
#include "../../common/headers/sck.h"

#define LOGIN_MAX 32

typedef struct {
    int sck;
    struct sockaddr_in *relais_saddr;
    dgram *dgsent;
    dgram *dgreceived;
    uint16_t id_counter;
    bool is_auth;
    sem_t gsem;
    char login[LOGIN_MAX];
} clientdata;

int fd_can_read (int fd, void *data);
int read_stdin (clientdata *cdata);
int send_auth (const char *login, const char *password, clientdata *cdata);
int exec_dg (const dgram *dg, void *data);
void print_read_res (const dgram *dg);
void print_prompt (const clientdata *cdata);
bool req_timeout (const dgram *dg);

#endif
