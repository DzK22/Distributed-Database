#include "../headers/sck.h"

int sck_create ()
{
    int sck = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sck == -1) {
        perror("socket");
        return -1;
    }
    return sck;
}

int sck_bind (const int sock, const uint16_t port) // format hote
{
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;
    const socklen_t saddr_len = sizeof(struct sockaddr_in);

    if (bind(sock, (struct sockaddr *) &saddr, saddr_len) == -1) {
        perror("bind");
        return -1;
    }
    return 0;
}

int sck_send (const int sck, const struct sockaddr_in *saddr, const char *buf, const size_t buf_len)
{
    ssize_t bytes;
    size_t total = 0;
    socklen_t salen = sizeof(struct sockaddr_in);
    while ((bytes = sendto(sck, buf + total, buf_len - total, 0, (struct sockaddr *) saddr, salen)) > 0)
        total += bytes;

    if (bytes == -1) {
        perror("send");
        return -1;
    }

    return 0;
}

ssize_t sck_recv (const int sck, char *buf, const size_t buf_max, const struct sockaddr_in *saddr)
{
    socklen_t salen = sizeof(struct sockaddr_in);
    ssize_t bytes;
    if ((bytes = recvfrom(sck, buf, buf_max , 0, (struct sockaddr *) saddr, &salen)) == -1) {
      perror("recvfrom");
      return -1;
    }

    return bytes;
}
