/**
* \file clients.h
* \brief Déclarations des fonctions de gestion de clients
* \author François et Danyl
*/

#ifndef __CLIENTS_H__
#define __CLIENTS_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include "../../common/headers/datagram.h"
#include "../../common/headers/sck.h"

typedef struct clientdata_s {
    int sck;
    struct sockaddr_in *relais_saddr;
    dgram *dg_sent;
    dgram *dg_received;
    uint16_t id_counter;
    bool is_auth;
} clientdata;

int on_request (int fd, void *data);
int read_stdin (clientdata *cdata);
int read_sck (clientdata *cdata);
int send_auth (const char *login, const char *password, clientdata *cdata);
int exec_dg (const dgram *dg, clientdata *cdata);
void print_read_res (const dgram *dg);

#endif
