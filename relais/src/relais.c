// Auteurs: Danyl El-Kabir et François Grabenstaetter

#include "../headers/relais.h"

int sck_can_read (const int sck, void *data)
{
    (void) sck;
    relaisdata *rdata = ((relaisdata *) data);
    if (sem_wait(&rdata->rsem) == -1) {
        perror("sem_wait");
        return -1;
    }
    if (sem_wait(&rdata->gsem) == -1) {
        perror("sem_wait");
        return -1;
    }

    if (dgram_process_raw(rdata->sck, &rdata->dgsent, &rdata->dgreceived, rdata, exec_dg) == -1)
        return -1;

    if (sem_post(&rdata->gsem) == -1) {
        perror("sem_post");
        return -1;
    }
    if (sem_post(&rdata->rsem) == -1) {
        perror("sem_post");
        return -1;
    }

    return 0;
}

int exec_dg (const dgram *dg, void *data)
{
    relaisdata *rdata = data;
    printf("nb nodes = %ld | nb hosts = %ld\n", rdata->pd->nb_nodes, rdata->pd->nb_hosts);
    update_last_mess_time_from_dg(dg, rdata);
    switch (dg->request) {
        case CREQ_AUTH:
            if (exec_creq_auth(dg, rdata) == -1)
                return -1;
            break;
        case CREQ_READ:
            if (exec_creq_read(dg, rdata) == -1)
                return -1;
            break;
        case CREQ_WRITE:
            if (exec_creq_write(dg, rdata) == -1)
                return -1;
            break;
        case CREQ_DELETE:
            if (exec_creq_delete(dg, rdata) == -1)
                return -1;
            break;
        case CREQ_LOGOUT:
            if (exec_creq_logout(dg, rdata) == -1)
                return -1;
            break;
        case NREQ_LOGOUT:
            if (exec_nreq_logout(dg, rdata) == -1)
                return -1;
            break;
        case NREQ_MEET:
            if (exec_nreq_meet(dg, rdata) == -1)
                return -1;
            break;
        case NRES_READ:
            if (exec_nres_read(dg, rdata) == -1)
                return -1;
            break;
        case NRES_WRITE:
            if (exec_nres_write(dg, rdata) == -1)
                return -1;
            break;
        case NRES_DELETE:
            if (exec_nres_delete(dg, rdata) == -1)
                return -1;
            break;
        case NRES_GETDATA:
            if (exec_nres_getdata(dg, rdata) == -1)
                return -1;
            break;
        case NRES_SYNC:
            if (exec_nres_sync(dg, rdata) == -1)
                return -1;
            break;
        case PING:
            break;
        case ACK:
            break;
        default:
            fprintf(stderr, "Paquet reçu avec requete non gérée par le relais: %d\n", dg->request);
    }

    return 0;
}

int exec_creq_auth (const dgram *dg, relaisdata *rdata)
{
    char *tmp;
    const char *login = strtok_r(dg->data, ":", &tmp);
    const char *password = strtok_r(NULL, ":", &tmp);
    if (!login || !password) {
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_AUTH, ERR_SYNTAX, dg->addr, dg->port, 0, NULL) == -1)
            return -1;
        return 1;
    }

    char *flogin, *fpassword;
    bool login_ok, password_ok;
    size_t i;
    for (i = 0; i < rdata->pd->nb_users; i ++) {
        flogin = rdata->pd->users[i].login;
        fpassword = rdata->pd->users[i].mdp;
        size_t max_log = (strlen(flogin) >= strlen(login)) ? strlen(flogin) : strlen(login);
        size_t max_pass = (strlen(fpassword) >= strlen(password)) ? strlen(fpassword) : strlen(password);
        login_ok = strncmp(flogin, login, max_log) == 0;
        password_ok = strncmp(fpassword, password, max_pass) == 0;

        if (login_ok && password_ok) {
            if (test_auth(login, rdata)) {
                if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_AUTH, ERR_ALREADYAUTH, dg->addr, dg->port, 0, NULL) == -1)
                    return -1;
                return 1;
            }

            // OK
            if (rdata->pd->nb_hosts == 0) {
                rdata->pd->hosts = malloc(sizeof(auth_user) * H);
                if (rdata->pd->hosts == NULL) {
                    perror("malloc");
                    return -1;
                }
            }
            if (rdata->pd->nb_hosts >= rdata->pd->max_hosts) {
                rdata->pd->max_hosts += H;
                rdata->pd->hosts = realloc(rdata->pd->hosts, rdata->pd->max_hosts);
                if (rdata->pd->hosts == NULL) {
                    perror("realloc");
                    return -1;
                }
            }
            memset(rdata->pd->hosts[rdata->pd->nb_hosts].login, 0, H);
            strncpy(rdata->pd->hosts[rdata->pd->nb_hosts].login, login, H);
            struct sockaddr_in saddr;
            saddr.sin_family = AF_INET;
            saddr.sin_addr.s_addr = dg->addr;
            saddr.sin_port = dg->port;

            rdata->pd->hosts[rdata->pd->nb_hosts].saddr = saddr;
            rdata->pd->hosts[rdata->pd->nb_hosts].last_mess_time = time(NULL);
            rdata->pd->nb_hosts ++;

            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_AUTH, SUCCESS, dg->addr, dg->port, 0, NULL) == -1)
                return -1;

            return 0;
        }
    }

    if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_AUTH, ERR_AUTHFAILED, dg->addr, dg->port, 0, NULL) == -1)
        return -1;

    return 1;
}

