#include "../headers/noeud.h"

int sck_can_read (const int sck, void *data)
{
    (void) sck;
    nodedata *ndata = ((nodedata *) data);
    if (dgram_process_raw(ndata->sck, &ndata->dgsent, &ndata->dgreceived, ndata, exec_dg) == -1)
        return -1;

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
      /*  case RREQ_GETDATA:
            if (exec_rreq_getdata(dg, ndata) == -1)
                return -1;
            break;*/
        /*case RREQ_SYNC:
            if (exec_rreq_sync(dg, ndata) == -1)
                return -1;
            break;*/
        /*case RREQ_DESTROY:
            if (exec_rreq_destroy(dg, ndata) == -1)
                return -1;
            break;*/
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
    strncpy(data_cpy, dg->data, DG_DATA_MAX);
    username = strtok_r(data_cpy, ":", &tmp);
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

    if (found != -1) {
      strncpy(ndata->datas[found].value, tmp, strlen(tmp));
    }
    else {
      strncpy(ndata->datas[ndata->nb_infos].login, username, strlen(username));
      strncpy(ndata->datas[ndata->nb_infos].value, tmp, strlen(tmp));
      ndata->nb_infos++;
    }

    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_WRITE, SUCCESS, dg->addr, dg->port, strnlen(username, FIELD_MAX), username) == -1)
        return -1;

    return 0;
}

int exec_rreq_delete (const dgram *dg, nodedata *ndata)
{
    const char *username = dg->data;
    size_t i;
    for (i = 0; i < ndata->nb_infos; i++)
    {
      if (strncmp(ndata->datas[i].login, username, strlen(username)) == 0)
      {
        memmove(&ndata->datas[i], &ndata->datas[i + 1], sizeof(user_info) * (ndata->nb_infos - i - 1));
        ndata->nb_infos--;
      }
    }

    char buf[DG_DATA_MAX];
    int val = snprintf(buf, DG_DATA_MAX, "%s", username);
    if (val >= DG_DATA_MAX) {
        fprintf(stderr, "snprintf truncate\n");
        return 1;
    } else if (val < 0) {
        perror("snprintf");
        return -1;
    }

    // envoyer succes au relais
    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_DELETE, SUCCESS, dg->addr, dg->port, strnlen(username, FIELD_MAX), username) == -1)
        return -1;

    return 0;
}

/*int exec_rreq_getdata (const dgram *dg, nodedata *ndata)
{
    // dg->data = <RELAIS_NODE_ID>
    FILE *datafile = fopen(ndata->datafile, "r");
    if (datafile == NULL) {
        perror("fopen");
        return -1;
    }

    char buf[DG_DATA_MAX];
    char *line = NULL;
    size_t n = 0;
    int val;

    // format message a envoyer: <RELAIS_NODE_ID>:<DATA>
    int offset = sprintf(buf, "%s:", dg->data);
    if (offset < 0) {
        perror("sprintf");
        return -1;
    }
    errno = 0;
    while (getline(&line, &n, datafile) != -1) {
        val = snprintf(buf + offset, DG_DATA_MAX - offset, "%s", line);
        if (val < 0) {
            perror("snprintf");
            return -1;
        } else if (val >= DG_DATA_MAX) {
            fprintf(stderr, "snprintf truncate\n");
            return 1;
        }
        offset += val;
    }

    if (errno != 0) {
        perror("getline");
        return -1;
    }

    if (fclose(datafile) == EOF) {
        perror("fclose");
        return -1;
    }

    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_GETDATA, SUCCESS, dg->addr, dg->port, strnlen(buf, DG_DATA_MAX), buf) == -1)
        return -1;

    return 0;
}*/

/*int exec_rreq_sync (const dgram *dg, nodedata *ndata)
{
    FILE *datafile = fopen(ndata->datafile, "w");
    if (datafile == NULL) {
        perror("fopen");
        return -1;
    }

    if (fputs(dg->data, datafile) == EOF) {
        perror("fputs");
        return -1;
    }

    if (fclose(datafile) == EOF) {
        perror("fclose");
        return -1;
    }

    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NRES_SYNC, SUCCESS, dg->addr, dg->port, 0, NULL) == -1)
        return -1;

    return 0;
}*/

int exec_rreq_destroy (const dgram *dg, nodedata *ndata)
{
    (void) dg, (void) ndata;

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

    if (dgram_create_send(ndata->sck, &ndata->dgsent, NULL, ndata->id_counter ++, NREQ_MEET, NORMAL, ndata->relais_saddr.sin_addr.s_addr, ndata->relais_saddr.sin_port, bytes, buf) == -1)
        return -1;

    return 0;
}

bool is_relais (const uint32_t addr, const in_port_t port, const nodedata *ndata)
{
    if ((addr == ndata->relais_saddr.sin_addr.s_addr) && (port == ndata->relais_saddr.sin_port))
        return true;

    return false;
}
