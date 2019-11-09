/**
 * \file main.c
 * \brief Main Page! Gère le programme des clients
 * \author François et Danyl
 */

#include "../headers/clients.h"
extern bool dg_debug_active;
clientdata *cdata_global = NULL;

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

    dg_debug_active = false;

    clientdata cdata;
    cdata.sck = sck;
    cdata.relais_saddr = &relais_saddr;
    cdata.dgsent = NULL;
    cdata.dgreceived = NULL;
    cdata.id_counter = 0;
    cdata.is_auth = false;
    strncpy(cdata.login, login, LOGIN_MAX);

    if (sem_init(&cdata.gsem, 0, 1) == -1) {
        return EXIT_FAILURE;
    }
    cdata_global = &cdata;

    // start thread
    pthread_t th;
    thread_targ targ;
    targ.sck = cdata.sck;
    targ.dgsent = &cdata.dgsent;
    targ.dgreceived = &cdata.dgreceived;
    targ.gsem = &cdata.gsem;
    if ((errno = pthread_create(&th, NULL, thread_timeout_loop, &targ)) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    printf(" > Tentative de connexion au serveur ...\n");
    if (send_auth(login, password, &cdata) == -1)
        return EXIT_FAILURE;

    // signal handler
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) == -1) {
        perror("sigemptyset");
        return EXIT_FAILURE;
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    sck_wait_for_request(sck, 300, true, &cdata, fd_can_read);
    return EXIT_FAILURE;
}
