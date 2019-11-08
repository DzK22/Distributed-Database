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

ssize_t sck_send (const int sck, const struct sockaddr_in *saddr, const void *buf, const size_t buf_len)
{
    ssize_t bytes;
    size_t total = 0;
    socklen_t salen = sizeof(struct sockaddr_in);
    while ((bytes = sendto(sck, buf + total, buf_len - total, 0, (struct sockaddr *) saddr, salen)) > 0) {
        total += bytes;
        if (total == buf_len)
            break;
    }

    if (bytes == -1) {
        perror("send");
        return -1;
    }

    return total;
}

ssize_t sck_recv (const int sck, void *buf, const size_t buf_max, const struct sockaddr_in *saddr)
{
    socklen_t salen = sizeof(struct sockaddr_in);
    ssize_t bytes;
    if ((bytes = recvfrom(sck, buf, buf_max , 0, (struct sockaddr *) saddr, &salen)) == -1) {
      perror("recvfrom");
      return -1;
    }

    return bytes;
}

int sck_create_saddr (struct sockaddr_in *saddr, const char *addr, const char *port)
{
    saddr->sin_family = AF_INET;
    saddr->sin_port = htons(atoi(port));

    int res = inet_pton(AF_INET, addr, &saddr->sin_addr);
    if (res == -1) {
        perror("inet_pton error");
        return -1;
    } else if (res == 0) {
        fprintf(stderr, "inet_pton: src is not an valid ipv4 address\n");
        return -1;
    }

    return 0;
}

int sck_wait_for_request (const int sck, const time_t delay, const bool use_stdin, void *cb_data, int (*callback) (int, void *)) // wait for sck and stdin
{
    fd_set fdset;
    struct timeval tv;
    tv.tv_sec = delay;
    tv.tv_usec = 0;
    int sel, max = sck + 1;

    while (1) {
        FD_ZERO(&fdset);
        if (use_stdin)
            FD_SET(0, &fdset);
        FD_SET(sck, &fdset);
        tv.tv_sec = delay;
        tv.tv_usec = 0;
        sel = select(max, &fdset, NULL, NULL, &tv);

        switch (sel) {
            case -1:
                perror("select error");
                return -1;
            case 0:
                fprintf(stderr, "Timeout reached\n");
                return -1;
            default:
                if (use_stdin && FD_ISSET(0, &fdset)) {
                    if (callback(0, cb_data) == -1)
                        return -1;
                }
                if (FD_ISSET(sck, &fdset)) {
                    if (callback(sck, cb_data) == -1)
                        return -1;
                }
        }
    }

    return -1;
}
