/**
* \file clients.c
* \brief Définitions des fonctions qui gèrent la partie client du projet
* \author François et Danyl
*/

#include "../headers/clients.h"

int socket_create()
{
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == -1)
  {
    perror("Erreur de création de la socket");
    return -1;
  }
  return sock;
}
