/**
 * \file clients.c.
 * \brief Définitions des fonctions qui gèrent la partie client du projet.
 * \author François et Danyl.
 */

#include "../headers/clients.h"

int fd_can_read (int fd, void *data)
{
    clientdata *cdata = ((clientdata *) data);
    if (fd == 0) { // stdin
        if (read_stdin(cdata) == -1)
            return -1;
    } else if (fd == cdata->sck) { // sck
        if (dgram_process_raw(cdata->sck, &cdata->dgsent, &cdata->dgreceived, cdata, exec_dg) == -1)
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

    if (!cdata->is_auth) {
        dgram_print_status(ERR_NOTAUTH);
        return 1;
    }

    data[strnlen(data, DG_DATA_MAX) - 1] = '\0';
    size_t data_len = strnlen(data, DG_DATA_MAX);
    char *request_str, *tmp;
    request_str = strtok_r(data, " ", &tmp);
    if (request_str == NULL) {
        perror("strtok_r");
        return -1;
    }

    uint8_t request;
    size_t instr_len;
    bool arg = false;
    if (strncmp(request_str, "lire", 5) == 0) {
        request = CREQ_READ;
        instr_len = 5;
        arg = true;
    } else if (strncmp(request_str, "ecrire", 7) == 0) {
        request = CREQ_WRITE;
        instr_len = 7;
        arg = true;
    } else if (strncmp(request_str, "supprimer", 10) == 0) {
        request = CREQ_DELETE;
        instr_len = 10;
        arg = false;
    } else {
        dgram_print_status(ERR_SYNTAX);
        return 1;
    }

    if ((!arg && (data_len > instr_len)) || (arg && (data_len <= instr_len))) {
        dgram_print_status(ERR_SYNTAX);
        return 1;
    }

    if (dgram_create_send(cdata->sck, &cdata->dgsent, NULL, cdata->id_counter ++, request, NORMAL, cdata->relais_saddr->sin_addr.s_addr, cdata->relais_saddr->sin_port, arg ? data_len : 0, arg ? data + instr_len : NULL) == -1)
       return -1;

    return 0;
}

int exec_dg (const dgram *dg, void *data)
{
   clientdata *cdata = data;
    switch (dg->request) {
        case RRES_AUTH:
            dgram_print_status(dg->status);
            if (dg->status == SUC_AUTH)
                cdata->is_auth = true;
            else  // exec, print & quit
                return -1;
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
        case ACK:
            dgram_del_from_id(&cdata->dgsent, dg->id);
            break;
        default:
            fprintf(stderr, "Paquet reçu avec requete non gérée par le client: %d\n", dg->request);
    }

    return 0;
}

int send_auth (const char *login, const char *password, clientdata *cdata)
{
    char buf[DG_DATA_MAX];
    int res = snprintf(buf, DG_DATA_MAX, "%s:%s", login, password);
    if (res >= DG_DATA_MAX) {
        fprintf(stderr, "authentificate: snprintf truncate\n");
        return -1;
    } else if (res < 0) {
        perror("snprintf");
        return -1;
    }

    dgram *dg;
    if (dgram_create_send(cdata->sck, &cdata->dgsent, &dg, cdata->id_counter ++, CREQ_AUTH, NORMAL, cdata->relais_saddr->sin_addr.s_addr, cdata->relais_saddr->sin_port, res, buf) == -1)
       return -1;

    return 0;
}

void print_read_res (const dgram *dg)
{
   printf("\033[36m%s\033[0m\n", dg->data);
}
