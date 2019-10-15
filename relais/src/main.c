/**
* \file main.c
* \brief Main program!
* \author François et Danyl
*/

#include "../headers/relais.h"

int main (int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: <port>\n");
        return EXIT_FAILURE;
    }

    const char *port = argv[1];

    int sock = socket_create();
    if (sock == -1)
        return EXIT_FAILURE;

    struct sockaddr_in test, client;
    if (candbind(sock, &test, port) == -1)
        return EXIT_FAILURE;

    FILE *fp = fopen("users/users.txt", "r");
    if (fp == NULL) {
        perror("fopen error");
        return EXIT_FAILURE;
    }

    socklen_t clientaddr = sizeof(struct sockaddr_in);
    int rec;
    char recu[N] = "";
    char save[N] = "";
    if ((rec = recvfrom(sock, &recu, N, 0, (struct sockaddr *)&client, &clientaddr)) == -1) {
        perror("recvfrom error");
        return EXIT_FAILURE;
    }

    printf("%s\n", recu);
    bool user = authentification(fp, recu, strlen(recu));
    if (user) {
        snprintf(save, N, "%s", recu);
        printf("save = %s\n", save);
        if (send_toclient(sock, "UR WELCOME BRO!", &client) == -1)
            return EXIT_FAILURE;
    }
    else {
        if (send_toclient(sock, "DENIED!", &client) == -1)
            return EXIT_FAILURE;
    }

    if (fclose(fp) == EOF) {
        perror("fclose error");
        return EXIT_FAILURE;
    }

    fd_set ensemble;
    struct timeval delai;
    delai.tv_sec = 600;
    delai.tv_usec = 0;
    int sel, maxsock = sock + 1;

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

            case 0:
                fprintf(stderr, "Timeout reached\n");
                return EXIT_FAILURE;

            default:
                if (FD_ISSET(sock, &ensemble)) {
                    if ((rec = recvfrom(sock, &recu, N, 0, (struct sockaddr *)&client, &clientaddr)) == -1) {
                        perror("recvfrom error");
                        return EXIT_FAILURE;
                    }
                    if ((fp = fopen("users/users.txt", "r")) == NULL) {
                        perror("fopen error");
                        return EXIT_FAILURE;
                    }
                    printf("save encore = %s\n", save);
                    if (cmd_test(recu, fp, save))
                        send_toclient(sock, "U CAN DO WHAT YOU WANT", &client);
                    else
                        send_toclient(sock, "IT'S A NOP !", &client);

                    if (fclose(fp)) {
                        perror("fclose error");
                        return EXIT_FAILURE;
                    }
                }
        }
    }
    return EXIT_SUCCESS;
}
