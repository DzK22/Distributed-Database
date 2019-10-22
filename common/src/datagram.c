#include "../headers/datagram.h"

int dgram_print_status (dgram *dg)
{
    if (dg->status < 0) // error
        printf("\033[91mErreur: "); // red
    else if (dg->status > 0)
        printf("\033[92mSuccès: "); // green
    else // n == 0
        printf("\033[96m"); // cyan

    switch (dg->status) {
        case SUC_DELETE:
            printf("La suppression a réussie");
            break;
        case SUC_WRITE:
            printf("L'écriture à réussie");
            break;
        case SUC_AUTH:
            printf("L'authentification à réussie");
            break;
        case NORMAL:
            break;
        case ERR_NOREPLY:
            printf("Pas de réponse");
            break;
        case ERR_AUTHFAILED:
            printf("L'authentification à échouée");
            break;
        case ERR_NOPERM:
            printf("Permission refusées");
            break;
        case ERR_NONODE:
            printf("Aucun noeud trouvé");
            break;
        case ERR_SYNTAX:
            printf("Syntaxe incorrecte");
            break;
        case ERR_UNKNOWFIELD:
            printf("Champ inconnu");
            break;
        case ERR_WRITE:
            printf("L'écriture a échouée");
            break;
        case ERR_DELETE:
            printf("La suppression a échouée");
            break;
        default:
            fprintf(stderr, "Code de status incorrect\n");
            return -1;
    }

    printf("\033[0m\n"); // reset color
}

dgram * dgram_add_from_raw (dgram *dglist, const char *raw)
{
    // ici parser le paquet brut. si un paquet avec meme ip, port, request, status, existe deja, concatener le data avec l'actuel.
    // sinon, creer un nouveau paquet
}

void dgram_del_from_ack (dgram *dglist, unsigned ack)
{

}
