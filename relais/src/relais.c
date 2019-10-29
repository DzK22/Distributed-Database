/**
 * \file relais.c
 * \brief Définitions des fonctions gérant le serveur d'accès
 * \author François et Danyl
 */

#include "../headers/relais.h"

int sck_can_read (const int sck, void *data)
{
    (void) sck;
    relaisdata *rdata = ((relaisdata *) data);
    if (dgram_process_raw(rdata->sck, &rdata->dgsent, &rdata->dgreceived, rdata, exec_dg) == -1)
        return -1;

    return 0;
}

int exec_dg (const dgram *dg, void *data)
{
  relaisdata *rdata = data;
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
        case ACK:
            dgram_del_from_id(&rdata->dgsent, dg->id);
            break;
        default:
            fprintf(stderr, "Paquet reçu avec requete non gérée par le relais: %d\n", dg->request);
    }

    return 0;
}

int exec_creq_auth (const dgram *dg, relaisdata *rdata)
{
    char *tmp;
    printf("data len = %d\n", dg->data_len);
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
    for (i = 0; i < rdata->mugi->nb_users; i ++) {
        flogin = rdata->mugi->users[i].login;
        fpassword = rdata->mugi->users[i].mdp;
        login_ok = strncmp(flogin, login, strlen(flogin)) == 0;
        password_ok = strncmp(fpassword, password, strlen(fpassword)) == 0;

        if (login_ok && password_ok) {
            if (test_auth(login, rdata)) {
                if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_AUTH, ERR_ALREADYAUTH, dg->addr, dg->port, 0, NULL) == -1)
                    return -1;
                return 1;
            }

            // OK
            if (rdata->mugi->nb_hosts == 0) {
                rdata->mugi->hosts = malloc(sizeof(auth_user) * H);
                if (rdata->mugi->hosts == NULL) {
                    perror("malloc");
                    return -1;
                }
            }
            if (rdata->mugi->nb_hosts >= rdata->mugi->max_hosts) {
                rdata->mugi->max_hosts += H;
                rdata->mugi->hosts = realloc(rdata->mugi->hosts, rdata->mugi->max_hosts);
                if (rdata->mugi->hosts == NULL) {
                    perror("realloc");
                    return -1;
                }
            }

            strncpy(rdata->mugi->hosts[rdata->mugi->nb_hosts].login, login, H);
            struct sockaddr_in saddr;
            saddr.sin_family = AF_INET;
            saddr.sin_addr.s_addr = dg->addr;
            saddr.sin_port = dg->port;

            rdata->mugi->hosts[rdata->mugi->nb_hosts].saddr = saddr;
            rdata->mugi->nb_hosts ++;

            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_AUTH, SUC_AUTH, dg->addr, dg->port, 0, NULL) == -1)
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
    size_t cpt = 0, n_fields = 0;

    field = strtok_r(dg->data, ",", &tmp);
    while (field != NULL) {
        n_fields ++;
        for (i = 0; i < rdata->mugi->nb_nodes; i ++) {
            if (strncmp(field, rdata->mugi->nodes[i].field, strlen(field)) != 0)
                continue;
            if (!rdata->mugi->nodes[i].active)
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

            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RREQ_READ, NORMAL, rdata->mugi->nodes[i].saddr.sin_addr.s_addr, rdata->mugi->nodes[i].saddr.sin_port, val, buf) == -1)
                return -1;
            cpt ++;
            break;
        }

        field = strtok_r(NULL, ",", &tmp);
    }

    if (n_fields != cpt) {
        if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_READ, ERR_NONODE, dg->addr, dg->port, 0, NULL) == -1)
            return -1;
        return 1;
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

    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    field_plus_val = strtok_r(data_cpy, ",", &tmp);
    while (field_plus_val != NULL) {
        filled = false;
        for (i = 0; i < rdata->mugi->nb_nodes; i ++) {
            tmp2 = NULL;
            strncpy(field_plus_val_cpy, field_plus_val, MAX_ATTR);
            field = strtok_r(field_plus_val_cpy, ":", &tmp2);
            if (strncmp(field, rdata->mugi->nodes[i].field, strnlen(field, MAX_ATTR)) != 0)
                continue;
            if (!rdata->mugi->nodes[i].active)
                continue;

            // OK
            value = strtok_r(NULL, ":", &tmp2);
            bytes = snprintf(buf, DG_DATA_MAX, "%s:%s", usr->login, value);
            if (bytes >= N) {
                fprintf(stderr, "snprintf truncate\n");
                return 1;
            } else if (bytes < 0) {
                perror("snprintf");
                return -1;
            }

            // send to node
            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RREQ_WRITE, NORMAL, rdata->mugi->nodes[i].saddr.sin_addr.s_addr, rdata->mugi->nodes[i].saddr.sin_port, bytes, buf) == -1)
                return -1;
            filled = true;
        }

        if (!filled) {
            // no node found for some field(s)
            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_WRITE, ERR_NONODE, dg->addr, dg->port, 0, NULL) == -1)
                return -1;
            return 1;
        }
        field_plus_val = strtok_r(NULL, ",", &tmp);
    }
    return 0;
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

    size_t i;
    for (i = 0; i < rdata->mugi->nb_nodes; i ++) {
      if (!rdata->mugi->nodes[i].active)
          continue;

      if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RREQ_DELETE, NORMAL, rdata->mugi->nodes[i].saddr.sin_addr.s_addr, rdata->mugi->nodes[i].saddr.sin_port, strnlen(usr->login, MAX_ATTR), usr->login) == -1)
          return -1;
    }

    return 0;
}