int exec_creq_read (const dgram *dg, relaisdata *rdata)
{
    user *usr = read_has_rights(dg, rdata);
    if (usr == NULL) { // PAS LES DROITS
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_READ, ERR_NOPERM, dg->addr, dg->port, 0, NULL) == -1)
            return -1;
        return 1;
    }

    // recherche d'un noeud valide pour chaque champ de la requête
    size_t i;
    char *tmp, *field, buf[DG_DATA_MAX];
    int val;
    size_t cpt = 0;
    bool field_filled;

    field = strtok_r(dg->data, ",", &tmp);
    while (field != NULL) {
        field_filled = false;
        for (i = 0; i < rdata->pd->nb_nodes; i ++) {
            if (strncmp(field, rdata->pd->nodes[i].field, strlen(field)) != 0)
                continue;
            if (!rdata->pd->nodes[i].active)
                continue;

            // OK
            val = snprintf(buf, DG_DATA_MAX, "%s:%s", usr->login, field);
            if (val >= DG_DATA_MAX) {
                fprintf(stderr, "snprintf truncate\n");
                return 1;
            } else if (val < 0) {
                perror("snprintf");
                return -1;
            }

            dgram *new_dg;
            if (dgram_create_send(rdata->sck, &rdata->dgsent, &new_dg, rdata->id_counter ++, RREQ_READ, NORMAL, rdata->pd->nodes[i].saddr.sin_addr.s_addr, rdata->pd->nodes[i].saddr.sin_port, val, buf) == -1)
                return -1;
            new_dg->resend_timeout_cb = node_send_timeout;
            new_dg->resend_timeout_cb_cparam = rdata;
            cpt ++;
            field_filled = true;
            break;
        }

        if (!field_filled) {
            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_READ, ERR_NONODE, dg->addr, dg->port, strlen(field), field) == -1)
                return -1;

        }

        field = strtok_r(NULL, ",", &tmp);
    }


    return 0;
}

