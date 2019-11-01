#include "../headers/datagram.h"

int dgram_print_status (const dgram *dg)
{
    if (dg->status == NORMAL) // error
        printf("\033[96m"); // cyan
    else if (dg->status == SUCCESS)
        printf("\033[92m"); // green
    else // error
        printf("\033[91m"); // red

    switch (dg->status) {
        case SUCCESS:
            switch (dg->request) {
                case RRES_AUTH:
                    printf("L'authentification a réussie");
                    break;
                case RRES_WRITE:
                    printf("L'écriture a réussie");
                    break;
                case RRES_DELETE:
                    printf("La suppression a réussie");
                    break;
            }
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
        case ERR_NOTAUTH:
            printf("Vous n'êtes pas authentifié");
            break;
        case ERR_ALREADYAUTH:
            printf("Votre login/mdp est déjà connecté");
            break;
        default:
            fprintf(stderr, "Code de status incorrect\n");
            return -1;
    }

    printf("\033[0m\n"); // reset color
    return 0;
}

bool dgram_is_ready (const dgram *dg)
{
    return dg->data_len == dg->data_size;
}

int dgram_add_from_raw (dgram **dglist, void *raw, const size_t raw_size, dgram *curdg, const struct sockaddr_in *saddr)
{
    // ici parser le paquet brut. si un paquet avec meme ip, port, request, status, existe deja, concatener le data avec l'actuel.
    // sinon, creer un nouveau paquet

    // début du paquet si first_octet == DG_FIRST_OCTET
    bool new_dgram = false;
    size_t data_len;
    if (((uint16_t *) raw)[0] == DG_FIRST_2OCTET)
        new_dgram = true;

    if (!new_dgram) {
        data_len = raw_size;
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
                *curdg = *dg;
                return 0;
            }
            dg = dg->next;
        }
    } else {
        // NOUVEAU PAQUET
        if (raw_size < DG_HEADER_SIZE) {
            // taille insuffisante pour être un en-tête
            fprintf(stderr, "Paquet rejeté, car trop petit\n");
            return 1;
        } else if (raw_size > SCK_DATAGRAM_MAX) {
            // taille trop importante, rejeter
            fprintf(stderr, "Paquet rejeté, car trop gros\n");
            return 1;
        }
        data_len = raw_size - DG_HEADER_SIZE;

        // si ici, ajouter un nouveau datagram
        dgram *new_dg = malloc(sizeof(dgram));
        if (new_dg == NULL) {
            perror("malloc");
            return -1;
        }
        new_dg->id = ((uint16_t *) raw)[1];
        new_dg->request = ((uint8_t *) raw)[4];
        new_dg->status = ((uint8_t *) raw)[5];
        new_dg->data_size = ((uint16_t *) raw)[3];
        new_dg->checksum = ((uint16_t *) raw)[4];
        new_dg->data_len = data_len;
        new_dg->data = malloc(new_dg->data_size + 1); // +1 pour le \0 de fin
        if (new_dg->data == NULL) {
            perror("malloc");
            return -1;
        }
        memcpy(new_dg->data, &(((const char *) raw)[DG_HEADER_SIZE]), new_dg->data_len);
        new_dg->data[new_dg->data_size] = '\0';
        new_dg->addr = saddr->sin_addr.s_addr;
        new_dg->port = saddr->sin_port;
        gettimeofday(&new_dg->creation_time, NULL);
        *curdg = *new_dg;

        if (new_dg->request != ACK) { // ne pas ajouter les ACK dans la liste
            new_dg->next = *dglist;
            *dglist = new_dg;
        } else
            new_dg->next = NULL;
    }

    return 0;
}

bool dgram_del_from_id (dgram **dglist, const uint16_t id) // renvoie true si un élément a été supprimé, false sinon
{
    dgram *dg = *dglist, *old = NULL;
    while (dg != NULL) {
        if (dg->id == id) {
            // del this dgram
            dgram *new_first;
            if (old == NULL)
                new_first = NULL;
            else {
                new_first = old;
                old->next = dg->next;
            }
            if (dg->data != NULL) // data peut être nul !
                free(dg->data);
            free(dg);
            *dglist = new_first;
            return true;
        }
        old = dg;
        dg = dg->next;
    }

    return false;
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
    // printf("tv1 = %ld, %ld | tv2 = %ld, %ld\n", tv1->tv_sec, tv1->tv_usec, tv2->tv_sec, tv2->tv_usec);
    return (tv1->tv_sec - tv2->tv_sec) * 1000 + (tv1->tv_usec - tv2->tv_usec) / 1000;
}

