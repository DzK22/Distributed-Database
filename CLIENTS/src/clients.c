/**
* \file clients.c.
* \brief Définitions des fonctions qui gèrent la partie client du projet.
* \author François et Danyl.
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

int addr_create(struct sockaddr_in *addr, const char *ip)
{
  addr->sin_family = AF_INET;
  addr->sin_port = htons(2100);
  int ret = inet_pton(AF_INET, ip, &addr->sin_addr.s_addr);
  if (ret == -1)
  {
    perror("inet_pton error");
    return -1;
  }
  return 0;
}

ssize_t recfromserveur(int sockfd, char *msg, struct sockaddr_in *serveur)
{
  ssize_t bytes;
  socklen_t servlen = sizeof(struct sockaddr_in);
  if ((bytes = recvfrom(sockfd, msg, N, 0, (struct sockaddr *)serveur, &servlen)) == -1)
  {
    perror("recvfrom error");
    return -1;
  }
  return bytes;
}

ssize_t sendtoserveur(int sockfd, const char *msg, struct sockaddr_in *serveur)
{
  ssize_t bytes;
  socklen_t servlen = sizeof(struct sockaddr_in);
  if ((bytes = sendto(sockfd, msg, N, 0, (struct sockaddr *)serveur, servlen)) == -1)
  {
    perror("sendto error");
    return -1;
  }
  return bytes;
}