int exec_creq_write (const dgram *dg, relaisdata *rdata)
{
    // format de dg->data = <field1>:<value1>,...,<fieldN>:<valueN>
    size_t i;
    char *tmp, *tmp2, *field_plus_val, *field, *value;
    char data_cpy[DG_DATA_MAX], buf[DG_DATA_MAX], field_plus_val_cpy[MAX_ATTR];
    int bytes;
    bool filled;

    user *usr = get_user_from_dg(dg, rdata);
    if (usr == NULL) {
        // send err not auth
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_WRITE, ERR_NOTAUTH, dg->addr, dg->port, 0, NULL) == -1)
            return -1;
        return 1;
    }

    size_t real_count = 0;
    auth_user *host = get_auth_user_from_login(usr->login, rdata);
    if (host == NULL)
        return 1;

    // syntax check
    bool syntax_err = false;
    memset(data_cpy, 0, DG_DATA_MAX);
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    field_plus_val = strtok_r(data_cpy, ",", &tmp);
    if (field_plus_val == NULL) {
        syntax_err = true;
        printf("fpv errro\n");
    }
    while (!syntax_err && (field_plus_val != NULL)) {
        tmp2 = NULL;
        memset(field_plus_val_cpy, 0, MAX_ATTR);
        strncpy(field_plus_val_cpy, field_plus_val, MAX_ATTR);
        field = strtok_r(field_plus_val_cpy, ":", &tmp2);
        if ((field == NULL) || (strnlen(field, MAX_ATTR) == 0))  {
            printf("field errro\n");
            syntax_err = true;
            break;
        }
        value = strtok_r(NULL, ":", &tmp2);
        if ((value == NULL) || (strnlen(value, MAX_ATTR) == 0)) {
            printf("value errro\n");
            if (value == NULL)
                printf("value == null\n");
            else
                printf("strnlen == 0\n");
            syntax_err = true;
            break;
        }

        field_plus_val = strtok_r(NULL, ",", &tmp);
    }

    if (syntax_err) {
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_WRITE, ERR_SYNTAX, dg->addr, dg->port, 0, NULL) == -1)
            return -1;
        return 1;
    }

    // verifier les noeuds sinon NONODE
    memset(data_cpy, 0, DG_DATA_MAX);
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    tmp = NULL;
    field_plus_val = strtok_r(data_cpy, ",", &tmp);
    while (field_plus_val != NULL) {
        filled = false;
        for (i = 0; i < rdata->pd->nb_nodes; i ++) {
            tmp2 = NULL;
            memset(field_plus_val_cpy, 0, MAX_ATTR);
            strncpy(field_plus_val_cpy, field_plus_val, MAX_ATTR);
            field = strtok_r(field_plus_val_cpy, ":", &tmp2);
            if (strncmp(field, rdata->pd->nodes[i].field, strnlen(field, MAX_ATTR)) != 0)
                continue;
            if (!rdata->pd->nodes[i].active)
                continue;
            filled = true;
        }

        if (!filled) {
            // no node found for some field(s)
            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_WRITE, ERR_NONODE, dg->addr, dg->port, strlen(field), field) == -1)
                return -1;
            return 1;
        }

        field_plus_val = strtok_r(NULL, ",", &tmp);
    }

    waiting_res *wr = add_node_responses(host, RRES_WRITE, rdata);
    if (wr == NULL)
        return 1;

    // main loop to send data
    memset(data_cpy, 0, DG_DATA_MAX);
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    tmp = NULL;
    field_plus_val = strtok_r(data_cpy, ",", &tmp);
    printf("data = %s\n", data_cpy);
    while (field_plus_val != NULL) {
        for (i = 0; i < rdata->pd->nb_nodes; i ++) {
            tmp2 = NULL;
            bytes = snprintf(field_plus_val_cpy, MAX_ATTR, "%s", field_plus_val);
            if (bytes < 0) {
                perror("snprintf");
                return -1;
            }
            field = strtok_r(field_plus_val_cpy, ":", &tmp2);
            if (strncmp(field, rdata->pd->nodes[i].field, strnlen(field, MAX_ATTR)) != 0)
                continue;
            if (!rdata->pd->nodes[i].active)
                continue;

            // OK
            value = strtok_r(NULL, ":", &tmp2);
            bytes = snprintf(buf, DG_DATA_MAX, "%ld:%s:%s", wr->id, usr->login, value);
            if (bytes >= N) {
                fprintf(stderr, "snprintf truncate\n");
                return 1;
            } else if (bytes < 0) {
                perror("snprintf");
                return -1;
            }

            // send to node
            dgram *new_dg;
            if (dgram_create_send(rdata->sck, &rdata->dgsent, &new_dg, rdata->id_counter ++, RREQ_WRITE, NORMAL, rdata->pd->nodes[i].saddr.sin_addr.s_addr, rdata->pd->nodes[i].saddr.sin_port, bytes, buf) == -1)
                return -1;

            real_count++;
            new_dg->resend_timeout_cb = node_send_timeout;
            new_dg->resend_timeout_cb_cparam = rdata;
        }

        field_plus_val = strtok_r(NULL, ",", &tmp);
    }

    int ind = get_ind_from_wait(wr->id, rdata);
    rdata->pd->node_responses[ind].nb_send = real_count;

    return 0;
}

/*return int*/
waiting_res * add_node_responses (auth_user *host, int req_type, relaisdata *rdata)
{
    waiting_res *wr = malloc(sizeof(waiting_res));
    if (wr == NULL) {
        perror("malloc error");
        return NULL;
    }
    wr->id = rdata->pd->responses_counter ++;
    wr->nb_rec = 0;
    wr->to = host;
    wr->success_type = req_type;
    wr->last_time_rec = time(NULL);
    rdata->pd->node_responses[rdata->pd->nb_responses++] = *wr;
    return wr;
}