int exec_creq_logout (const dgram *dg, relaisdata *rdata)
{
    (void) dg, (void) rdata;

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
    if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RNRES_MEET, SUC_MEET, dg->addr, dg->port, 0, NULL) == -1)
        return -1;

    if (rdata->mugi->nb_nodes >= rdata->mugi->max_nodes) {
        rdata->mugi->max_nodes += H;
        rdata->mugi->nodes = realloc(rdata->mugi->nodes, sizeof(node) * rdata->mugi->max_nodes);
        if (rdata->mugi->nodes == NULL) {
            perror("realloc");
            return -1;
        }
    }

    // ajouter le noeud
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = dg->addr;
    saddr.sin_port = dg->port;
    strncpy(rdata->mugi->nodes[rdata->mugi->nb_nodes].field, dg->data, MAX_ATTR);
    rdata->mugi->nodes[rdata->mugi->nb_nodes].saddr = saddr;
    rdata->mugi->nodes[rdata->mugi->nb_nodes].id = rdata->mugi->node_id_counter;
    rdata->mugi->nodes[rdata->mugi->nb_nodes].active = true;
    rdata->mugi->nb_nodes ++;
    rdata->mugi->node_id_counter ++;

    // si un noeud du meme type existe deja, ecraser les données du nouveau noeud par les siennes (surement plus à jour) envoie message a ce noeud pour lui demander toutes ses donnees
    size_t i;
    for (i = 0; i < (rdata->mugi->nb_nodes - 1); i ++) {
        if (strncmp(dg->data, rdata->mugi->nodes[i].field, MAX_ATTR) == 0) {
            // bloquer temporairement le noeud car n'a pas les données a jour
            rdata->mugi->nodes[rdata->mugi->nb_nodes - 1].active = false;
            char msg[N];
            if (sprintf(msg, "getall %lu;", rdata->mugi->node_id_counter - 1) < 0) {
                fprintf(stderr, "sprintf error\n");
                return -1;
            }

            // SEND MSG GETDATA TO THIS NODE
            if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RREQ_GETDATA, NORMAL, dg->addr, dg->port, 0, NULL) == -1)
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
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    username = strtok_r(data_cpy, ":", &tmp);
    if (username == NULL) {
        fprintf(stderr, "exec_nres_read: username == NULL\n");
        return 1;
    }

    node *nd = get_node_field_from_dg(dg, rdata);
    if (nd == NULL)
        return 1;

    char buf[DG_DATA_MAX];
    int val = snprintf(buf, DG_DATA_MAX, "%s=%s", nd->field, dg->data + strnlen(username, N) + 1);
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

    if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_READ, SUC_READ, authusr->saddr.sin_addr.s_addr, authusr->saddr.sin_port, strnlen(buf, DG_DATA_MAX), buf) == -1)
        return -1;

    return 0;
}

int exec_nres_write (const dgram *dg, relaisdata *rdata)
{
    // /!/ dg->data = <user>
    char *username, *tmp, data_cpy[DG_DATA_MAX];
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    username = strtok_r(data_cpy, ":", &tmp);
    if (!username) {
        fprintf(stderr, "strtok_r error ?\n");
        return 1;
    }

    // envoyer success au client
    auth_user *authusr = get_auth_user_from_login(username, rdata);
    if (authusr == NULL) // le client a été déconnecté entre temps
        return 1;

    if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_WRITE, SUC_WRITE, authusr->saddr.sin_addr.s_addr, authusr->saddr.sin_port, 0, NULL) == -1)
        return -1;

    return 0;
}

int exec_nres_delete (const dgram *dg, relaisdata *rdata)
{
    // /!/ dg->data = <user>
    char *username = dg->data;

    // envoyer success au client
    auth_user *authusr = get_auth_user_from_login(username, rdata);
    if (authusr == NULL) // le client s'est déconnecté entre temps
        return 1;

    if (dgram_create_send(rdata->sck, &rdata->dgsent, NULL, rdata->id_counter ++, RRES_DELETE, SUC_DELETE, authusr->saddr.sin_addr.s_addr, authusr->saddr.sin_port, 0, NULL) == -1)
        return -1;

    return 0;
}

int exec_nres_getdata (const dgram *dg, relaisdata *rdata)
{
    (void) dg, (void) rdata;

    return 0;
}

int exec_nres_sync (const dgram *dg, relaisdata *rdata)
{
    (void) dg, (void) rdata;

    return 0;
}

