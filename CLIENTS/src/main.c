/**
* \file main.c
* \brief Main Page! Gère le programme des clients
* \author François et Danyl
*/

#include "../headers/clients.h"

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "Usage %s <login> <mdp>\n", argv[0]);
    return EXIT_FAILURE;
  }
  int sock = socket_create();
  if (sock == -1)
    return EXIT_FAILURE;
  printf("socket numéro = %d\n", sock);

  struct sockaddr_in test, serveur;
  int port = addr_create(&test, "127.0.0.1");
  if (port == -1)
    return EXIT_FAILURE;

  char buff[N];
  int ret = snprintf(buff, N, "<%s>:<%s>", argv[1], argv[2]);
  if (ret > N || ret < 0)
  {
    fprintf(stderr, "snprintf error\n");
    return EXIT_FAILURE;
  }
  if (sendtoserveur(sock, buff, &test) == -1)
    return EXIT_FAILURE;

  if (recfromserveur(sock, buff, &serveur) == -1)
    return EXIT_FAILURE;
  printf("DECISION = %s\n", buff);
  return EXIT_SUCCESS;
}
