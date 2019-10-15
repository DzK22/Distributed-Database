/**
* \file relais.h
* \brief Déclarations des fonctions de gestion du serveur d'accès
* \author François et Danyl
*/

#ifndef __RELAIS_H__
#define __RELAIS_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdbool.h>
#define N 1024

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
ssize_t send_toclient(const int sockfd, const char *msg, struct sockaddr_in *client);

/**
* \fn authentification(FILE *fp, const char *recu, const size_t len).
* \brief Fonction qui teste si un client fait parti de la liste de clients autorisés à se connecter.
* \param [in] fp fichier contenant les identitifiants des clients.
* \param [in] recu login + mdp envoyés par le client.
* \param [in] len taille du msg (login + mdp) envoyé par le client.
* \return booléen (vrai si le client est autorisé à se connecter, faux sinon).
*/
bool authentification(FILE *fp, const char *recu, const size_t len);

/**
* \fn cmd_test(char *recu, FILE *fp, const char *user).
* \brief Fonction qui teste la commande envoyé par le client
* \param [in] fp fichier contenant les id + droits des clients.
* \param [in] recu requête envoyée par le client.
* \param [in] user identitifiants de l'utilisateur.
* \return booléen (vrai si la commande du client est acceptée, faux sinon).
*/
bool cmd_test(char *recu, FILE *fp, const char *user);

#endif
