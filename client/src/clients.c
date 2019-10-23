/**
 * \file clients.c.
 * \brief Définitions des fonctions qui gèrent la partie client du projet.
 * \author François et Danyl.
 */

#include "../headers/clients.h"

int on_request (int fd, void *data)
{
    reqdata *rd = ((reqdata *) data);
    if (fd == 0) { // stdin
        if (read_stdin(rd) == -1)
            return -1;
    } else if (fd == rd->sck) { // sck
        if (read_sck(rd) == -1)
            return -1;
    }

    return 0;
}

int read_stdin (reqdata *rd)
{
    char data[SCK_DATAGRAM_MAX];
    errno = 0;
    if ((fgets(data, SCK_DATAGRAM_MAX - 1, stdin) == NULL) && (errno != 0)) {
        perror("gets");
        return -1;
    }
    size_t data_len = strnlen(data, SCK_DATAGRAM_MAX);
    char *request_str, *tmp;
    request_str = strtok_r(data, " ", &tmp);
    if (request_str == NULL) {
        perror("strtok_r");
        return -1;
    }

    uint8_t request;
    if (strncmp(request_str, "lire", 5) == 0)
        request = CREQ_READ;
    else if (strncmp(request_str, "ecrire", 7) == 0)
        request = CREQ_WRITE;
    else if (strncmp(request_str, "supprimer", 10) == 0)
        request = CREQ_DELETE;
    else {
        dgram_print_status(ERR_SYNTAX);
        return -1;
    }

    dgram *dg = malloc(sizeof(dgram));
    if (dg == NULL) {
        perror("malloc");
        return -1;
    }
    if (dgram_create(dg, rd->id_counter ++, request, 0, data_len, data) == -1)
        return -1;

    if (dgram_send(rd->sck, dg, &rd->dg_sent) == -1)
        return -1;

    return 0;
}

int read_sck (reqdata *rd)
{
    (void) rd;

    return 0;
}

int authentificate (const char *login, const char *password, reqdata *rd)
{
    (void) login, (void) password, (void) rd;

    return 0;
}
