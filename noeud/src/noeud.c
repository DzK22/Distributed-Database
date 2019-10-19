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
    while ((bytes = send(sock, buf + total, buf_len - total, 0)) > 0)
        total += bytes;
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
    size_t total = 0;
    while ((bytes = recvfrom(sock, buf + total, buf_max - total, MSG_DONTWAIT, (struct sockaddr *) sender_saddr, &sender_saddr_len)) > 0)
        total += bytes;

    if ((bytes == -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
        perror("recvfrom");
        return -1;
    }

    return total;
}

int meet_relais (const node_data *ndata)
{
    char str[MESS_MAX];
    int tmp = snprintf(str, FIELD_MAX, "meet %s;", ndata->field);
    if ((tmp >= FIELD_MAX) || (tmp < 0)) {
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
    fd_set rset;
    char buf[MESS_MAX];
    ssize_t bytes;
    struct sockaddr_in sender_saddr;
    relaisreq rreq;

    while (true) {
        FD_ZERO(&rset);
        FD_SET(ndata->sock, &rset);

        switch(select(ndata->sock + 1, &rset, NULL, NULL, NULL)) {
            case -1:
                perror("select");
                return;
            case 0:
                fprintf(stderr, "Timeout reached\n");
                return;
            default:
               if ((bytes = recv_from_relais(ndata->sock, buf, MESS_MAX - 1, &sender_saddr)) == -1)
                   return;

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
    }
}

int parse_datagram (const char *buf, relaisreq *rreq) // buf must be null terminated
{
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

    if (strncmp(tmp, "lire", 5) == 0)
        rreq->type = Read;
    else if (strncmp(tmp, "ecrire", 7) == 0)
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
            printf("READ: %s\n", rreq->args);
            if (node_read(ndata, rreq->args) == -1)
                return -1;
            break;
        case Write:
            printf("WRITE: %s\n", rreq->args);
            if (node_write(ndata, rreq->args) == -1)
                return -1;
            break;
        case Delete:
            printf("DELETE: %s\n", rreq->args);
            if (node_delete(ndata, rreq->args) == -1)
                return -1;
            break;
        case Changeall:
            printf("CHANGE ALL, new data: %s\n", rreq->args);
            if (change_all_data(ndata, rreq->args) == -1)
                return -1;
            break;
        case Getall:
            printf("GET ALL\n");
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

    // verifions que ce noeud possède bien le champs
    if (strcmp(args, ndata->field) != 0) {
        send_to_relais(ndata->sock, "3", 1);
        return 0;
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
    char response[MESS_MAX];
    size_t len;
    size_t n = DATAFILE_LINE_MAX;

    errno = 0;
    while ((bytes = getline(&line, &n, datafile)) != -1) {
        len = strlen(response);
        strncat(response, line, MESS_MAX - len -1);
        if (len >= MESS_MAX)
            break;
        strncat(response, ",", MESS_MAX - len - 1);
    }

    free(line);
    if (errno != 0) {
        perror("getline");
        return -1;
    }

    if (fclose(datafile) == EOF) {
        perror("fclose");
        return -1;
    }

    if (send_to_relais(ndata->sock, response, strlen(response)) == -1)
        return -1;

    return 0;
}

int node_write (const node_data *ndata, const char *args) // args must be null terminated
{
    (void) ndata;
    (void) args;

    return 0;
}

int node_delete (const node_data *ndata, const char *args) // args must be null terminated
{
    (void) ndata;
    (void) args;

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
