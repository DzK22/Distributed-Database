#include "../headers/relais.h"

int main()
{
  int sock = socket_create();
  if (sock == -1)
    return EXIT_FAILURE;

  struct sockaddr_in test, client;
  if (candbind(sock, &test) == -1)
    return EXIT_FAILURE;

  FILE *fp = fopen("users/users.txt", "r");
  if (fp == NULL)
  {
    perror("fopen error");
    return EXIT_FAILURE;
  }
  socklen_t clientaddr = sizeof(struct sockaddr_in);
  int rec;
  char recu[N];
  if ((rec = recvfrom(sock, &recu, N, 0, (struct sockaddr *)&client, &clientaddr)) == -1)
  {
    perror("recvfrom error");
    return EXIT_FAILURE;
  }
  printf("%s\n", recu);
  bool user = authentification(fp, recu, strlen(recu));
  if (user)
  {
    if (send_toclient(sock, "UR WELCOME BRO!", &client) == -1)
      return EXIT_FAILURE;
  }
  else
  {
    if (send_toclient(sock, "DENIED!", &client) == -1)
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