auth_user * get_auth_user_from_login (const char *login, const relaisdata *rdata)
{
    size_t i, login_len = strlen(login);
    for (i = 0; i < rdata->mugi->nb_hosts; i ++) {
       if (strncmp(login, rdata->mugi->hosts[i].login, login_len) == 0)
           return &rdata->mugi->hosts[i];
    }

    return NULL;
}

node * get_node_field_from_dg (const dgram *dg, const relaisdata *rdata)
{
  size_t i;
  bool ip_ok, port_ok;
  for (i = 0; i < rdata->mugi->nb_nodes; i ++)
  {
    ip_ok = dg->addr == rdata->mugi->nodes[i].saddr.sin_addr.s_addr;
    if (!ip_ok)
        continue;
    port_ok = dg->port == rdata->mugi->nodes[i].saddr.sin_port;
    if (!port_ok)
        continue;

    return &rdata->mugi->nodes[i];
  }
  return NULL;
}

user * get_user_from_dg (const dgram *dg, const relaisdata *rdata)
{
    // verifie que le user qui correspond a la requete est bien connecté sinon NULL
    size_t i;
    bool ip_ok, port_ok;
    for (i = 0; i < rdata->mugi->nb_hosts; i ++) {
        ip_ok = dg->addr == rdata->mugi->hosts[i].saddr.sin_addr.s_addr;
        if (!ip_ok)
            continue;
        port_ok = dg->port == rdata->mugi->hosts[i].saddr.sin_port;
        if (!port_ok)
            continue;
        // OK
        const char *login = rdata->mugi->hosts[i].login;
        const size_t login_len = strlen(login);
        // search for the corresponding user in the users array
        for (i = 0; i < rdata->mugi->nb_users; i ++) {
            if (strncmp(login, rdata->mugi->users[i].login, login_len) == 0)
                return &rdata->mugi->users[i];
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
            fprintf(stderr, "not found for attr = |%s|\n", attr);
            return NULL;
        }
        attr = strtok_r(NULL, ",", &tmp);
    }

    return usr;
}

bool test_auth (const char *login, const relaisdata *rdata)
{
    size_t i, login_len = strlen(login);
    for (i = 0; i < rdata->mugi->nb_hosts; i ++) {
        if (strncmp(login, rdata->mugi->hosts[i].login, login_len) == 0)
            return true;
    }
    return false;
}

mugiwara *init_mugiwara ()
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

    mugiwara *mugi = malloc(sizeof(mugiwara));
    if (mugi == NULL) {
        perror("malloc error");
        return NULL;
    }
    mugi->users = malloc(sizeof(user) * nb_lines);
    if (mugi->users == NULL)
    {
        perror("malloc error");
        return NULL;
    }
    mugi->nb_users = nb_lines;
    mugi->nb_hosts = 0;
    mugi->max_hosts = H;
    int i;
    char line[N];
    char *tmp;
    char *str;
    if (fseek(fp, 0, SEEK_SET) == -1)
    {
      perror("fseek error");
      return NULL;
    }

    for (i = 0; i < nb_lines; i ++) {
        fgets(line, N, fp);
        line[strlen(line) - 1] = '\0';
        str = strtok_r(line, ":", &tmp); // login
        strncpy(mugi->users[i].login, str, MAX_ATTR);
        str = strtok_r(NULL, ":", &tmp); // psswd
        strncpy(mugi->users[i].mdp, str, MAX_ATTR);

        size_t j = 0;
        while ((str = strtok_r(NULL, ":", &tmp)) != NULL) {
            strncpy(mugi->users[i].attributs[j], str,  MAX_ATTR);
            j ++;
        }

        mugi->users[i].attributs_len = j;
    }

    if (fclose(fp) == EOF) {
        perror("fclose error");
        return NULL;
    }

    // init nodes
    mugi->nb_nodes = 0;
    mugi->max_nodes = H;
    mugi->node_id_counter = 0;
    mugi->nodes = malloc(sizeof(node) * H);
    if (mugi->nodes == NULL) {
        perror("malloc");
        return NULL;
    }

    return mugi;
}

/*

int follow_getallres (const int sock, clientreq *creq, mugiwara *mugi)
{
    // message de creq = id_node:data;
    size_t to_id;
    char data[N];
    switch (sscanf(creq->message, "%lu:%s;", &to_id, data)) {
        case EOF:
            perror("sscanf");
            return -1;
        case 2: // no problem
            break;
        default: // problem, mais ne pas quitter le prog => 0
            fprintf(stderr, "follow_getallres: sscanf, cannot assign all items\n");
            return 1;
    }

    // envoyer les nouvelles données au noeud qui a l'id "to_id"
    char msg[N];
    int res;
    res = snprintf(msg, N, "changeall %s;", data);
    if (res >= N) {
        fprintf(stderr, "sprintf error\n");
        return -1;
    } else if (res < 0) {
        perror("snprintf");
        return -1;
    }

    size_t i;
    for (i = 0; i < mugi->nb_nodes; i ++) {
        if (mugi->nodes[i].id == to_id) {
            if (send_toclient(sock, msg, &mugi->nodes[i].saddr) == -1)
                return -1;
            break;
        }
    }

    return 0;
}
*/
