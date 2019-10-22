#include "../headers/noeud.h"

int create_socket ()
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        perror("socket error");
        return -1;
    }

    return sock;
}

int fill_node_data (int sock, const char *field, const char *datafile, const char *relais_ip, const char *relais_port, node_data *ndata)
{
    struct sockaddr_in saddr_relais;
    saddr_relais.sin_family = AF_INET;
    saddr_relais.sin_port = htons(atoi(relais_port));
    if (inet_pton(AF_INET, relais_ip, &saddr_relais.sin_addr) == -1) {
        perror("inet_pton");
        return -1;
    }

    ndata->sock = sock;
    ndata->field = field;
    ndata->datafile = datafile;
    ndata->saddr_relais = saddr_relais;
    return 0;
}

int can_bind (const int sock)
{
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(0);
    saddr.sin_addr.s_addr = INADDR_ANY;
    const socklen_t saddr_len = sizeof(struct sockaddr_in);

    if (bind(sock, (struct sockaddr *) &saddr, saddr_len) == -1) {
        perror("bind error");
        return -1;
    }

    return 0;
}

int remember_relais_addr (const node_data *ndata)
{
    const socklen_t saddr_len = sizeof(struct sockaddr_in);
    if (connect(ndata->sock, (struct sockaddr *) &ndata->saddr_relais, saddr_len) == -1) {
        perror("connect");
        return 1;
    }

    return 0;
}

int send_to_relais (const int sock, const char *buf, const size_t buf_len)
{
    ssize_t bytes;
    size_t total = 0;
    printf("aller on send\n");
    while ((bytes = send(sock, buf + total, buf_len - total, 0)) > 0)
    {
      total += bytes;
      printf("send = %s\n", buf);
    }
    if (bytes == -1) {
        perror("send error");
        return -1;
    }

    return 0;
}

ssize_t recv_from_relais (const int sock, char *buf, const size_t buf_max, struct sockaddr_in *sender_saddr)
{
    socklen_t sender_saddr_len = sizeof(struct sockaddr_in);
    ssize_t bytes;
    if ((bytes = recvfrom(sock, buf, buf_max , 0, (struct sockaddr *) sender_saddr, &sender_saddr_len)) == -1)
    {
      perror("recvfrom");
      return -1;
    }
    return bytes;
}

int meet_relais (const node_data *ndata)
{
    char str[MESS_MAX];
    int tmp = snprintf(str, MESS_MAX, "meet %s;", ndata->field);
    if ((tmp >= MESS_MAX) || (tmp < 0)) {
        fprintf(stderr, "sprintf error\n");
        return -1;
    }

    if (send_to_relais(ndata->sock, str, strlen(str)) == -1)
        return -1;

    ssize_t bytes;
    if ((bytes = recv(ndata->sock, str, MESS_MAX - 1, 0)) == -1)
        return -1;
    str[bytes] = '\0';
    if (strncmp(str, "success;", 9) != 0) {
        printf("MEET_RELAIS FAILED: WRONG RESPONSE MESSAGE FROM RELAIS: %s\n", str);
        return -1;
    }

    printf("MEET SUCCESS\n");
    return 0;
}

void wait_for_request (const node_data *ndata)
{
    char buf[MESS_MAX];
    ssize_t bytes;
    struct sockaddr_in sender_saddr;
    relaisreq rreq;

    while ((bytes = recv_from_relais(ndata->sock, buf, MESS_MAX - 1, &sender_saddr)) > 0) {
        printf("ndata->sock = %s\n", buf);
        buf[bytes] = '\0';
        switch (parse_datagram(buf, &rreq)) {
            case -1:
                return;
            case 1:
                // skip
                break;
        }

        if (exec_relais_request(ndata, &rreq) == -1)
            return;
    }
    // if here, there is a problem
}

int parse_datagram (const char *buf, relaisreq *rreq) // buf must be null terminated
{
    printf("Nouvelle requete recue: %s\n", buf);
    char tmp[MESS_MAX];
    errno = 0;
    if (sscanf(buf, "%s %s;", tmp, rreq->args) != 2) {
        if (errno != 0) {
            perror("sscanf");
            return -1;
        }
        // else
        fprintf(stderr, "parse_datagram: cannot assign all items: %s\n", buf);
        return 1;
    }
    // degeullasse a virer quand trouver le bug du ; a fin de args
    rreq->args[strlen(rreq->args) - 1] = '\0';

    if (strncmp(tmp, "read", 5) == 0)
        rreq->type = Read;
    else if (strncmp(tmp, "write", 7) == 0)
        rreq->type = Write;
    else if (strncmp(tmp, "delete", 7) == 0)
        rreq->type = Delete;
    else if (strncmp(tmp, "getall", 7) == 0)
        rreq->type = Getall;
    else if (strncmp(tmp, "changeall", 10) == 0)
        rreq->type = Changeall;
    else {
        fprintf(stderr, "parse_datagram: wrong message type\n");
        return -1;
    }

    return 0;
}

