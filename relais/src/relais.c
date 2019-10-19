/**
 * \file relais.c
 * \brief Définitions des fonctions gérant le serveur d'accès
 * \author François et Danyl
 */


#include "../headers/relais.h"

int socket_create ()
{
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == -1) {
        perror("socket creation error");
        return -1;
    }
    return sock;
}

int candbind (const int sockfd, struct sockaddr_in *addr, const char *port)
{
    addr->sin_addr.s_addr = INADDR_ANY;
    addr->sin_family = AF_INET;
    addr->sin_port = htons(atoi(port));
    socklen_t len = sizeof(struct sockaddr_in);

    int bd = bind(sockfd, (struct sockaddr *) addr, len);
    if (bd == -1) {
        perror("bind error");
        return -1;
    }
    return 0;
}

ssize_t send_toclient (const int sockfd, void *msg, struct sockaddr_in *client)
{
    socklen_t len = sizeof(struct sockaddr_in);
    ssize_t bytes = sendto(sockfd, msg, N, 0, (struct sockaddr *) client, len);
    if (bytes == -1) {
        perror("sendto error");
        return -1;
    }
    return bytes;
}

int wait_for_request (int sock, mugiwara *mugi)
{
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(struct sockaddr_in);
    ssize_t bytes;
    char buff[N];
    clientreq creq;

    while ((bytes = recvfrom(sock, buff, N - 1, 0, (struct sockaddr *) &clientaddr, &clientaddr_len)) >= 0) {
        buff[bytes] = '\0';
        switch (parse_datagram(buff, &creq, &clientaddr)) {
            case -1:
                return -1;
            case 1: // not program error, only incorrect message received
                continue;
        }
        if (exec_client_request(sock, &creq, mugi) == -1)
            return -1;
    }

    perror("recvfrom");
    return -1;
}

int parse_datagram (char *data, clientreq *cr, struct sockaddr_in *client) // data should be null terminated
{
    char tmp[N];
    errno = 0;
    if (sscanf(data, "%s %s;", tmp, cr->message) != 2) {
        if (errno != 0) {
            perror("sscanf error");
            return -1;
        }
        // else
        fprintf(stderr, "cannot assing all sscanf items: %s\n", data);
        return 1; // don't quit the program
    }
    cr->saddr = *client;
    if (strncmp(tmp, "auth", 5) == 0)
        cr->type = Authentification;
    else if (strncmp(tmp, "lire", 5) == 0)
        cr->type = Read;
    else if (strncmp(tmp, "ecrire", 7) == 0)
        cr->type = Write;
    else if (strncmp(tmp, "supprimer", 10) == 0)
        cr->type = Delete;
    else if (strncmp(tmp, "meet", 5) == 0)
        cr->type = Meet; // REQUETE D'UN NOUVEAU NOEUD, PAS D'UN CLIENT !
    else if (strncmp(tmp, "getallres", 10) == 0)
        cr->type = Getallres; // IDEM
    else if (strncmp(tmp, "readres", 8) == 0)
        cr->type = Readres;
    else {
        printf("message avec mauvais type recu: %s\n", tmp);
        return 1;
    }

    return 0;
}

int exec_client_request (int sock, clientreq *cr, mugiwara *mugi)
{
    user *usr;
    //static int z = 0;
    switch (cr->type) {
        case Authentification:
            printf("AUTHENTIFICATION => %s\n", cr->message);
            if (authentification(cr, mugi)) {
                printf("success\n");
                if (send_toclient(sock, "0", &cr->saddr) == -1)
                    return -1;
            } else {
                printf("failed\n");
                if (send_toclient(sock, "-1", &cr->saddr) == -1)
                    return -1;
            }
            break;
        case Read:
            usr = read_has_rights(cr, mugi);
            if (usr == NULL) 
                return 1;
            if (node_read_request(sock, cr, mugi, usr) == -1)
                return -1;
            break;
        case Write:
            break;
        case Delete:
            break;
        case Meet: // REQUETE D'UN NOEUD, PAS D'UN CLIENT
            if (meet_new_node(sock, cr, mugi) == -1)
                return -1;
            break;
        case Getallres: // IDEM
            if (follow_getallres(sock, cr, mugi) == -1)
                return -1;
            break;
        case Readres: // IDEM (réponse de la requete read du relais vers le noeud)
            if (follow_readres(sock, cr, mugi) == -1)
                return -1;
            break;
        default:
            fprintf(stderr, "Requête non reconnue\n");
            return 1;
    }
    return 0;
}

