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

/**
* \socket_create().
* \brief Fonction qui crée une socket UDP.
* \return descripteur du socket qui vient d'être créer (int).
*/
int socket_create();
