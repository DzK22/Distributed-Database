/**
 * \file clients.c.
 * \brief Définitions des fonctions qui gèrent la partie client du projet.
 * \author François et Danyl.
 */

#include "../headers/clients.h"

int fd_can_read (int fd, void *data)
{
    clientdata *cdata = ((clientdata *) data);
    if (sem_wait(&cdata->gsem) == -1) {
        perror("sem_wait");
        return -1;
    }

    if (fd == 0) { // stdin
        if (read_stdin(cdata) == -1)
            return -1;
    } else if (fd == cdata->sck) { // sck
        if (dgram_process_raw(cdata->sck, &cdata->dgsent, &cdata->dgreceived, cdata, exec_dg) == -1)
            return -1;
    }

    if (sem_post(&cdata->gsem) == -1) {
        perror("sem_post");
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
        dgram tmp;
        tmp.status = ERR_NOTAUTH;
        tmp.request = RRES_AUTH;
        dgram_print_status(&tmp);
        return 1;
    }

    data[strnlen(data, DG_DATA_MAX) - 1] = '\0';
    size_t data_len = strnlen(data, DG_DATA_MAX);
    char *request_str, *tmp;
    request_str = strtok_r(data, " ", &tmp);
    if (request_str == NULL) {
        print_prompt(cdata);
        return 1;
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
    } else if (strncmp(request_str, "bye", 4) == 0) {
        request = CREQ_LOGOUT;
        instr_len = 4;
        arg = false;
    }
    else {
        dgram tmp;
        tmp.status = ERR_SYNTAX;
        dgram_print_status(&tmp);
        print_prompt(cdata);
        return 1;
    }

    if ((!arg && (data_len > instr_len)) || (arg && (data_len <= instr_len))) {
        dgram tmp;
        tmp.status = ERR_SYNTAX;
        dgram_print_status(&tmp);
        print_prompt(cdata);
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
            dgram_print_status(dg);
            if (dg->status == SUCCESS)
                cdata->is_auth = true;
            else  // exec, print & quit
                return -1;
            print_prompt(cdata);
            break;
        case RRES_READ:
            if (dg->status == SUCCESS)
                print_read_res(dg);
            else
                dgram_print_status(dg);
            print_prompt(cdata);
            break;
        case RRES_WRITE:
            dgram_print_status(dg);
            print_prompt(cdata);
            break;
        case RRES_DELETE:
            dgram_print_status(dg);
            print_prompt(cdata);
            break;
        case RRES_LOGOUT:
            dgram_print_status(dg);
            if (dg->status == SUCCESS)
                return -1; // quitter le prog
            break;
        case ACK:
            break;
        case PING:
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
    dg->resend_timeout_cb = auth_timeout;

    return 0;
}

void print_read_res (const dgram *dg)
{
    bool empty = false;
    if (strchr(dg->data, ':') == (dg->data + dg->data_len - 1))
        empty = true;
    if (empty)
        printf("  \033[36m%s aucune donnée\033[0m\n", dg->data);
    else
        printf("  \033[36m%s \033[0m\n", dg->data);
}

void print_prompt (const clientdata *cdata)
{
    printf("\033[33m[%s]\033[0m ", cdata->login);
    if (fflush(stdout) == -1)
        perror("fflush");
}

bool auth_timeout (const dgram *dg)
{
    (void) dg;
    dgram tmpdg;
    tmpdg.status = ERR_NOREPLY;
    dgram_print_status(&tmpdg);
    return false;
}
