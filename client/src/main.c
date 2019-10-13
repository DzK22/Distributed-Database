/**
 * \file main.c
 * \brief Main Page! Gère le programme des clients
 * \author François et Danyl
 */

#include "../headers/clients.h"

int main (int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <login> <mdp> <relais_port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *login = argv[1];
    const char *mdp = argv[2];
    const char *relais_port = argv[3];

    int sock = socket_create();
    if (sock == -1)
        return EXIT_FAILURE;
    printf("socket numéro = %d\n", sock);

    struct sockaddr_in test, serveur;
    addr_create(&test, relais_port);

    char buff[N];
    int ret = snprintf(buff, N, "%s:%s", login, mdp);
    if ((ret > N) || (ret < 0)) {
        fprintf(stderr, "snprintf error\n");
        return EXIT_FAILURE;
    }
    if (sendtoserveur(sock, buff, &test) == -1)
        return EXIT_FAILURE;

    if (recfromserveur(sock, buff, &serveur) == -1)
        return EXIT_FAILURE;

    if (strncmp(buff, "DENIED!", 7) == 0) {
        fprintf(stderr, "UR AUTHENTIFICATION HAS FAILED!\n");
        return EXIT_FAILURE;
    }
    else
        fprintf(stdout, "CONNEXION ACCEPTED\n");

    fd_set ensemble;
    struct timeval delai;
    delai.tv_sec = 600;
    delai.tv_usec = 0;
    int sel, maxsock = sock + 1;

    while (1) {
        FD_ZERO(&ensemble);
        FD_SET(0, &ensemble);
        FD_SET(sock, &ensemble);
        sel = select(maxsock, &ensemble, NULL, NULL, &delai);

        switch (sel) {
            case -1:
                perror("select error");
                return EXIT_FAILURE;

            case 0:
                fprintf(stderr, "Timeout reached\n");
                return EXIT_FAILURE;

            default:
                if (FD_ISSET(0, &ensemble)) {
                    ssize_t bytes = read(0, &buff, N);
                    if (bytes == -1) {
                        perror("read error");
                        return EXIT_FAILURE;
                    }
                    if (sendtoserveur(sock, buff, &test) == -1)
                        return EXIT_FAILURE;
                }

                if (FD_ISSET(sock, &ensemble)) {
                    ssize_t bytes = recfromserveur(sock, buff, &serveur);
                    if (bytes == -1)
                        return EXIT_FAILURE;
                    printf("ACCESS = %s\n", buff);
                }
        }
    }
    return EXIT_SUCCESS;
}