auth_user * get_auth_user_from_login (const char *login, mugiwara *mugi)
{
    unsigned i;
    size_t login_len = strlen(login);
    for (i = 0; i < mugi->nb_hosts; i ++) {
       if (strncmp(login, mugi->hosts[i].login, login_len) == 0)
           return &mugi->hosts[i];
    }

    return NULL;
}

user * get_user_from_req (clientreq *creq, mugiwara *mugi)
{
    unsigned i;
    bool ip_ok, port_ok;
    for (i = 0; i < mugi->nb_hosts; i ++) {
        ip_ok = creq->saddr.sin_addr.s_addr == mugi->hosts[i].saddr.sin_addr.s_addr;
        if (!ip_ok)
            continue;
        port_ok = creq->saddr.sin_port == mugi->hosts[i].saddr.sin_port;
        if (!port_ok)
            continue;
        // OK
        const char *login = mugi->hosts[i].login;
        const size_t login_len = strlen(login);
        // search for the corresponding user in the users array
        for (i = 0; i < mugi->nb_users; i ++) {
            if (strncmp(login, mugi->users[i].login, login_len) == 0)
                return &mugi->users[i];
        }
    }

    return NULL;
}

user * read_has_rights (clientreq *creq, mugiwara *mugi)
{
    user *usr = get_user_from_req(creq, mugi);
    if (!usr) {
        printf("putin fils de pute\n");
        return NULL;
    }

    char *tmp, *attr;
    size_t i, attr_len;
    while ((attr = strtok_r(creq->message, ",", &tmp)) != NULL) {
        attr_len = strlen(attr);
        for (i = 0; i < usr->attributs_len; i ++) {
            if (strncmp(attr, usr->attributs[i], attr_len) != 0) {
                printf("has right FALSE\n");
                return NULL;
            }
        }
    }

    printf("has right TRUE\n");
    return usr;
}

bool authentification (clientreq *creq, mugiwara *mugi)
{
    char *tmp;
    const char *login = strtok_r(creq->message, ":", &tmp);
    const char *password = strtok_r(NULL, ":", &tmp);
    if (!login || !password) {
        fprintf(stderr, "Authentification error: login or password is missing\n");
        return false;
    }
    char *flogin, *fpassword;
    bool login_ok, password_ok;
    size_t i;
    for (i = 0; i < mugi->nb_users; i++)
    {
        flogin = mugi->users[i].login;
        fpassword = mugi->users[i].mdp;
        login_ok = strncmp(flogin, login, strlen(flogin)) == 0;
        password_ok = strncmp(fpassword, password, strlen(fpassword)) == 0;
        if (login_ok && password_ok)
        {
            if (test_auth(mugi, login))
                return false;

            if (mugi->nb_hosts == 0)
            {
                mugi->hosts = malloc(sizeof(auth_user) * H);
                if (mugi->hosts == NULL)
                {
                    perror("malloc error");
                    return EXIT_FAILURE;
                }
            }
            if (mugi->nb_hosts >= mugi->max_hosts)
            {
                mugi->max_hosts += H;
                mugi->hosts = realloc(mugi->hosts, mugi->max_hosts);
                if (mugi->hosts == NULL)
                {
                    perror("realloc error");
                    return false;
                }
            }
            strncpy(mugi->hosts[mugi->nb_hosts].login, login, strlen(login));
            mugi->hosts[mugi->nb_hosts].saddr = creq->saddr;
            mugi->nb_hosts ++;
            return true;
        }
    }
    return false;
}

