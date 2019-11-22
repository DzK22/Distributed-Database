// Auteurs: Danyl El-Kabir et François Grabenstaetter

#include "../headers/noeud.h"

int sck_can_read (const int sck, void *data)
{
    (void) sck;
    nodedata *ndata = ((nodedata *) data);
    if (sem_wait(&ndata->gsem) == -1) {
        perror("sem_wait");
        return -1;
    }

    if (dgram_process_raw(ndata->sck, &ndata->dgsent, &ndata->dgreceived, ndata, exec_dg) == -1)
        return -1;

    if (sem_post(&ndata->gsem) == -1) {
        perror("sem_post");
        return -1;
    }

    return 0;
}

int exec_dg (const dgram *dg, void *data)
{
    nodedata *ndata = data;

    // petite sécurité des familles
    if (!is_relais(dg->addr, dg->port, ndata)) {
        if (!dgram_del_from_id(&ndata->dgreceived, dg->id))
            return -1;
        return 1;
    }

    switch (dg->request) {
        case RNRES_MEET:
            if (exec_rnres_meet(dg, ndata) == -1)
                return -1;
            break;
        case RREQ_READ:
            if (exec_rreq_read(dg, ndata) == -1)
                return -1;
            break;
        case RREQ_WRITE:
            if (exec_rreq_write(dg, ndata) == -1)
                return -1;
            break;
        case RREQ_DELETE:
            if (exec_rreq_delete(dg, ndata) == -1)
                return -1;
            break;
        case RREQ_GETDATA:
            if (exec_rreq_getdata(dg, ndata) == -1)
                return -1;
            break;
        case RREQ_SYNC:
            if (exec_rreq_sync(dg, ndata) == -1)
                return -1;
            break;
        case PING:
            break;
        case ACK:
            break;
        default:
            fprintf(stderr, "Paquet reçu avec requete non gérée par le noeud: %d\n", dg->request);
    }

    return 0;
}

int exec_rnres_meet (const dgram *dg, nodedata *ndata)
{
    if (dg->status == SUCCESS)
        ndata->meet_success = true;
    else
        return -1;

    return 0;
}

int exec_rreq_read (const dgram *dg, nodedata *ndata)
{
    // args ne peut être que un seul champ ! (c'est au relais de split les champs vers les divers noeuds)
    // /!/ args = <username>:<field>;

    char *username, *field, *tmp;
    username = strtok_r(dg->data, ":", &tmp);
    if (username == NULL) {
        perror("strtok_r");
        return -1;
    }
    field = strtok_r(NULL, ":", &tmp);
    if (field == NULL) {
        perror("strtok_r");
        return -1;
    }

    // verifions que ce noeud possède bien le champs
    if (strncmp(field, ndata->field, strlen(field) + 1) != 0) {
        // envoyer err
        if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_READ, ERR_UNKNOWFIELD , dg->addr, dg->port, 0, NULL) == -1)
            return -1;

        return 1;
    }

    char data[DG_DATA_MAX] = "";
    size_t len;
    size_t i;
    for (i = 0; i < ndata->nb_infos; i++)
    {
        len = strnlen(data, DG_DATA_MAX);
        strncat(data, ndata->datas[i].login, DG_DATA_MAX - len -1);
        strcat(data, ":");
        strncat(data, ndata->datas[i].value, DG_DATA_MAX - len -1);
        if (len >= DG_DATA_MAX)
            break;
        strncat(data, ",", DG_DATA_MAX - len);
    }

    data[strnlen(data, DG_DATA_MAX) - 1] = '\0';
    char buf[DG_DATA_MAX];
    int val = snprintf(buf, DG_DATA_MAX, "%s:%s", username, data);
    if (val >= DG_DATA_MAX) {
        fprintf(stderr, "snprintf truncate\n");
        return 1;
    } else if (val < 0) {
        perror("snprintf");
        return -1;
    }

    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_READ, SUCCESS, dg->addr, dg->port, val, buf) == -1)
        return -1;

    return 0;
}

int exec_rreq_write (const dgram *dg, nodedata *ndata)
{
    // ne peut être que un seul champ ! (c'est au relais de split les champs vers les divers noeuds)
    // dg->data = <username>:<field_value>

    char *username, *tmp, data_cpy[DG_DATA_MAX];
    memset(data_cpy, 0, DG_DATA_MAX);
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    char *id = strtok_r(data_cpy, ":", &tmp);                  //pour enlever l'id;
    username = strtok_r(NULL, ":", &tmp);
    if (username == NULL) {
        fprintf(stderr, "strtok_r error ?\n");
        return 1;
    }

    size_t i; int found = -1;
    for (i = 0; i < ndata->nb_infos; i++)
    {
        if (strncmp(username, ndata->datas[i].login, strlen(username)) == 0)
        {
            found = i;
            break;
        }
    }

    if (found != -1)
    {
      memset(ndata->datas[ndata->nb_infos].value, 0, FIELD_MAX);
      strncpy(ndata->datas[found].value, tmp, FIELD_MAX);
    }
    else {
        memset(ndata->datas[ndata->nb_infos].login, 0, FIELD_MAX);
        strncpy(ndata->datas[ndata->nb_infos].login, username, FIELD_MAX);

        memset(ndata->datas[ndata->nb_infos].value, 0, FIELD_MAX);
        strncpy(ndata->datas[ndata->nb_infos].value, tmp, FIELD_MAX);
        ndata->nb_infos++;
    }

    int identifiant = atoi(id);
    char buf[DG_DATA_MAX];
    int ret = snprintf(buf, DG_DATA_MAX, "%d", identifiant);
    if (ret >= DG_DATA_MAX || ret < 0) {
        fprintf(stderr, "snprintf error\n");
        return -1;
    }
    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_WRITE, SUCCESS, dg->addr, dg->port, strnlen(buf, FIELD_MAX), buf) == -1)
        return -1;

    return 0;
}

