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
#define N 1024

/**
* \fn socket_create().
* \brief Fonction qui crée une socket UDP.
* \return descripteur du socket qui vient d'être créer (int).
*/
int socket_create ();

/**
* \fn addr_create(struct sockaddr_in *addr const char *ip, cont char *port).
* \brief Fonction qui initialise une structure sockaddr_in (IP + port).
* \param [in] addr structure à allouer et initialiser.
* \param [in] ip du serveur relais
* \param [in] port du serveur relais
* \return 0 si succès, sinon -1
*/
int addr_create (struct sockaddr_in *addr, const char*ip, const char *port);

/**
* \fn recfromserveur(const int sockfd, char *msg, struct sockaddr_in *serveur).
* \brief Fonction qui récupère des données depuis un serveur.
* \param [in] sockfd socket associée au serveur.
* \param [in] msg données récupérées.
* \param [in] serveur infos du serveur (IP + port).
* \return nombres d'octets reçus.
*/
ssize_t recfromserveur (const int sockfd, void *msg, struct sockaddr_in *serveur);

/**
* \fn sendtoserveur(const int sockfd, const char *msg, struct sockaddr_in *serveur).
* \brief Fonction qui envoie des données au serveur.
* \param [in] sockfd socket associée au serveur.
* \param [in] msg données à envoyer.
* \param [in] serveur infos du serveur (IP + port).
* \return nombres d'octets envoyés.
*/
ssize_t sendtoserveur (const int sockfd, void *msg, struct sockaddr_in *serveur);

#endif