int exec_creq_delete (const dgram *dg, relaisdata *rdata)
{
    user *usr = get_user_from_dg(dg, rdata);
    if (usr == NULL) {
        // not auth !
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_DELETE, ERR_NOTAUTH, dg->addr, dg->port, 0, NULL) == -1)
            return -1;
        return 1;
    }
    bool filled = false;
    size_t i, real_count = 0;

    auth_user *host = get_auth_user_from_login(usr->login, rdata);
    if (host == NULL)
        return 1;

    waiting_res *wr = add_node_responses(host, RRES_DELETE, rdata);
    if (wr == NULL)
        return 1;

    for (i = 0; i < rdata->pd->nb_nodes; i ++) {
        if (!rdata->pd->nodes[i].active)
            continue;

        char buf[DG_DATA_MAX];
        snprintf(buf, DG_DATA_MAX, "%ld:%s", wr->id, usr->login);
        dgram *new_dg;
        if (dgram_create_send(rdata->sck, &rdata->dgsent, &new_dg, rdata->id_counter ++, RREQ_DELETE, NORMAL, rdata->pd->nodes[i].saddr.sin_addr.s_addr, rdata->pd->nodes[i].saddr.sin_port, strnlen(buf, MAX_ATTR), buf) == -1)
            return -1;

        real_count++;
        new_dg->resend_timeout_cb = node_send_timeout;
        new_dg->resend_timeout_cb_cparam = rdata;
        filled = true;
    }

    if (!filled) {
        //aucun noeud trouvé
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_DELETE, ERR_NONODE, dg->addr, dg->port, 0, NULL) == -1)
            return -1;
        return 1;
    }
    int ind = get_ind_from_wait(wr->id, rdata);
    rdata->pd->node_responses[ind].nb_send = real_count;
    return 0;
}

int exec_creq_logout (const dgram *dg, relaisdata *rdata)
{
    user *usr = get_user_from_dg(dg, rdata);
    if (usr == NULL) {
        // not auth !
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_DELETE, ERR_NOTAUTH, dg->addr, dg->port, 0, NULL) == -1)
            return -1;
        return 1;
    }

    size_t i;
    for (i = 0; i < rdata->pd->nb_hosts; i++)
    {
        if (strncmp(usr->login, rdata->pd->hosts[i].login, strlen(usr->login)) == 0) {
            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_LOGOUT, SUCCESS, rdata->pd->hosts[i].saddr.sin_addr.s_addr, rdata->pd->hosts[i].saddr.sin_port, 0, NULL) == -1) {
                return -1;
            }
            memmove(&rdata->pd->hosts[i], &rdata->pd->hosts[i + 1], sizeof(auth_user) * (rdata->pd->nb_hosts - i - 1));
            rdata->pd->nb_hosts--;

        }
    }
    return 0;
}

int exec_nreq_logout (const dgram *dg, relaisdata *rdata)
{
    (void) dg, (void) rdata;

    return 0;
}

int exec_nreq_meet (const dgram *dg, relaisdata *rdata)
{
    // send confirmation
    if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RNRES_MEET, SUCCESS, dg->addr, dg->port, 0, NULL) == -1)
        return -1;

    if (rdata->pd->nb_nodes >= rdata->pd->max_nodes) {
        rdata->pd->max_nodes += H;
        rdata->pd->nodes = realloc(rdata->pd->nodes, sizeof(node) * rdata->pd->max_nodes);
        if (rdata->pd->nodes == NULL) {
            perror("realloc");
            return -1;
        }
    }

    // ajouter le noeud
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = dg->addr;
    saddr.sin_port = dg->port;
    memset(rdata->pd->nodes[rdata->pd->nb_nodes].field, 0, MAX_ATTR);
    strncpy(rdata->pd->nodes[rdata->pd->nb_nodes].field, dg->data, MAX_ATTR);
    rdata->pd->nodes[rdata->pd->nb_nodes].saddr = saddr;
    rdata->pd->nodes[rdata->pd->nb_nodes].id = rdata->pd->node_id_counter;
    rdata->pd->nodes[rdata->pd->nb_nodes].active = true;
    rdata->pd->nodes[rdata->pd->nb_nodes].last_mess_time = time(NULL);
    rdata->pd->nb_nodes ++;
    rdata->pd->node_id_counter ++;

    // si un noeud du meme type existe deja, ecraser les données du nouveau noeud par les siennes (surement plus à jour) envoie message a ce noeud pour lui demander toutes ses donnees
    size_t i;
    for (i = 0; i < rdata->pd->nb_nodes; i ++) {
        if (rdata->pd->nodes[i].id == (rdata->pd->node_id_counter - 1))
            continue;
        else if (strncmp(dg->data, rdata->pd->nodes[i].field, MAX_ATTR) == 0) {
            // bloquer temporairement le noeud car n'a pas les données a jour
            rdata->pd->nodes[rdata->pd->nb_nodes - 1].active = false;

            // SEND MSG GETDATA TO THIS NODE
            // format message a envoyer = <NEW_NODE_ID>
            char buf[DG_DATA_MAX];
            int res = snprintf(buf, DG_DATA_MAX, "%ld", rdata->pd->node_id_counter - 1);
            if (res < 0) {
                perror("snprintf");
                return -1;
            } else if (res >= DG_DATA_MAX) {
                fprintf(stderr, "snprintf truncate\n");
                return -1;
            }
            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RREQ_GETDATA, NORMAL, rdata->pd->nodes[i].saddr.sin_addr.s_addr, rdata->pd->nodes[i].saddr.sin_port, res, buf) == -1)
                return -1;

            break;
        }
    }

    printf("new node registered for field = %s\n", dg->data);
    return 0;
}

