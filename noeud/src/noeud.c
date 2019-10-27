#include "../headers/noeud.h"

int fd_can_read (int fd, void *data)
{
    nodedata *ndata = ((nodedata *) data);
    if (fd == ndata->sck) {
        if (read_sck(ndata) == -1)
            return -1;
    }

    return 0;
}

int read_sck (nodedata *ndata)
{
    void *buf = malloc(SCK_DATAGRAM_MAX);
    if (buf == NULL) {
        perror("malloc");
        return -1;
    }
    struct sockaddr_in saddr;
    ssize_t bytes = sck_recv(ndata->sck, buf, SCK_DATAGRAM_MAX, &saddr);
    if (bytes == -1)
        return -1;

    if (!is_relais(&saddr, ndata)) {
        fprintf(stderr, "Un paquet ne provenant pas du relais a été rejeté\n");
        return 1;
    }

    dgram dg;
    if (dgram_add_from_raw(&ndata->dgreceived, buf, bytes, &dg, &saddr) == -1)
        return -1;
    free(buf);
    dgram_debug(&dg);

    if (dgram_is_ready(&dg)) {
        printf("is rdy\n");
        // SEND ACK
        if (dg.request != ACK) {
            dgram *ack = malloc(sizeof(dgram));
            if (ack == NULL) {
                perror("malloc");
                return -1;
            }
            if (dgram_create(ack, dg.id, ACK, NORMAL, dg.addr, dg.port, 0, NULL) == -1)
                return -1;
            if (dgram_send(ndata->sck, ack, &ndata->dgsent) == -1)
                return -1;
        }

        if (exec_dg(&dg, ndata) == -1)
            return -1;
        // supprimer ce dg
        if (!dgram_del_from_id(&ndata->dgreceived, dg.id))
            return 1;
    }

    return 0;
}

int exec_dg (const dgram *dg, nodedata *ndata)
{
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
        case RREQ_DESTROY:
            if (exec_rreq_destroy(dg, ndata) == -1)
                return -1;
            break;
        case ACK:
            dgram_del_from_id(&ndata->dgsent, dg->id);
            break;
        default:
            fprintf(stderr, "Paquet reçu avec requete non gérée par le noeud: %d\n", dg->request);
    }

    return 0;
}

int exec_rnres_meet (const dgram *dg, nodedata *ndata)
{
    if (dg->status == SUC_MEET)
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
        dgram *newdg = malloc(sizeof(dgram));
        if (newdg == NULL) {
            perror("malloc");
            return -1;
        }
        if (dgram_create(newdg, ndata->id_counter ++, NRES_READ, ERR_UNKNOWFIELD , dg->addr, dg->port, 0, NULL) == -1)
            return -1;
        if (dgram_send(ndata->sck, newdg, &ndata->dgsent) == -1)
            return -1;

        return 1;
    }

    int tmpfd;
    if ((tmpfd = open(ndata->datafile, O_RDONLY | O_CREAT, 0644)) == -1) {
        perror("open");
        return -1;
    }
    if (close(tmpfd) == -1) {
        perror("close");
        return -1;
    }

    FILE *datafile = fopen(ndata->datafile, "r");
    if (datafile == NULL) {
        perror("fopen");
        return -1;
    }

    ssize_t bytes;
    char *line = malloc(DATAFILE_LINE_MAX);
    if (line == NULL) {
        perror("malloc");
        return -1;
    }
    char data[DG_DATA_MAX] = "";
    size_t len;
    size_t n = DATAFILE_LINE_MAX;

    errno = 0;
    while ((bytes = getline(&line, &n, datafile)) != -1) {
        len = strnlen(data, DG_DATA_MAX);
        if (line[bytes - 1] == '\n')
            line[bytes - 1] = '\0';
        strncat(data, line, DG_DATA_MAX - len -1);
        if (len >= DG_DATA_MAX)
            break;
        strncat(data, ",", DG_DATA_MAX - len);
    }
    if (errno != 0) {
        perror("getline");
        return -1;
    }

    data[strnlen(data, DG_DATA_MAX) - 1] = '\0';
    free(line);
    if (fclose(datafile) == EOF) {
        perror("fclose");
        return -1;
    }

    char buf[DG_DATA_MAX];
    int val = snprintf(buf, DG_DATA_MAX, "%s:%s", username, data);
    if (val >= DG_DATA_MAX) {
        fprintf(stderr, "snprintf truncate\n");
        return 1;
    } else if (val < 0) {
        perror("snprintf");
        return -1;
    }

    dgram *newdg = malloc(sizeof(dgram));
    if (newdg == NULL) {
        perror("malloc");
        return -1;
    }
    if (dgram_create(newdg, ndata->id_counter ++, NRES_READ, SUC_READ, dg->addr, dg->port, val, buf) == -1)
        return -1;
    if (dgram_send(ndata->sck, newdg, &ndata->dgsent) == -1)
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

    int tmpfd = open(ndata->datafile, O_RDONLY | O_CREAT, 0644);
    if (tmpfd == -1) {
        perror("open");
        return -1;
    }
    if (close(tmpfd) == -1) {
        perror("close");
        return -1;
    }

    // si elle existe, supprimer la ligne contenant le user et l'ancienne valeur qui lui est associée
    if (delete_user_file_line(username, ndata) == -1)
        return -1;

    // si le dernier caractere est un \n, le supprimer ! sinon doublon de \n et ligne vide inutile

    // ajouter la nouvelle ligne
    FILE *datafile = fopen(ndata->datafile, "a+");
    if (datafile == NULL) {
        perror("fopen");
        return -1;
    }

    errno = 0;
    int c;
    if (((c = fgetc(datafile)) == EOF) && (errno != 0)) {
        perror("fgetc");
        return -1;
    }

    if (c != EOF) {
        if (fseek(datafile, -1, SEEK_END) == -1) {
            perror("fseek");
            return -1;
        }

        // lire le dernier caractere du fichier
        switch (fgetc(datafile)) {
            case -1:
                perror("fgetc");
                return -1;
            case '\n':
                break;
            case '\r':
                break;
            default:
                // add new line
                if (fputc('\n', datafile) == EOF) {
                    perror("fputc");
                    return -1;
                }
                break;
        }
    }

    // ajout de la nouvelle ligne
    if (fputs(dg->data, datafile) == EOF) {
        perror("fputs");
        return -1;
    }

    if (fclose(datafile) == EOF) {
        perror("fclose");
        return -1;
    }

    dgram *newdg = malloc(sizeof(dgram));
    if (newdg == NULL) {
        perror("malloc");
        return -1;
    }
    if (dgram_create(newdg, ndata->id_counter ++, NRES_WRITE, SUC_WRITE, dg->addr, dg->port, strnlen(username, FIELD_MAX), username) == -1)
        return -1;
    if (dgram_send(ndata->sck, newdg, &ndata->dgsent) == -1)
        return -1;

    return 0;
}

