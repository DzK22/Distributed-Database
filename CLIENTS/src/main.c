/**
* \file main.c
* \brief Main Page! Gère le programme des clients
* \author François et Danyl
*/

#include "../headers/clients.h"

int main()
{
  int sock = socket_create();
  printf("socket numéro = %d\n", sock);
  return EXIT_SUCCESS;
}