int exec_nres_read (const dgram *dg, relaisdata *rdata)
{
    // /!/ dg->data = <user>:<data>
    char *username, *tmp, data_cpy[DG_DATA_MAX];
    memset(data_cpy, 0, DG_DATA_MAX);
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    username = strtok_r(data_cpy, ":", &tmp);
    if (username == NULL) {
        fprintf(stderr, "exec_nres_read: username == NULL\n");
        return 1;
    }

    node *nd = get_node_from_dg(dg, rdata);
    if (nd == NULL)
        return 1;

    char buf[DG_DATA_MAX];
    int val = snprintf(buf, DG_DATA_MAX, "%s:%s", nd->field, dg->data + strnlen(username, N) + 1);
    if (val >= DG_DATA_MAX) {
        fprintf(stderr, "snprintf truncate\n");
        return 1;
    } else if (val < 0) {
        perror("snprintf");
        return -1;
    }

    auth_user *authusr = get_auth_user_from_login(username, rdata);
    if (authusr == NULL)
        return 1;

    if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_READ, SUCCESS, authusr->saddr.sin_addr.s_addr, authusr->saddr.sin_port, strnlen(buf, DG_DATA_MAX), buf) == -1)
        return -1;

    return 0;
}

int exec_nres_write (const dgram *dg, relaisdata *rdata)
{
    // /!/ dg->data = <user>
    char *id = get_id_from_dg(dg);
    if (id == NULL)
        return 1;

    int identifiant = atoi(id);
    int ind = get_ind_from_wait(identifiant, rdata);
    if (ind == -1)
        return 1;
    rdata->pd->node_responses[ind].nb_rec ++;

    if (check_node_responses(identifiant, rdata) == -1)
        return -1;
    return 0;
}

char * get_id_from_dg(const dgram *dg)
{
    char *id, *tmp, data_cpy[DG_DATA_MAX];
    memset(data_cpy, 0, DG_DATA_MAX);
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    id = strtok_r(data_cpy, ":", &tmp);
    if (!id) {
        fprintf(stderr, "strtok_r error ?\n");
        return NULL;
    }
    return id;
}

int exec_nres_delete (const dgram *dg, relaisdata *rdata)
{
    // /!/ dg->data = <user>
    char *id = get_id_from_dg(dg);
    if (id == NULL)
        return 1;

    int identifiant = atoi(id);
    int ind = get_ind_from_wait(identifiant, rdata);
    if (ind == -1)
        return 1;
    rdata->pd->node_responses[ind].nb_rec++;
    if (check_node_responses(identifiant, rdata) == -1)
        return 1;
    return 0;
}

int check_node_responses (int resp_id, relaisdata *rdata)
{
    int ind = get_ind_from_wait(resp_id, rdata);
    if (ind == -1)
        return 1;

    const waiting_res *wr = &rdata->pd->node_responses[ind];
    if (wr->nb_rec == wr->nb_send) {
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, wr->success_type, SUCCESS, wr->to->saddr.sin_addr.s_addr, wr->to->saddr.sin_port, 0, NULL) == -1)
            return -1;

        memmove(&rdata->pd->node_responses[ind], &rdata->pd->node_responses[ind + 1], sizeof(waiting_res) * (rdata->pd->nb_responses - ind - 1));
        rdata->pd->nb_responses --;
    }
    return 0;
}