int exec_rreq_delete (const dgram *dg, nodedata *ndata)
{
    const char *username = dg->data;
    int tmpfd = open(ndata->datafile, O_RDONLY | O_CREAT, 0644);
    if (tmpfd == -1) {
        perror("open");
        return -1;
    }
    if (close(tmpfd) == -1) {
        perror("close");
        return -1;
    }

    // si elle existe, supprimer la ligne contenant le user et l'ancienne valeur qui lui est associée
    if (delete_user_file_line(username, ndata) == -1)
        return -1;

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
    dgram *newdg = malloc(sizeof(dgram));
    if (newdg == NULL) {
        perror("malloc");
        return -1;
    }
    if (dgram_create(newdg, ndata->id_counter ++, NRES_DELETE, SUC_DELETE, dg->addr, dg->port, strnlen(username, FIELD_MAX), username) == -1)
        return -1;
    if (dgram_send(ndata->sck, newdg, &ndata->dgsent) == -1)
        return -1;

    return 0;
}

int exec_rreq_getdata (const dgram *dg, nodedata *ndata)
{
    (void) dg, (void) ndata;

    return 0;
}

int exec_rreq_sync (const dgram *dg, nodedata *ndata)
{
    (void) dg, (void) ndata;

    return 0;
}

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

    dgram *dg = malloc(sizeof(dgram));
    if (dg == NULL) {
        perror("malloc");
        return -1;
    }
    if (dgram_create(dg, ndata->id_counter ++, NREQ_MEET, NORMAL, ndata->relais_saddr.sin_addr.s_addr, ndata->relais_saddr.sin_port, bytes, buf) == -1)
        return -1;
    if (dgram_send(ndata->sck, dg, &ndata->dgsent) == -1)
        return -1;

    return 0;
}

bool is_relais (const struct sockaddr_in *saddr, const nodedata *ndata)
{
    if ((saddr->sin_addr.s_addr == ndata->relais_saddr.sin_addr.s_addr) && (saddr->sin_port == ndata->relais_saddr.sin_port))
        return true;

    return false;
}

int delete_user_file_line (const char *username, const nodedata *ndata)
{
    char tmp_w_filename[N];
    sprintf(tmp_w_filename, "%s_tmp", ndata->datafile);

    FILE *file_r = fopen(ndata->datafile, "r");
    if (file_r == NULL) {
        perror("fopen");
        return -1;
    }

    FILE *file_w = fopen(tmp_w_filename, "w");
    if (file_w == NULL) {
        perror("fopen");
        return -1;
    }

    char *line = malloc(DATAFILE_LINE_MAX);
    if (line == NULL) {
        perror("malloc");
        return -1;
    }

    char *usr, *tmp, line_cpy[DATAFILE_LINE_MAX];
    size_t username_len = strnlen(username, FIELD_MAX), n = DATAFILE_LINE_MAX;
    ssize_t bytes;
    errno = 0;
    while ((bytes = getline(&line, &n, file_r)) != -1) {
        tmp = NULL;
        strncpy(line_cpy, line, n);
        usr = strtok_r(line_cpy, ":", &tmp);
        if (usr == NULL) {
            perror("strtok_r");
            return -1;
        }
        if (strncmp(usr, username, username_len + 1) == 0) {
            continue;
        }

        // add this line
        if (fputs(line, file_w) == EOF) {
            perror("fputs");
            return -1;
        }
    }

    if (errno != 0) {
        perror("getline");
        return -1;
    }

    free(line);
    if (fclose(file_r) == EOF) {
        perror("fclose");
        return -1;
    }

    if (fclose(file_w) == EOF) {
        perror("fclose");
        return -1;
    }

    if (rename(tmp_w_filename, ndata->datafile) == -1) {
        perror("rename");
        return -1;
    }

    return 0;
}

/*
int change_all_data (const node_data *ndata, const char *new_data) // args must be null terminated
{
    FILE *datafile = fopen(ndata->datafile, "w");
    if (datafile == NULL) {
        perror("fopen");
        return -1;
    }

    if (fputs(new_data, datafile) == EOF) {
        perror("fputs");
        return -1;
    }

    if (fclose(datafile) == EOF) {
        perror("fclose");
        return -1;
    }

    return 0;
}

    return 0;
}
*/