int exec_rreq_delete (const dgram *dg, nodedata *ndata)
{
    char *username, *tmp, data_cpy[DG_DATA_MAX];
    memset(data_cpy, 0, DG_DATA_MAX);
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    char *id = strtok_r(data_cpy, ":", &tmp);                  //pour enlever l'id;
    username = strtok_r(NULL, ":", &tmp);
    if (username == NULL) {
        fprintf(stderr, "strtok_r error ?\n");
        return 1;
    }
    size_t i;
    for (i = 0; i < ndata->nb_infos; i++)
    {
        if (strncmp(ndata->datas[i].login, username, strlen(username)) == 0)
        {
            memmove(&ndata->datas[i], &ndata->datas[i + 1], sizeof(user_info) * (ndata->nb_infos - i - 1));
            ndata->nb_infos--;
        }
    }

    int val = atoi(id);
    char buf[DG_DATA_MAX];
    int ret = snprintf(buf, DG_DATA_MAX, "%d", val);
    if (ret >= DG_DATA_MAX || ret < 0) {
        fprintf(stderr, "snprintf error\n");
        return -1;
    }
    // envoyer succes au relais
    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_DELETE, SUCCESS, dg->addr, dg->port, strnlen(buf, FIELD_MAX), buf) == -1)
        return -1;

    return 0;
}

int exec_rreq_getdata (const dgram *dg, nodedata *ndata)
{
    // dg->data = <RELAIS_NODE_ID>
    char buf[DG_DATA_MAX];

    // format message a envoyer: <RELAIS_NODE_ID>:<DATA>
    sprintf(buf, "%s:", dg->data);

    size_t i;
    for (i = 0; i < ndata->nb_infos; i++) {
        strcat(buf, ndata->datas[i].login);
        strcat(buf, ",");
        strcat(buf, ndata->datas[i].value);
        strcat(buf, ";");
    }
    printf("buf = %s\n", buf);
    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_GETDATA, SUCCESS, dg->addr, dg->port, strnlen(buf, DG_DATA_MAX), buf) == -1)
        return -1;

    return 0;
}

int exec_rreq_sync (const dgram *dg, nodedata *ndata)
{
    char *tmp, *tmp2;
    char *rest = dg->data;
    while ((tmp = (strtok_r(rest, ";", &rest))) != NULL)
    {
        tmp2 = strtok_r(tmp, ",", &tmp);
        memset(ndata->datas[ndata->nb_infos].login, 0, FIELD_MAX);
        strncpy(ndata->datas[ndata->nb_infos].login, tmp2, FIELD_MAX);

        memset(ndata->datas[ndata->nb_infos].value, 0, FIELD_MAX);
        strncpy(ndata->datas[ndata->nb_infos].value, tmp, FIELD_MAX);
        ndata->nb_infos++;
    }

    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_SYNC, SUCCESS, dg->addr, dg->port, 0, NULL) == -1)
        return -1;

    return 0;
}

int send_meet (nodedata *ndata)
{
    char buf[DG_DATA_MAX];
    int bytes = snprintf(buf, DG_DATA_MAX, "%s", ndata->field);
    if (bytes >= DG_DATA_MAX) {
        fprintf(stderr, "snprintf truncate\n");
        return 1;
    } else if (bytes < 0) {
        fprintf(stderr, "snprintf error\n");
        return -1;
    }

    dgram *new_dg;
    if (dgram_create_send(ndata->sck, &ndata->dgsent, &new_dg, ndata->id_counter ++, NREQ_MEET, NORMAL, ndata->relais_saddr.sin_addr.s_addr, ndata->relais_saddr.sin_port, bytes, buf) == -1)
        return -1;

    new_dg->resend_timeout_cb = meet_timeout;

    return 0;
}

bool is_relais (const uint32_t addr, const in_port_t port, const nodedata *ndata)
{
    if ((addr == ndata->relais_saddr.sin_addr.s_addr) && (port == ndata->relais_saddr.sin_port))
        return true;

    return false;
}

bool meet_timeout (const dgram *dg)
{
    (void) dg;
    dgram tmpdg;
    tmpdg.status = ERR_NOREPLY;
    dgram_print_status(&tmpdg);
    return false;
}