int exec_nres_getdata (const dgram *dg, relaisdata *rdata)
{
    // dg->data = <NODE_ID>:<DATA>
    char data_cpy[DG_DATA_MAX];
    memset(data_cpy, 0, DG_DATA_MAX);
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    char *tmp;
    char *node_id = strtok_r(data_cpy, ":", &tmp);
    const size_t did = atol(node_id);
    char *node_data = NULL;
    if (dg->data_len >= (did + 2))
        node_data = node_id + strlen(node_id) + 1;

    if (!node_id) {
        // ATTENTION: si node_data est null, NE PAS QUITTER, sinon le relais ne pourra pas mettre active = true lors de NRES_SYNC au noeud
        return 1;
    }

    // envoyer les nouvelles données au noeud qui a l'id "data_id"
    for (size_t i = 0; i < rdata->pd->nb_nodes; i ++) {
        if (rdata->pd->nodes[i].id != did)
            continue;

        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RREQ_SYNC, NORMAL, rdata->pd->nodes[i].saddr.sin_addr.s_addr, rdata->pd->nodes[i].saddr.sin_port, node_data ? strlen(node_data) : 0, node_data) == -1)
            return -1;

        break;
    }

    return 0;
}

int exec_nres_sync (const dgram *dg, relaisdata *rdata)
{
    // juste mettre active = true pour le node concerné
    node *nd = get_node_from_dg(dg, rdata);
    if (nd == NULL)
        return 1;
    nd->active = true;

    return 0;
}

auth_user * get_auth_user_from_login (const char *login, const relaisdata *rdata)
{
    size_t i, login_len = strlen(login);
    for (i = 0; i < rdata->pd->nb_hosts; i ++) {
        if (strncmp(login, rdata->pd->hosts[i].login, login_len) == 0)
            return &rdata->pd->hosts[i];
    }

    return NULL;
}

ssize_t get_ind_from_wait(const size_t id, const relaisdata *rdata)
{
    size_t i;
    for (i = 0; i < rdata->pd->nb_responses; i++) {
        if (id == rdata->pd->node_responses[i].id)
            return i;
    }
    return -1;
}

node * get_node_from_dg (const dgram *dg, const relaisdata *rdata)
{
    size_t i;
    bool ip_ok, port_ok;
    for (i = 0; i < rdata->pd->nb_nodes; i ++)
    {
        ip_ok = dg->addr == rdata->pd->nodes[i].saddr.sin_addr.s_addr;
        if (!ip_ok)
            continue;
        port_ok = dg->port == rdata->pd->nodes[i].saddr.sin_port;
        if (!port_ok)
            continue;

        return &rdata->pd->nodes[i];
    }
    return NULL;
}

user * get_user_from_dg (const dgram *dg, const relaisdata *rdata)
{
    // verifie que le user qui correspond a la requete est bien connecté sinon NULL
    size_t i;
    bool ip_ok, port_ok;
    for (i = 0; i < rdata->pd->nb_hosts; i ++) {
        ip_ok = dg->addr == rdata->pd->hosts[i].saddr.sin_addr.s_addr;
        if (!ip_ok)
            continue;
        port_ok = dg->port == rdata->pd->hosts[i].saddr.sin_port;
        if (!port_ok)
            continue;
        // OK
        const char *login = rdata->pd->hosts[i].login;
        const size_t login_len = strlen(login);
        // search for the corresponding user in the users array
        for (i = 0; i < rdata->pd->nb_users; i ++) {
            if (strncmp(login, rdata->pd->users[i].login, login_len) == 0)
                return &rdata->pd->users[i];
        }
    }

    return NULL;
}

