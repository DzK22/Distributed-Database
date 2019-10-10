/**
* \file clients.h
* \brief Déclarations des fonctions de gestion de clients
* \author François et Danyl
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <arpa/inet.h>
#define N 1024
#define LEN sizeof(struct sockaddr_in)

/**
* \fn socket_create().
* \brief Fonction qui crée une socket UDP.
* \return descripteur du socket qui vient d'être créer (int).
*/
int socket_create();

/**
* \fn addr_create(struct sockaddr_in *addr, const char *src,  int port).
* \brief Fonction qui initialise une structure sockaddr_in (IP + port).
* \param [in] addr structure à allouer et initialiser.
* \param [in] ip adresse IP du serveur d'accès passée en paramètre du prog.
* \return int.
*/
int addr_create(struct sockaddr_in *addr, const char *ip);

/**
* \fn recfromserveur(int sockfd, char *msg, struct sockaddr_in *serveur).
* \brief Fonction qui récupère des données depuis un serveur.
* \param [in] sockfd socket associée au serveur.
* \param [in] msg données récupérées.
* \param [in] serveur infos du serveur (IP + port).
* \return nombres d'octets reçus.
*/
ssize_t recfromserveur(int sockfd, char *msg, struct sockaddr_in *serveur);

/**
* \fn sendtoserveur(int sockfd, const char *msg, struct sockaddr_in *serveur).
* \brief Fonction qui envoie des données au serveur.
* \param [in] sockfd socket associée au serveur.
* \param [in] msg données à envoyer.
* \param [in] serveur infos du serveur (IP + port).
* \return nombres d'octets envoyés.
*/
ssize_t sendtoserveur(int sockfd, const char *msg, struct sockaddr_in *serveur);
