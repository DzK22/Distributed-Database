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
} auth_user;

typedef struct node {
    struct sockaddr_in saddr;
    char field[MAX_ATTR];
    size_t id; // identifiant unique
    bool active; // mettre a false pour ne rien demander a ce serveur
} node;

/**
 * \struct mugiwara
 * \brief Structure contenant les utilisateurs (fichier), les clients actuellment connectés et les noeuds de données.
 */
typedef struct mugiwara
{
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
} relaisdata;


int fd_can_read (int fd, void *data);
int read_sck (relaisdata *rdata);
int exec_dg (const dgram *dg, relaisdata *rdata);

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

node * get_node_field_from_dg (const dgram *dg, const relaisdata *rdata);

auth_user * get_auth_user_from_login (const char *login, const relaisdata *rdata);
/*
int node_read_request (const int sock, clientreq *creq, mugiwara *mugi, user *usr);
int node_write_request (const int sock, clientreq *creq, mugiwara *mugi, user *usr);
int node_delete_request (const int sock, clientreq *creq, mugiwara *mugi, user *usr);

int meet_new_node (const int sock, clientreq *creq, mugiwara *mugi);

int follow_readres (const int sock, clientreq *creq, mugiwara *mugi);
int follow_writeres (const int sock, clientreq *creq, mugiwara *mugi);
int follow_deleteres (const int sock, clientreq *creq, mugiwara *mugi);
*/

#endif
