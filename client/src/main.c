/**
 * \file main.c
 * \brief Main Page! Gère le programme des clients
 * \author François et Danyl
 */

#include "../headers/clients.h"

int main (int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <relais_ip> <relais_port> <login> <mdp>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *relais_ip = argv[1];
    const char *relais_port = argv[2];
    const char *login = argv[3];
    const char *password = argv[4];

    int sck = sck_create();
    if (sck == -1)
        return EXIT_FAILURE;

    struct sockaddr_in relais_saddr;
    if (sck_create_saddr(&relais_saddr, relais_ip, relais_port) == -1)
        return EXIT_FAILURE;

    reqdata rd;
    rd.sck = sck;
    rd.relais_saddr = &relais_saddr;
    rd.dg_sent = NULL;
    rd.dg_received = NULL;
    rd.id_counter = 0;
    if (authentificate(login, password, &rd) == -1)
        return EXIT_FAILURE;
    sck_wait_for_request(sck, 600, &rd, on_request);

    return EXIT_FAILURE;
}
