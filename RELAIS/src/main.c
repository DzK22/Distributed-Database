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
  fd_set ensemble;
  struct timeval delai;
  delai.tv_sec = 0;
  delai.tv_usec = 0;
  int sel, maxsock = sock+1;
  while (1)
  {
    FD_ZERO(&ensemble);
    FD_SET(sock, &ensemble);
    sel = select(maxsock, &ensemble, NULL, NULL, &delai);
    switch (sel)
    {
      case -1:
        perror("select error");
        return EXIT_FAILURE;

      default:
        if (FD_ISSET(sock, &ensemble))
        {
          if ((rec = recvfrom(sock, &recu, N, 0, (struct sockaddr *)&client, &clientaddr)) == -1)
          {
            perror("recvfrom error");
            return EXIT_FAILURE;
          }
          if (cmd_test(recu))
            send_toclient(sock, "U CAN DO WHAT YOU WANT", &client);
          else
          {
            printf("TOTO\n");
            return EXIT_FAILURE;
          }
        }
    }
  }
  return EXIT_SUCCESS;
}
