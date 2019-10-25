/**
 * \file clients.c.
 * \brief Définitions des fonctions qui gèrent la partie client du projet.
 * \author François et Danyl.
 */

#include "../headers/clients.h"

int on_request (int fd, void *data)
{
    clientdata *cdata = ((clientdata *) data);
    if (fd == 0) { // stdin
        if (read_stdin(cdata) == -1)
            return -1;
    } else if (fd == cdata->sck) { // sck
        if (read_sck(cdata) == -1)
            return -1;
    }

    return 0;
}

int read_stdin (clientdata *cdata)
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
    if (dgram_create(dg, cdata->id_counter ++, request, NORMAL, cdata->relais_saddr->sin_addr.s_addr, cdata->relais_saddr->sin_port, data_len, data) == -1)
        return -1;

    if (dgram_send(cdata->sck, dg, &cdata->dg_sent) == -1)
        return -1;

    return 0;
}

int read_sck (clientdata *cdata)
{
    char buf[SCK_DATAGRAM_MAX];
    struct sockaddr_in saddr;
    ssize_t bytes = sck_recv(cdata->sck, buf, SCK_DATAGRAM_MAX, &saddr);
    if (bytes == -1)
        return -1;

    dgram dg;
    if (dgram_add_from_raw(&cdata->dg_received, buf, bytes, &dg, &saddr) == -1)
        return -1;

    if (dgram_is_ready(&dg)) {
        // SEND ACK
        dgram ack;
        if (dgram_create(&ack, dg.id, ACK, NORMAL, dg.addr, dg.port, 0, NULL) == -1)
            return -1;
        if (dgram_send(cdata->sck, &dg, &cdata->dg_sent) == -1)
            return -1;

        if (exec_dg(&dg, cdata) == -1)
            return -1;
        // supprimer ce dg
        cdata->dg_received = dgram_del_from_id(cdata->dg_received, dg.id);
        if (cdata->dg_received == NULL)
            return -1;

    }

    return 0;
}

int send_auth (const char *login, const char *password, clientdata *cdata)
{
    (void) login, (void) password, (void) cdata;
    dgram *dg = malloc(sizeof(dgram));
    if (dg == NULL) {
        perror("malloc");
        return -1;
    }

    char buf[DG_DATA_MAX];
    int res = snprintf(buf, DG_DATA_MAX, "%s:%s", login, password);
    if (res >= DG_DATA_MAX) {
        fprintf(stderr, "authentificate: snprintf truncate\n");
        return -1;
    } else if (res < 0) {
        perror("snprintf");
        return -1;
    }

    if (dgram_create(dg, cdata->id_counter ++, CREQ_AUTH, NORMAL, cdata->relais_saddr->sin_addr.s_addr, cdata->relais_saddr->sin_port, res, buf) == -1)
        return -1;

    if (dgram_send(cdata->sck, dg, &cdata->dg_sent) == -1)
        return -1;

    return 0;
}

int exec_dg (const dgram *dg, clientdata *cdata)
{
    switch (dg->request) {
        case RRES_AUTH:
            if (dg->status == SUC_AUTH)
                cdata->is_auth = true;
            else // echec
                dgram_print_status(dg->status);
            break;
        case RRES_READ:
            if (dg->status == SUC_READ)
                print_read_res(dg);
            else
                dgram_print_status(dg->status);
            break;
        case RRES_WRITE:
            dgram_print_status(dg->status);
            break;
        case RRES_DELETE:
            dgram_print_status(dg->status);
            break;
        default:
            fprintf(stderr, "Paquet reçu avec requete non gérée par le client: %d\n", dg->request);
    }

    return 0;
}

void print_read_res (const dgram *dg)
{
   printf("\033[183m%s\033[0m", dg->data);
}