user * read_has_rights (const dgram *dg, const relaisdata *rdata)
{
    user *usr = get_user_from_dg(dg, rdata);
    if (!usr) {
        return NULL;
    }

    bool found;
    char *tmp, *attr = NULL, data_cpy[DG_DATA_MAX];
    size_t i, attr_len;
    memset(data_cpy, 0, dg->data_len + 1);
    strncpy(data_cpy, dg->data, dg->data_len + 1); // + 1 for the null byte
    attr = strtok_r(data_cpy, ",", &tmp);
    while (attr != NULL) {
        attr_len = strlen(attr) + 1; // with \0
        found = false;
        for (i = 0; i < usr->attributs_len; i ++) {
            attr_len = (strlen(usr->attributs[i]) > strlen(attr)) ? strlen(usr->attributs[i]) : strlen(attr);
            if (strncmp(attr, usr->attributs[i], attr_len) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return NULL;
        }
        attr = strtok_r(NULL, ",", &tmp);
    }

    return usr;
}

bool test_auth (const char *login, const relaisdata *rdata)
{
    size_t i, login_len = strlen(login);
    for (i = 0; i < rdata->pd->nb_hosts; i ++) {
        if (strncmp(login, rdata->pd->hosts[i].login, login_len) == 0)
            return true;
    }
    return false;
}

privdata *init_privdata ()
{
    FILE *fp = fopen("users.txt", "r");
    if (fp == NULL) {
        perror("fopen error");
        return NULL;
    }

    int nb_lines = 0, cpt = 0;
    char c;
    while ((c = fgetc(fp)) != EOF) {
        if ((c == '\n') || (c == '\r')) {
            if (cpt >= 3)
                nb_lines ++;
            cpt = 0;
        } else
            cpt ++;
    }
    if (cpt >= 3)
        nb_lines ++;

    privdata *pd = malloc(sizeof(privdata));
    if (pd == NULL) {
        perror("malloc error");
        return NULL;
    }
    pd->users = malloc(sizeof(user) * nb_lines);
    if (pd->users == NULL) {
        perror("malloc error");
        return NULL;
    }

    pd->nb_users = nb_lines;
    pd->nb_hosts = 0;
    pd->max_hosts = H;
    int i;
    char line[N];
    char *tmp;
    char *str;

    if (fseek(fp, 0, SEEK_SET) == -1) {
        perror("fseek error");
        return NULL;
    }

    for (i = 0; i < nb_lines; i ++) {
        fgets(line, N, fp);
        line[strlen(line) - 1] = '\0';
        str = strtok_r(line, ":", &tmp); // login
        memset(pd->users[i].login, 0, MAX_ATTR);
        strncpy(pd->users[i].login, str, MAX_ATTR);
        str = strtok_r(NULL, ":", &tmp); // psswd
        if (str == NULL)
            return NULL;
        memset(pd->users[i].mdp, 0, MAX_ATTR);
        strncpy(pd->users[i].mdp, str, MAX_ATTR);

        size_t j = 0;
        while ((str = strtok_r(NULL, ":", &tmp)) != NULL) {
            memset(pd->users[i].attributs[j], 0, MAX_ATTR);
            strncpy(pd->users[i].attributs[j], str,  MAX_ATTR);
            j ++;
        }

        pd->users[i].attributs_len = j;
    }

    if (fclose(fp) == EOF) {
        perror("fclose error");
        return NULL;
    }

    // init nodes
    pd->nb_nodes = 0;
    pd->max_nodes = H;
    pd->node_id_counter = 0;
    pd->nodes = malloc(sizeof(node) * H);
    if (pd->nodes == NULL) {
        perror("malloc");
        return NULL;
    }

    pd->nb_responses = 0;
    pd->max_responses = H;
    pd->node_responses = malloc(sizeof(waiting_res) * H);
    if (pd->node_responses == NULL) {
        perror("malloc error");
        return NULL;
    }
    return pd;
}

void update_last_mess_time_from_dg (const dgram *dg, relaisdata *rdata)
{
    bool force_alltests = (dg->request == ACK); // rajouter autres si nécéssaire
    if (force_alltests || ((dg->request >= 1) && (dg->request < 10))) {
        // Client message
        user *usr = get_user_from_dg(dg, rdata);
        if (usr == NULL)
            goto here;
        auth_user *authusr = get_auth_user_from_login(usr->login, rdata);
        if (authusr == NULL)
            goto here;
        authusr->last_mess_time = time(NULL);
    }

here:
    if (force_alltests || ((dg->request >= 20) && (dg->request < 30))) {
        // Node message
        node *nd = get_node_from_dg(dg, rdata);
        if (nd == NULL)
            return;
        nd->last_mess_time = time(NULL);
    } // else => rien a faire
}

void * rthread_check_loop (void *data)
{
    relaisdata *rdata = data;
    unsigned i;
    time_t now;
    while (1) {
        sleep(1);
        now = time(NULL);
        if (sem_wait(&rdata->rsem) == -1) {
            perror("sem_wait");
            return NULL;
        }

        // check for authusers
        for (i = 0; i < rdata->pd->nb_hosts; i ++) {
            if ((now - rdata->pd->hosts[i].last_mess_time) > DISCONNECT_TIMEOUT) {
                // delete the authuser
                printf("SUPPRESSION HOST - TIMEOUT REACHED\n");
                if (i < (rdata->pd->nb_hosts - 1))
                    memmove(&rdata->pd->hosts[i], &rdata->pd->hosts[i + 1], sizeof(auth_user) * (rdata->pd->nb_hosts - i - 1));
                rdata->pd->nb_hosts --;
                i --;

            } else if ((now - rdata->pd->hosts[i].last_mess_time) > PING_TIMEOUT) {
                // send PING mess
                if (dgram_create_send(rdata->sck, NULL, NULL, rdata->id_counter ++, PING, NORMAL, rdata->pd->hosts[i].saddr.sin_addr.s_addr, rdata->pd->hosts[i].saddr.sin_port, 0, NULL) == -1)
                    return NULL;
            }
        }

        // check for nodes
        for (i = 0; i < rdata->pd->nb_nodes; i ++) {
            if ((now - rdata->pd->nodes[i].last_mess_time) > DISCONNECT_TIMEOUT) {
                // delete the node
                printf("SUPPRESSION NODE - TIMEOUT REACHED\n");
                if (i < (rdata->pd->nb_nodes - 1))
                    memmove(&rdata->pd->nodes[i], &rdata->pd->nodes[i + 1], sizeof(node) * (rdata->pd->nb_nodes - i - 1));
                rdata->pd->nb_nodes --;
                i --;

            } else if ((now - rdata->pd->nodes[i].last_mess_time) > PING_TIMEOUT) {
                // send PING mess
                if (dgram_create_send(rdata->sck, NULL, NULL, rdata->id_counter ++, PING, NORMAL, rdata->pd->nodes[i].saddr.sin_addr.s_addr, rdata->pd->nodes[i].saddr.sin_port, 0, NULL) == -1)
                    return NULL;
            }
        }

        waiting_res *wr;
        for (i = 0; i < rdata->pd->nb_responses; i ++) {
            wr = &rdata->pd->node_responses[i];
            if ((now - wr->last_time_rec) > DISCONNECT_TIMEOUT) {
                // delete the waiting_res and send response
                printf("SUPPRESSION & SEND POUR WAITING RES - TIMEOUT REACHED\n");
                // message au client
                if (wr->nb_rec > 0) {
                    // success
                    if (dgram_create_send(rdata->sck, NULL, NULL, rdata->id_counter ++, wr->success_type, SUCCESS, wr->to->saddr.sin_addr.s_addr, wr->to->saddr.sin_port, 0, NULL) == -1)
                        return NULL;
                } else { // error
                    if (dgram_create_send(rdata->sck, NULL, NULL, rdata->id_counter ++, wr->success_type, ERR_NONODE, wr->to->saddr.sin_addr.s_addr, wr->to->saddr.sin_port, 0, NULL) == -1)
                        return NULL;
                }

                // suppression
                if (i < (rdata->pd->nb_responses - 1))
                    memmove(wr, wr + sizeof(waiting_res), sizeof(waiting_res) * (rdata->pd->nb_responses - i - 1));
                rdata->pd->nb_responses --;
                i --;
            }
        }

        if (sem_post(&rdata->rsem) == -1) {
            perror("sem_post");
            return NULL;
        }
    }
}

bool node_send_timeout (const dgram *dg)
{
    relaisdata *rdata = dg->resend_timeout_cb_cparam;
    char *login;
    char *tmp;
    char data_cpy[DG_DATA_MAX];
    memcpy(data_cpy, dg->data, dg->data_len + 1); // + 1 car dg->data contient TOUJOURS un \0 a la fin
    login = strtok_r(data_cpy, ":", &tmp);
    if (!login)
        return true; // le client est surement deco entre temps
    auth_user *host = get_auth_user_from_login(login, rdata);

    if (host) {
        unsigned req;
        switch (dg->request) {
            case RREQ_READ:
                req = RRES_READ;
                break;
            case RREQ_WRITE:
                req = RRES_WRITE;
                break;
            case RREQ_DELETE:
                req = RRES_DELETE;
                break;
            default:
                return true;
        }
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, req, ERR_NOREPLY, host->saddr.sin_addr.s_addr, host->saddr.sin_port, 0, NULL) == -1)
            return false;
    }

    return true; // ne pas quitter (false => quitter)
}
