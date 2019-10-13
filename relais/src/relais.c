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

bool authentification (FILE *fp, const char *recu, const size_t len)
{
    char res[N], total[N], *ligne, *another, *tmp, *tmp2;
    while ((ligne = fgets(res, N, fp)) != NULL)
    {
        tmp = strtok_r(res, ":", &another);
        tmp2 = strtok_r(another, ":", &another);
        snprintf(total, N, "%s:%s", tmp, tmp2);
        printf("tmp = %s\n", tmp);
        printf("tmp2 = %s\n", tmp2);
        printf("total = %s\n", total);
        if (strncmp(total, recu, len) == 0)
            return true;
    }
    return false;
}

bool cmd_test(char *rest, FILE *fp, const char *user)
{
    char *tmp = "";
    int acc = -1;
    tmp = strtok_r(rest, " ", &rest);

    if (strncmp(tmp, "lire", strlen(tmp)) == 0)
        acc = 0;
    if (strncmp(tmp, "ecrire", strlen(tmp)) == 0)
        acc = 1;
    if (strncmp(tmp, "supprimer", 9) == 0)
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