int exec_relais_request (const node_data *ndata, relaisreq *rreq)
{
    switch (rreq->type) {
        case Read:
            if (node_read(ndata, rreq->args) == -1)
                return -1;
            break;
        case Write:
            if (node_write(ndata, rreq->args) == -1)
                return -1;
            break;
        case Delete:
            if (node_delete(ndata, rreq->args) == -1)
                return -1;
            break;
        case Changeall:
            if (change_all_data(ndata, rreq->args) == -1)
                return -1;
            break;
        case Getall:
            // if get_all_data ...
            break;
        default:
            return -1;
    }

    return 0;
}

int node_read (const node_data *ndata, const char *args) // args must be null terminated
{
    // args ne peut être que un seul champ ! (c'est au relais de split les champs vers les divers noeuds)
    // /!/ args = <username>:<field>;

    char *username, *field, *tmp;
    username = strtok_r((char *) args, ":", &tmp);
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
        send_to_relais(ndata->sock, "3", 1);
        printf("ERROR 3\n");
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
    char data[MESS_MAX] = "";
    size_t len;
    size_t n = DATAFILE_LINE_MAX;

    errno = 0;
    while ((bytes = getline(&line, &n, datafile)) != -1) {
        len = strlen(data);
        if (line[bytes - 1] == '\n')
            line[bytes - 1] = '\0';
        strncat(data, line, MESS_MAX - len -1);
        if (len >= MESS_MAX)
            break;
        strcat(data, ",");
    }
    if (errno != 0) {
        perror("getline");
        return -1;
    }

    data[strlen(data) - 1] = '\0';
    free(line);
    if (fclose(datafile) == EOF) {
        perror("fclose");
        return -1;
    }

    char msg[MESS_MAX];
    int val;
    val = snprintf(msg, MESS_MAX, "readres %s:%s;", username, data);
    if (val >= MESS_MAX) {
        fprintf(stderr, "snprintf truncate\n");
        return 1;
    } else if (val < 0) {
        perror("snprintf");
        return -1;
    }

    if (send_to_relais(ndata->sock, msg, val) == -1)
        return -1;

    return 0;
}

int node_write (const node_data *ndata, const char *args) // args must be null terminated
{
    // args ne peut être que un seul champ ! (c'est au relais de split les champs vers les divers noeuds)
    // /!/ args = <username>:<field_value>;

    char *username, *tmp, args_cpy[MESS_MAX];
    strncpy(args_cpy, args, MESS_MAX);
    username = strtok_r(args_cpy, ":", &tmp);
    if (username == NULL) {
        perror("strtok_r");
        return -1;
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

    // si elle existe, supprimer la ligne contenant le user et l'ancienne valeur qui lui est associée
    if (delete_user_file_line(ndata->datafile, username) == -1)
        return -1;


    // si le dernier caractere est un \n, le supprimer ! sinon doublon de \n et ligne vide inutile

    // ajouter la nouvelle ligne
    FILE *datafile = fopen(ndata->datafile, "a+");
    if (datafile == NULL) {
        perror("fopen");
        return -1;
    }

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

    // ajout de la nouvelle ligne
    if (fputs(args, datafile) == EOF) {
        perror("fputs");
        return -1;
    }

    if (fclose(datafile) == EOF) {
        perror("fclose");
        return -1;
    }

    char msg[MESS_MAX];
    int val;
    val = snprintf(msg, MESS_MAX, "writeres success:%s;", username);
    if (val >= MESS_MAX) {
        fprintf(stderr, "snprintf truncate\n");
        return 1;
    } else if (val < 0) {
        perror("snprintf");
        return -1;
    }

    if (send_to_relais(ndata->sock, msg, val) == -1)
        return -1;

    return 0;
}

int delete_user_file_line (const char *filename, const char *username)
{
    char tmp_w_filename[MESS_MAX];
    sprintf(tmp_w_filename, "%s_tmp", filename);

    FILE *file_r = fopen(filename, "r");
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

    char *usr, *tmp, line_cpy[MESS_MAX];
    size_t username_len = strlen(username), n = DATAFILE_LINE_MAX;
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
            // do not add this line to the buffer
            printf("SKIP THIS LINE\n");
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

    if (rename(tmp_w_filename, filename) == -1) {
        perror("rename");
        return -1;
    }

    return 0;
}

int node_delete (const node_data *ndata, const char *args) // args must be null terminated
{
    const char *username = args;

    int tmpfd;
    if ((tmpfd = open(ndata->datafile, O_RDONLY | O_CREAT, 0644)) == -1) {
        perror("open");
        return -1;
    }
    if (close(tmpfd) == -1) {
        perror("close");
        return -1;
    }

    // si elle existe, supprimer la ligne contenant le user et l'ancienne valeur qui lui est associée
    if (delete_user_file_line(ndata->datafile, username) == -1)
        return -1;

    char msg[MESS_MAX];
    int val;
    val = snprintf(msg, MESS_MAX, "deleteres success:%s;", username);
    if (val >= MESS_MAX) {
        fprintf(stderr, "snprintf truncate\n");
        return 1;
    } else if (val < 0) {
        perror("snprintf");
        return -1;
    }

    if (send_to_relais(ndata->sock, msg, val) == -1)
        return -1;

    return 0;
}

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
