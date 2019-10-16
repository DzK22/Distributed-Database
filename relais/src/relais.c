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

ssize_t send_toclient (const int sockfd, const char *msg, struct sockaddr_in *client)
{
    socklen_t len = sizeof(struct sockaddr_in);
    ssize_t bytes = sendto(sockfd, msg, N, 0, (struct sockaddr *) client, len);
    if (bytes == -1) {
        perror("sendto error");
        return -1;
    }
    return bytes;
}

int wait_for_request (int sock)
{
    fd_set ensemble;
    struct timeval delai;
    delai.tv_sec = 600;
    delai.tv_usec = 0;
    int sel, maxsock = sock + 1;
    struct sockaddr_in clientaddr;
    socklen_t clientaddr_len = sizeof(struct sockaddr_in);
    ssize_t bytes;
    char buff[N];
    unsigned buff_offset = 0;
    clientreq creq;

    while (true)
    {
        FD_ZERO(&ensemble);
        FD_SET(sock, &ensemble);
        sel = select(maxsock, &ensemble, NULL, NULL, &delai);
        switch (sel)
        {
            case -1:
                perror("select error");
                return -1;

            case 0:
                fprintf(stderr, "Timeout reached\n");
                return -1;

            default:
                /* client request */
                if (FD_ISSET(sock, &ensemble)) {
                    while ((bytes = recvfrom(sock, buff + buff_offset, N - buff_offset - 1, MSG_DONTWAIT, (struct sockaddr *) &clientaddr, &clientaddr_len)) > 0) {
                        buff_offset += bytes;
                    }

                    if ((bytes == -1) && (errno != EAGAIN) && (errno != EWOULDBLOCK)) {
                        perror("recvfrom error");
                        return -1;
                    }

                    buff[buff_offset] = '\0';
                    if (parse_datagram(buff, &creq) == -1)
                        return -1;
                    if (exec_client_request(sock, &creq) == -1)
                        return -1;
                    buff_offset = 0;
                }
        }
    }
}

int parse_datagram (char *data, clientreq *cr) // data should be null terminated
{
    char tmp[N];
    errno = 0;
    if (sscanf(data, "%s %s;", tmp, cr->message) != 2) {
        if (errno != 0)
            perror("sscanf error");
        else
            fprintf(stderr, "cannot assing all sscanf items\n");
        return -1;
    }

    if (strncmp(tmp, "auth", N) == 0)
        cr->type = Authentification;
    else if (strncmp(tmp, "lire", N) == 0)
        cr->type = Read;
    else if (strncmp(tmp, "ecrire", N) == 0)
        cr->type = Write;
    else if (strncmp(tmp, "supprimer", N) == 0)
        cr->type = Delete;
    else {
        fprintf(stderr, "parse_datagram: wrong message type\n");
        return -1;
    }

    return 0;
}

int exec_client_request (int sock, clientreq *cr)
{
    (void) sock;
    switch (cr->type) {
        case Authentification:
            printf("AUTHENTIFICATION => %s\n", cr->message);
            if (authentification(cr) == true)
                printf("success\n");
            else
                printf("failed\n");
            break;
        case Read:
            break;
        case Write:
            break;
        case Delete:
            break;
        default:
            return 1;
    }

    return 0;
}

bool authentification (clientreq *creq)
{
    char *tmp;
    const char *login = strtok_r(creq->message, ":", &tmp);
    const char *password = strtok_r(NULL, ":", &tmp);
    if (!login || !password) {
        fprintf(stderr, "Authentification error: login or password is missing\n");
        return false;
    }

    FILE *users = fopen("users.txt", "r");
    if (users == NULL) {
        perror("fopen error");
        return false;
    }

    size_t login_len = strlen(login);
    size_t password_len = strlen(password);
    char line[N];
    char *flogin, *fpassword;
    bool login_ok, password_ok;
    while (fgets(line, N, users) != NULL)
    {
        flogin = strtok_r(line, ":", &tmp);
        fpassword = strtok_r(NULL, ":", &tmp);
        login_ok = strncmp(flogin, login, login_len) == 0;
        password_ok = strncmp(fpassword, password, password_len) == 0;
        if (login_ok && password_ok)
            return true;
    }

    if (fclose(users) == EOF) {
        perror("fclose error");
        return false;
    }

    return false;
}

/*
bool cmd_test(char *rest, FILE *fp, const char *user)
{
    char *tmp = "";
    int acc = -1;
    tmp = strtok_r(rest, " ", &rest);

    if (strncmp(tmp, "lire", strlen(tmp)) == 0)
        acc = 0;
    else if (strncmp(tmp, "ecrire", strlen(tmp)) == 0)
        acc = 1;
    else if (strncmp(tmp, "supprimer", 9) == 0)
        acc = 2;

    switch (acc)
    {
        case -1:
            return false;

        case 0:
            //ICI IL FAUDRA VERIFIER SI L'UTILISATEUR APPARAIT DANS LE FICHIER + LE NOM DE CHAMP QU'IL A ENVOYER POUR VALIDER S'IL A LES DROITS OU PAS
            printf("JE PEUX LIRE HAHA\n");
            char *ligne;
            char temp[N] = "";
            char *tmp1 = "";
            char *tmp2 = "";
            char *another = "";
            char res[N] = "";
            char *testa = "";
            char *recevoir = "";
            char *cmd = "";
            cmd = strtok_r(rest, " ", &recevoir);
            printf("cmd = %s\n", cmd);
            while ((ligne = fgets(temp, N, fp)) != NULL)
            {
                tmp1 = strtok_r(temp, ":", &another);
                tmp2 = strtok_r(another, ":", &another);
                printf("tmp1 = %s\n", tmp1);
                printf("tmp2 = %s\n", tmp2);
                snprintf(res, N, "%s:%s", tmp1, tmp2);
                printf("res = %s\n\n", res);
                if (strncmp(res, user, strlen(another)) == 0)
                {
                    while ((testa = strtok_r(another, ":", &another)) != NULL)
                    {
                        printf("testa = %s\n", testa);
                        printf("cmd = %s\n", cmd);
                        if (strncmp(testa, cmd, strlen(testa)) == 0)
                            return true;
                        else
                            return false;
                    }
                }
            }
            return false;

        case 1:
            //ICI IL FAUDRA VERFIER SI LE CLIENT VEUT MODIFIER UN CHAMP LE CONCERNANT
            printf("JE PEUX ECRIRE QUE SI CEST MOI\n");
            return true;

        case 2:
            //ICI ON SUPPRIME TOUS LES CHAMPS RELATIFS A UN UTILISATEUR
            printf("JE SUPPRIME TOUT HAHA\n");
            return true;

        default:
            printf("AUCUN\n");
            return false;
    }
}
*/
