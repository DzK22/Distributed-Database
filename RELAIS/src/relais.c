#include "../headers/relais.h"

int socket_create()
{
  int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == -1)
  {
    perror("socket creation error");
    return -1;
  }
  return sock;
}

int candbind(int sockfd, struct sockaddr_in *addr)
{
  addr->sin_addr.s_addr = htonl(INADDR_ANY);
  addr->sin_family = AF_INET;
  addr->sin_port = htons(2100);
  socklen_t len = sizeof(struct sockaddr_in);
  int bd = bind(sockfd, (struct sockaddr *)addr, len);
  if (bd == -1)
  {
    perror("bind error");
    return -1;
  }
  return 0;
}

ssize_t send_toclient(int sockfd, const char *msg, struct sockaddr_in *client)
{
  socklen_t len = sizeof(struct sockaddr_in);
  ssize_t bytes = sendto(sockfd, msg, N, 0, (struct sockaddr *)client, len);
  if (bytes == -1)
  {
    perror("sendto error");
    return -1;
  }
  return bytes;
}
