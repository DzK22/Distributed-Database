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

    struct sockaddr_in test;
    if (candbind(sock, &test, port) == -1)
        return EXIT_FAILURE;

    wait_for_request(sock);
    return EXIT_FAILURE;
}