bool test_auth(mugiwara *mugi, const char *log)
{
    size_t i;
    for (i = 0; i < mugi->nb_hosts; i++)
    {
        if (strncmp(log, mugi->hosts[i].login, strlen(log)) == 0)
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
    fseek(fp, 0, SEEK_SET);

    for (i = 0; i < nb_lines; i ++) {
        fgets(line, N, fp);
        str = strtok_r(line, ":", &tmp); // login
        strncpy(mugi->users[i].login, str, MAX_ATTR);
        str = strtok_r(NULL, ":", &tmp); // psswd
        strncpy(mugi->users[i].mdp, str, MAX_ATTR);
        //printf("login = %s\n", login);
        //printf("mdp = %s\n", mdp);

        unsigned j = 0;
        while ((str = strtok_r(NULL, ":", &tmp)) != NULL) {
            //printf("attr = %s\n", str);
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

int meet_new_node (const int sock, clientreq *creq, mugiwara *mugi)
{
    // send confirmation
    char confstr[N] = "success;";
    if (send_toclient(sock, confstr, &creq->saddr) == -1)
        return -1;

    if (mugi->nb_nodes >= mugi->max_nodes) {
        mugi->max_nodes += H;
        mugi->nodes = realloc(mugi->nodes, sizeof(node) * mugi->max_nodes);
        if (mugi->nodes == NULL) {
            perror("realloc");
            return -1;
        }
    }

    // ajouter le noeud
    strcpy(mugi->nodes[mugi->nb_nodes].field, creq->message);
    mugi->nodes[mugi->nb_nodes].saddr = creq->saddr;
    mugi->nodes[mugi->nb_nodes].id = mugi->node_id_counter;
    mugi->nodes[mugi->nb_nodes].active = true;
    mugi->nb_nodes ++;
    mugi->node_id_counter ++;

    // si un noeud du meme type existe deja, ecraser les données du nouveau noeud par les siennes (surement plus à jour) envoie message a ce noeud pour lui demander toutes ses donnees
    unsigned i;
    for (i = 0; i < (mugi->nb_nodes - 1); i ++) {
        if (strcmp(creq->message, mugi->nodes[i].field) == 0) {
            // bloquer temporairement le noeud car n'a pas les données a jour
            mugi->nodes[mugi->nb_nodes - 1].active = false;
            char msg[N];
            if (sprintf(msg, "getall %u;", mugi->node_id_counter - 1) < 0) {
                fprintf(stderr, "sprintf error\n");
                return -1;
            }

            if (send_toclient(sock, msg, &mugi->nodes[i].saddr) == -1)
                return -1;
        }
    }

    return 0;
}

int follow_getallres (const int sock, clientreq *creq, mugiwara *mugi)
{
    // message de creq = id_node:data;
    unsigned to_id;
    char data[N];
    switch (sscanf(creq->message, "%u:%s;", &to_id, data)) {
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

int node_read_request (const int sock, clientreq *creq, mugiwara *mugi, user *usr)
{
    // recherche d'un noeud valide pour chaque champ de la reqûete
    unsigned i;
    char *tmp, *field, buf[N];
    int val;

    while ((field = strtok_r(creq->message, ",", &tmp)) != NULL) {
        for (i = 0; i < mugi->nb_nodes; i ++) {
            if (strncmp(field, mugi->nodes[i].field, strlen(field)) != 0)
                continue;
            if (!mugi->nodes[i].active)
                continue;

            // OK
            val = snprintf(buf, N - 1, "read %s:%s;", usr->login, field);
            if (val >= N) {
                fprintf(stderr, "snprintf truncate\n");
                return 1;
            } else if (val < 0) {
                perror("snprintf");
                return -1;
            }

            buf[val] = '\0';
            if (send_toclient(sock, buf, &mugi->nodes[i].saddr) == -1)
                return -1;
            return 0;
        }
    }

    // NO CORRESPONDING NODE AVAILABLE
    printf("NO NODE FOUND POUR LA REQUEST\n");
    return 1;
}

int follow_readres (const int sock, clientreq *creq, mugiwara *mugi)
{
    printf("follow readres bg\n");
    // /!/ creq->message = <user>:<data>;
    char *username, *data, *tmp;
    username = strtok_r(creq->message, ":", &tmp);
    if (username == NULL) {
        perror("strtok_r");
        return -1;
    }
    data = strtok_r(NULL, ":", &tmp);
    if (data == NULL) {
        perror("strtok_r");
        return -1;
    }

    auth_user *authusr = get_auth_user_from_login(username, mugi);
    if (authusr == NULL)
        return 1;
    if (send_toclient(sock, data, &authusr->saddr) == -1)
        return -1;

    return 0;
}
