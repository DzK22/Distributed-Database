#include "../headers/datagram.h"

int dgram_print_status (const uint8_t status)
{
    if (status > NORMAL) // error
        printf("\033[91mErreur: "); // red
    else if (status < NORMAL)
        printf("\033[92mSuccès: "); // green
    else // == NORMAL
        printf("\033[96m"); // cyan

    switch (status) {
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
    return 0;
}

int dgram_add_from_raw (dgram **dglist, void *raw, const size_t raw_size, const struct sockaddr_in *saddr)
{
    // ici parser le paquet brut. si un paquet avec meme ip, port, request, status, existe deja, concatener le data avec l'actuel.
    // sinon, creer un nouveau paquet

    // début du paquet si first_octet == DG_FIRST_OCTET
    bool new_dgram = false;
    if (((uint16_t *) raw)[0] == DG_FIRST_OCTET)
        new_dgram = true;

    if (!new_dgram) {
        // SUITE DE DATA
        dgram *dg = *dglist;
        while (dg != NULL) {
            if ((dg->addr == saddr->sin_addr.s_addr) && (dg->port == saddr->sin_port)) {
                if ((raw_size + dg->data_len) > dg->data_size) {
                    // DEPASSEMENT BUFFER
                    return 1;
                }
                // concatener le data
                const size_t n = (((int) raw_size) > (((int) dg->data_size) - ((int) dg->data_len))) ? ((int) dg->data_size) - ((int) dg->data_len) : ((int) raw_size);
                memcpy(dg->data + dg->data_len, raw, n);
                return 0;
            }
            dg = dg->next;
        }
    } else {
        // NOUVEAU PAQUET
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
        new_dg->id = ((uint16_t *) raw)[1];
        new_dg->request = ((uint8_t *) raw)[4];
        new_dg->status = ((uint8_t *) raw)[5];
        new_dg->data_len = ((uint16_t *) raw)[3];
        new_dg->checksum = ((uint16_t *) raw)[4];
        new_dg->data = malloc(new_dg->data_len);
        if (new_dg->data == NULL) {
            perror("malloc");
            return -1;
        }

        if (gettimeofday(&new_dg->creation_time, NULL) == -1) {
            perror("gettimeofday");
            return -1;
        }
        new_dg->addr = saddr->sin_addr.s_addr;
        new_dg->port = saddr->sin_port;
        new_dg->next = *dglist;
        *dglist = new_dg;
    }

    return 0;
}

dgram * dgram_del_from_id (dgram *dglist, const uint16_t id)
{
    dgram *dg = dglist, *old = NULL;
    while (dg != NULL) {
        if (dg->id == id) {
            // del this dgram
            dgram *to_return;
            if (old == NULL)
                to_return = NULL;
            else {
                to_return = old;
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

dgram * dgram_del_from_ack (dgram *dglist, const uint16_t ack)
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

dgram * dgram_get_by_id (dgram *dglist, const uint16_t id)
{
    dgram *dg = dglist;
    while (dg != NULL) {
        if (dg->id == id)
            return dg;
        dg = dg->next;
    }

    return NULL;
}

unsigned time_ms_diff (struct timeval *tv1, struct timeval *tv2) // le plus récent en 1er
{
   return (tv1->tv_sec - tv2->tv_sec) * 1000 - (tv1->tv_usec - tv2->tv_usec) / 1000;
}

int dgram_check_timeout_delete (dgram **dglist)
{
    dgram *dg = *dglist, *old = NULL;
    struct timeval now;
    if (gettimeofday(&now, NULL) == -1) {
        perror("gettimeofday");
        return -1;
    }
    while (dg != NULL) {
        if (time_ms_diff(&now, &dg->creation_time) > DG_DELETE_TIMEOUT) {
            // supprimer ce dgram
            if (old == NULL)
                *dglist = dg->next;
            else
                old->next = dg->next;
            free(dg->data);
            free(dg);
        }

        old = dg;
        dg = dg->next;
    }

    return 0;
}


int dgram_check_timeout_resend (const int sock, dgram **dglist)
{
    dgram *dg = *dglist;
    struct timeval now;
    if (gettimeofday(&now, NULL) == -1) {
        perror("gettimeofday");
        return -1;
    }
    while (dg != NULL) {
        if (time_ms_diff(&now, &dg->creation_time) > DG_RESEND_TIMEOUT) {
            // réenvoyer ce dgram et actualiser le temps
            if (dgram_send(sock, dg, dglist) == -1)
                return -1;
        }
        dg = dg->next;
    }

    return 0;
}

int dgram_create_raw (const dgram *dg, void *buf, size_t buf_size)
{
    // seulement id, request, status, data_size et checksum doivent êtres remplis (HEADER)
    if (buf_size < ((size_t) (DG_HEADER_SIZE + dg->data_len))) {
        fprintf(stderr, "dgram_create_raw size error\n");
        return -1;
    }

    // header
    ((uint16_t *) buf)[0] = dg->id;
    ((uint8_t *) buf)[2] = dg->request;
    ((uint8_t *) buf)[3] = dg->status;
    ((uint16_t *) buf)[2] = dg->data_size;
    ((uint16_t *) buf)[3] = dg->checksum;
    // data
    memcpy(buf + DG_HEADER_SIZE, dg->data, dg->data_len);
    return 0;
}

int dgram_create (dgram *dg, const uint16_t id, const uint8_t request, const uint8_t status, const uint16_t data_size, char *data)
{
    dg->id = id;
    dg->request = request;
    dg->status = status;
    dg->data_size = data_size;
    dg->data = malloc(data_size);
    if (dg->data == NULL) {
        perror("malloc");
        return -1;
    }
    memcpy(dg->data, data, data_size);
    dg->data_len = data_size;
    dg->checksum = dgram_checksum(dg);

    return 0;
}

uint16_t dgram_checksum (const dgram *dg)
{
    // champs minimum devant êtres remplis dans dg:
    // data, data_size
    // le dg doit etre complet (data_size == data_len)
    long long unsigned sum = 0;
    char c;
    unsigned i;
    for (i = 0; i < dg->data_size; i ++) {
        c = dg->data[i];
        sum += c * i;
    }

    return (uint16_t) (sum % ((long long unsigned) pow(2, 16)));
}

dgram * dgram_add (dgram *dglist, dgram *dg) // /!/ dg doit avoir été alloué avec malloc
{
    if (dglist == NULL)
        return dg;
    dg->next = dglist;
    return dg;
}

int dgram_send (const int sck, dgram *dg, dgram **dg_sent)
{
    *dg_sent = dgram_add(*dg_sent, dg);

    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = dg->addr;
    saddr.sin_port = dg->port;
    char buf[DG_DATA_MAX + DG_HEADER_SIZE];
    if (dgram_create_raw(dg, buf, DG_DATA_MAX + DG_HEADER_SIZE) == -1)
        return -1;
    if (sck_send(sck, &saddr, buf, dg->data_len + DG_HEADER_SIZE) == -1)
        return -1;

    return 0;
}
