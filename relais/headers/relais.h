/**
* \file relais.h
* \brief Déclarations des fonctions de gestion du serveur d'accès
* \author François et Danyl
*/

#ifndef __RELAIS_H__
#define __RELAIS_H__

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#define N 1024
#define MAX_ATTR 32
#define H 32

enum clientreq_codes {Authentification, Read, Write, Delete};

typedef struct clientreq {
    int type;
    char message[N];
    struct sockaddr_in saddr;
} clientreq;

typedef struct user {
  char login[N];
  char mdp[N];
  char attributs[MAX_ATTR][MAX_ATTR];
  size_t attributs_len;
} user;

typedef struct auth_user
{
  struct in_addr ip;
  uint16_t port;
  char login[H];
} auth_user;

typedef struct mugiwara
{
  user *users;        //tableah de tous les utilisateurs (fichier chargé)
  size_t nb_users;    //nombre d'utilisateurs stockés dans le fichier chargé
  auth_user *hosts;   //tableau des clients actuellement connectés
  size_t nb_hosts;    //clients actuellement connectés
  size_t max_hosts;   //max de clients pouvant se connecter
} mugiwara;

bool test_auth(mugiwara *mugi, const char *log1);

bool read_req(clientreq *creq, mugiwara *mugi);

mugiwara *init_mugiwara ();
/**
* \fn socket_create().
* \brief Fonction qui crée un socket.
* \return descripteur du socket qui vient d'être créer (int).
*/
int socket_create();

/**
* \fn candbind(const int sockfd, struct sockaddr_in *addr, const char *port).
* \brief Fonction qui initialise une structure sockaddr_in puis la lie avec un socket.
* \param [in] sockfd descripteur du socket à lier.
* \param [in] addr structure sockaddr_in à initialiser.
* \param [in] port port pour la structure à initialiser.
* \return int.
*/
int candbind(const int sockfd, struct sockaddr_in *addr, const char *port);

/**
* \fn send_toclient(const int sockfd, const char *msg, struct sockaddr_in *client).
* \brief Fonction qui envoie des données à un client.
* \param [in] sockfd descripteur du socket associé au client.
* \param [in] msg données à envoyer.
* \param [in] client informations du client (IP + PORT).
* \return nombre d'octets envoyés au client.
*/
ssize_t send_toclient(const int sockfd, void *msg, struct sockaddr_in *client);

/**
* \fn authentification(clientreq *creq).
* \brief Fonction qui teste si un client fait parti de la liste de clients autorisés à se connecter.
* \param [in] creq structure de requête client
* \return booléen (vrai si le client est autorisé à se connecter, faux sinon).
*/
bool authentification (clientreq *creq, mugiwara *mugi);

/**
* \fn cmd_test(char *recu, FILE *fp, const char *user).
* \brief Fonction qui teste la commande envoyé par le client
* \param [in] fp fichier contenant les id + droits des clients.
* \param [in] recu requête envoyée par le client.
* \param [in] user identitifiants de l'utilisateur.
* \return booléen (vrai si la commande du client est acceptée, faux sinon).
*/
bool cmd_test(char *recu, FILE *fp, const char *user);

/**
 * \fn parse_datagram (char *data, clientreq *cr);
 * \brief Fonction qui parse un datagram UDP et remplie la structure clientreq cr
 * \param [in] data le datagram UDP
 * \param [in] cr la structure clientreq
 * \return 0 si succès, -1 sinon
 */
int parse_datagram (char *data, clientreq *cr, struct sockaddr_in *client);

/**
 * \fn wait_for_request (int sock)
 * \param [in] sock le socket master
 * \return 0 si succès, -1 sinon
 */
int wait_for_request (int sock, mugiwara *mugi);

/**
 * \fn exec_client_request (int sock, clientreq *cr)
 * \param [in] sock le socket master
 * \param [in] cr la structure clientreq à éxecuter
 * \return 0 si succès, -1 sinon
 */
int exec_client_request (int sock, clientreq *cr, mugiwara *mugi);

#endif
