/**
 * \file relais.h
 * \brief Déclarations des fonctions de gestion du serveur d'accès
 * \author François et Danyl
 */

#ifndef __RELAIS_H__
#define __RELAIS_H__

#define N 1024
#define MAX_ATTR 32
#define H 32
#define PING_TIMEOUT 4
#define DISCONNECT_TIMEOUT 10

#include "../../common/headers/datagram.h"
#include "../../common/headers/sck.h"

typedef struct user {
    char login[N];
    char mdp[N];
    char attributs[MAX_ATTR][MAX_ATTR];
    size_t attributs_len;
} user;

typedef struct auth_user {
    struct sockaddr_in saddr;
    char login[H];
    time_t last_mess_time;
} auth_user;

typedef struct node {
    struct sockaddr_in saddr;
    char field[MAX_ATTR];
    size_t id; // identifiant unique
    bool active; // mettre a false pour ne rien demander a ce serveur
    time_t last_mess_time;
} node;

/**
 * \struct mugiwara
 * \brief Structure contenant les utilisateurs (fichier), les clients actuellment connectés et les noeuds de données.
 */
typedef struct mugiwara {
    user *users;        // tableau de tous les utilisateurs (fichier chargé)
    size_t nb_users;    // nombre d'utilisateurs stockés dans le fichier chargé

    auth_user *hosts;   // tableau des clients actuellement connectés
    size_t nb_hosts;    // clients actuellement connectés
    size_t max_hosts;   // max de clients pouvant se connecter

    node *nodes;        // tableau des noeuds connus
    size_t nb_nodes;    // nombre de noeuds actuels
    size_t max_nodes;   // max de noeuds
    size_t node_id_counter;
} mugiwara;

typedef struct {
    int sck;
    dgram *dgsent;
    dgram *dgreceived;
    unsigned id_counter;
    mugiwara *mugi;
    sem_t rsem;
    sem_t gsem;
} relaisdata;

int sck_can_read (const int sck, void *data);
int exec_dg (const dgram *dg, void *data);

// traitement des paquets
int exec_creq_auth (const dgram *dg, relaisdata *data);
int exec_creq_read (const dgram *dg, relaisdata *rdata);
int exec_creq_write (const dgram *dg, relaisdata *rdata);
int exec_creq_delete (const dgram *dg, relaisdata *rdata);
int exec_creq_logout (const dgram *dg, relaisdata *rdata);
int exec_nreq_logout (const dgram *dg, relaisdata *rdata);
int exec_nreq_meet (const dgram *dg, relaisdata *rdata);
int exec_nres_read (const dgram *dg, relaisdata *rdata);
int exec_nres_write (const dgram *dg, relaisdata *rdata);
int exec_nres_delete (const dgram *dg, relaisdata *rdata);
int exec_nres_getdata (const dgram *dg, relaisdata *rdata);
int exec_nres_sync (const dgram *dg, relaisdata *rdata);

bool test_auth (const char *login, const relaisdata *rdata);
user * read_has_rights (const dgram *dg, const relaisdata *rdata);
mugiwara *init_mugiwara ();
user * get_user_from_dg (const dgram *dg, const relaisdata *rdata);
node * get_node_from_dg (const dgram *dg, const relaisdata *rdata);
auth_user * get_auth_user_from_login (const char *login, const relaisdata *rdata);
void update_last_mess_time_from_dg (const dgram *dg, relaisdata *rdata);
void * rthread_check_loop (void *data);

bool node_send_timeout (const dgram *dg);

#endif
