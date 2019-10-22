#include "../headers/datagram.h"

int dgram_print_status (const dgram *dg)
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

int dgram_add_from_raw (dgram *dglist, dgram **newdg, void *raw, const size_t raw_size, const struct sockaddr_in *saddr)
{
    // ici parser le paquet brut. si un paquet avec meme ip, port, request, status, existe deja, concatener le data avec l'actuel.
    // sinon, creer un nouveau paquet

    dgram *dg = dglist;
    while (dg != NULL) {
        if (!dg->ready && (dg->addr == saddr->sin_addr.s_addr) && (dg->port == saddr->sin_port)) {
            // concatener le data
            const size_t n = (raw_size > (dg->data_size - dg->data_len)) ? dg->data_size - dg->data_len : raw_size;
            memcpy(dg->data + dg->data_len, raw, n);
            return 0;
        }
        dg = dg->next;
    }

    if (raw_size < 8) {
        // taille insuffisante pour être un en-tête
        return 1;
    }

    // si ici, ajouter un nouveau datagram
    dgram *new_dg = malloc(sizeof(dgram));
    if (new_dg == NULL) {
        perror("malloc");
        return -1;
    }
    new_dg->id = ((u_int16_t *) raw)[0];
    new_dg->request = ((u_int8_t *) raw)[2];
    new_dg->status = ((u_int8_t *) raw)[3];
    new_dg->data_len = ((u_int16_t *) raw)[2];
    new_dg->checksum = ((u_int16_t *) raw)[3];
    new_dg->data = malloc(new_dg->data_len);
    if (new_dg->data == NULL) {
        perror("malloc");
        return -1;
    }

    new_dg->next = dglist;
    return 0;
}

dgram * dgram_del_from_ack (dgram *dglist, const u_int16_t ack)
{
    dgram *old = NULL;
    dgram *dg = dglist;
    while (dg != NULL) {
        if (dg->id == ack) {
            dgram *to_return;
            if (old == NULL) {
               to_return = dg->next; 
            } else {
                to_return = dglist;
                old->next = dg->next;
            }
            free(dg->data);
            free(dg);
            return to_return;
        }
        old = dg;
        dg = dg->next;
    }

    return NULL;
}