int dgram_check_timeout_delete (dgram **dgsent)
{
    dgram *tmp, *dg = *dgsent, *old = NULL;
    struct timeval now;
    gettimeofday(&now, NULL);

    while (dg != NULL) {
        if (time_ms_diff(&now, &dg->creation_time) > DG_DELETE_TIMEOUT) {
            // supprimer ce dgram
            if (old == NULL)
                *dgsent = dg->next;
            else
                old->next = dg->next;
            tmp = dg;
            dg = dg->next;
            if (tmp->data != NULL) // data peut etre nul !
                free(tmp->data);
            free(tmp);
            continue;
        }

        old = dg;
        dg = dg->next;
    }

    return 0;
}


int dgram_check_timeout_resend (const int sock, dgram **dgsent)
{
    dgram *dg = *dgsent;
    struct timeval now;
    gettimeofday(&now, NULL);

    while (dg != NULL) {
        unsigned t;
        if ((t = time_ms_diff(&now, &dg->creation_time)) > DG_RESEND_TIMEOUT) {
            if (dgram_send(sock, dg, dgsent) == -1)
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
    ((uint16_t *) buf)[0] = DG_FIRST_2OCTET;
    ((uint16_t *) buf)[1] = dg->id;
    ((uint8_t *) buf)[4] = dg->request;
    ((uint8_t *) buf)[5] = dg->status;
    ((uint16_t *) buf)[3] = dg->data_size;
    ((uint16_t *) buf)[4] = dg->checksum;

    // data
    memcpy(buf + DG_HEADER_SIZE, dg->data, dg->data_len);

    return 0;
}

int dgram_create (dgram **dgres, const uint16_t id, const uint8_t request, const uint8_t status, const uint32_t addr, const in_port_t port, const uint16_t data_size, const char *data)
{
    dgram *dg = malloc(sizeof(dgram));
    if (dg == NULL) {
        perror("malloc");
        return -1;
    }

    dg->id = id;
    dg->request = request;
    dg->status = status;
    dg->addr = addr;
    dg->port = port;
    dg->data_size = data_size;
    dg->data_len = data_size;
    dg->data = malloc(data_size + 1); // +1 pour le \0 de fin
    if (dg->data == NULL) {
        perror("malloc");
        return -1;
    }
    memcpy(dg->data, data, dg->data_len);
    dg->data[dg->data_size] = '\0';
    dg->checksum = dgram_checksum(dg);
    dg->next = NULL;

    if (gettimeofday(&dg->creation_time, NULL) == -1) {
        perror("gettimeofday");
        return -1;
    }

    *dgres = dg;
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
        sum += c * (i + 1);
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
    dgram_debug(dg, false);

    *dg_sent = dgram_add(*dg_sent, dg);
    struct sockaddr_in saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = dg->addr;
    saddr.sin_port = dg->port;
    void *buf = malloc(SCK_DATAGRAM_MAX);
    if (buf == NULL) {
        perror("malloc");
        return -1;
    }

    if (dgram_create_raw(dg, buf, SCK_DATAGRAM_MAX) == -1)
        return -1;
    if (sck_send(sck, &saddr, buf, dg->data_len + DG_HEADER_SIZE) == -1)
        return -1;

    gettimeofday(&dg->creation_time, NULL); // actualiser le temps
    return 0;
}

void * thread_timeout_loop (void *arg) // arg = thread_targ
{
    thread_targ *targ = arg;
    return NULL; // a sup quand pret a test
    while (1) {
        if (usleep(100) == -1) {
            perror("usleep");
            return NULL;
        }
        if (dgram_check_timeout_delete(targ->dgreceived) == -1)
            return NULL;
        if (dgram_check_timeout_resend(targ->sck, targ->dgsent) == -1)
            return NULL;
    }

    return NULL;
}

bool dgram_verify_checksum (const dgram *dg)
{
    if (dgram_checksum(dg) == dg->checksum)
        return true;
    return false;
}

void dgram_debug (const dgram *dg, bool received) // !received = sent
{
    char d[DG_DATA_MAX];
    memcpy(d, dg->data, dg->data_len);
    if (dg->data_len >= (DG_DATA_MAX - 1))
        return;
    d[dg->data_len] = '\0';

    if (received)
        printf("\033[35m  >> [IN] ");
    else
        printf("\033[34m  << [OUT] ");

    printf("DGRAM {");

    if (dg->request == ACK)
        printf("(ACK pour) id=");
    else
        printf("id=");
    printf("%d, request=%s, status=%s, data_size=%d, data_len=%d, checksum=%d\n\tdata=%s\033[0m\n", dg->id, dgram_request_str(dg), dgram_status_str(dg), dg->data_size, dg->data_len, dg->checksum, d);
}

int dgram_create_send (const int sck, dgram **dgsent, dgram **dgres, const uint16_t id, const uint8_t request, const uint8_t status, const uint32_t addr, const in_port_t port, const uint16_t data_size, const char *data)
{
    dgram *tmp;
    if (dgres == NULL)
        dgres = &tmp;

    if (dgram_create(dgres, id, request, status, addr, port, data_size, data) == -1)
        return -1;
    if (dgram_send(sck, *dgres, dgsent) == -1)
        return -1;
    return 0;
}

int dgram_process_raw (const int sck, dgram **dgsent, dgram **dgreceived, void *cb_data, int (*callback) (const dgram *, void *))
{
    void *buf = malloc(SCK_DATAGRAM_MAX);
    if (buf == NULL) {
        perror("malloc");
        return -1;
    }
    struct sockaddr_in saddr;
    ssize_t bytes = sck_recv(sck, buf, SCK_DATAGRAM_MAX, &saddr);
    if (bytes == -1)
        return -1;

    dgram dg;
    if (dgram_add_from_raw(dgreceived, buf, bytes, &dg, &saddr) == -1)
        return -1;

    dgram_debug(&dg, true);

    if (dgram_is_ready(&dg)) {
        if (!dgram_verify_checksum(&dg)) { // données erronées => jeter paquet
            dgram_del_from_id(dgreceived, dg.id);
            return 1;
        }

        // SEND ACK
        if (dg.request != ACK) {
            if (dgram_create_send(sck, dgsent, NULL, dg.id, ACK, NORMAL, dg.addr, dg.port, 0, NULL) == -1)
                return -1;
        }

        if (callback(&dg, cb_data) == -1)
            return -1;
        // supprimer ce dg
        if (!dgram_del_from_id(dgreceived, dg.id))
            return 1;
    }

    return 0;
}

char * dgram_request_str (const dgram *dg)
{
    switch (dg->request) {
        case ACK:
            return "ACK";
        case PING:
            return "PING";
        case CREQ_AUTH:
            return "CREQ_AUTH";
        case CREQ_READ:
            return "CREQ_READ";
        case CREQ_WRITE:
            return "CREQ_WRITE";
        case CREQ_DELETE:
            return "CREQ_DELETE";
        case CREQ_LOGOUT:
            return "CREQ_LOGOUT";
        case RREQ_READ:
            return "RREQ_READ";
        case RREQ_WRITE:
            return "RREQ_WRITE";
        case RREQ_DELETE:
            return "RREQ_DELETE";
        case RREQ_GETDATA:
            return "RREQ_GETDATA";
        case RREQ_SYNC:
            return "RREQ_SYNC";
        case RREQ_DESTROY:
            return "RREQ_DESTROY";
        case NREQ_LOGOUT:
            return "NREQ_LOGOUT";
        case NREQ_MEET:
            return "NREQ_MEET";
        case NRES_READ:
            return "NRES_READ";
        case NRES_WRITE:
            return "NRES_WRITE";
        case NRES_DELETE:
            return "NRES_DELETE";
        case NRES_GETDATA:
            return "NRES_GETDATA";
        case NRES_SYNC:
            return "NRES_SYNC";
        case RRES_AUTH:
            return "RRES_AUTH";
        case RRES_READ:
            return "RRES_READ";
        case RRES_WRITE:
            return "RRES_WRITE";
        case RRES_DELETE:
            return "RRES_DELETE";
        case RNRES_MEET:
            return "RNRES_MEET";
        case RRES_LOGOUT:
            return "RRES_LOGOUT";
        default:
            return "UNDEFINED";
    }
}

char * dgram_status_str (const dgram *dg)
{
    switch (dg->status) {
        case SUCCESS:
            return "SUCCESS";
        case NORMAL:
            return "NORMAL";
        case ERR_NOREPLY:
            return "ERR_NOREPLY";
        case ERR_AUTHFAILED:
            return "ERR_AUTHFAILED";
        case ERR_NOPERM:
            return "ERR_NOPERM";
        case ERR_NONODE:
            return "ERR_NONODE";
        case ERR_SYNTAX:
            return "ERR_SYNTAX";
        case ERR_UNKNOWFIELD:
            return "ERR_UNKNOWFIELD";
        case ERR_WRITE:
            return "ERR_WRITE";
        case ERR_DELETE:
            return "ERR_DELETE";
        case ERR_NOTAUTH:
            return "ERR_NOTAUTH";
        case ERR_ALREADYAUTH:
            return "ERR_ALREADYAUTH";
        default:
            return "UNDEFINED";
    }
}
