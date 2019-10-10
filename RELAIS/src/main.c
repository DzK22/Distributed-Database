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
  char *boucle;
  char ret[N];
  char azerty[N];
  bool ok = false;
  int rec;
  if ((rec = recvfrom(sock, &azerty, N, 0, (struct sockaddr *)&client, &clientaddr)) == -1)
  {
    perror("recvfrom error");
    return EXIT_FAILURE;
  }
  printf("%s\n", azerty);
  while ((boucle = fgets(ret,N,fp)) != NULL)
  {
    printf("%s\n", ret);
    if (strncmp(ret, azerty, strlen(azerty)) == 0)
      ok = true;
  }
  if (ok)
  {
    if (send_toclient(sock, "IT'S OK!", &client) == -1)
      return EXIT_FAILURE;
  }
  else
  {
    if (send_toclient(sock, "NO WAY!", &client) == -1)
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
