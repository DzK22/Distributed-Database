// Auteurs: Danyl El-Kabir et François Grabenstaetter

#include "../headers/clients.h"

extern clientdata *cdata_global;

int fd_can_read (int fd, void *data)
{
    clientdata *cdata = ((clientdata *) data);
    if (sem_wait(&cdata->gsem) == -1) {
        perror("sem_wait");
        return -1;
    }
    sigset_t mask;
    if (sigemptyset(&mask) == -1) {
        perror("sigemptyset");
        return -1;
    }
    if (sigaddset(&mask, SIGINT) == -1) {
        perror("sigaddset");
        return -1;
    }
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        return -1;
    }

    if (fd == 0) { // stdin
        if (read_stdin(cdata) == -1)
            return -1;
    } else if (fd == cdata->sck) { // sck
        if (dgram_process_raw(cdata->sck, &cdata->dgsent, &cdata->dgreceived, cdata, exec_dg) == -1)
            return -1;
    }
    if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1) {
        perror("sigprocmask");
        return -1;
    }
    if (sem_post(&cdata->gsem) == -1) {
        perror("sem_post");
        return -1;
    }

    return 0;
}

int read_stdin (clientdata *cdata)
{
    char data[SCK_DATAGRAM_MAX];
    errno = 0;

    if ((fgets(data, SCK_DATAGRAM_MAX - 1, stdin) == NULL) && (errno != 0)) {
        perror("gets");
        return -1;
    }

    data[strnlen(data, DG_DATA_MAX) - 1] = '\0';
    size_t data_len = strnlen(data, DG_DATA_MAX);
    char *request_str, *tmp;
    request_str = strtok_r(data, " ", &tmp);
    if (request_str == NULL) {
        print_prompt(cdata);
        return 1;
    }

    if ((strncmp(request_str, "aide", 5) == 0) || (strncmp(request_str, "help", 5) == 0)) {
        print_help();
        print_prompt(cdata);
        return 0;
    } else if (strncmp(request_str, "clear", 6) == 0) {
        printf("\e[1;1H\e[2J");
        print_prompt(cdata);
        return 0;
    }

    uint8_t request;
    size_t instr_len;
    bool arg = false;
    if (strncmp(request_str, "lire", 5) == 0) {
        request = CREQ_READ;
        instr_len = 5;
        arg = true;
    } else if (strncmp(request_str, "ecrire", 7) == 0) {
        request = CREQ_WRITE;
        instr_len = 7;
        arg = true;
    } else if (strncmp(request_str, "supprimer", 10) == 0) {
        request = CREQ_DELETE;
        instr_len = 10;
        arg = false;
    } else if (strncmp(request_str, "bye", 4) == 0) {
        request = CREQ_LOGOUT;
        instr_len = 4;
        arg = false;
    }
    else {
        dgram tmp;
        tmp.status = ERR_SYNTAX;
        dgram_print_status(&tmp);
        print_prompt(cdata);
        return 1;
    }

    if (!cdata->is_auth) {
        if (request == CREQ_LOGOUT)
            return -1;
        dgram tmp;
        tmp.status = ERR_NOTAUTH;
        tmp.request = RRES_AUTH;
        dgram_print_status(&tmp);
        return 1;
    }

    if ((!arg && (data_len > instr_len)) || (arg && (data_len <= instr_len))) {
        dgram tmp;
        tmp.status = ERR_SYNTAX;
        dgram_print_status(&tmp);
        print_prompt(cdata);
        return 1;
    }

    dgram *dg;
    if (dgram_create_send(cdata->sck, &cdata->dgsent, &dg, cdata->id_counter ++, request, NORMAL, cdata->relais_saddr->sin_addr.s_addr, cdata->relais_saddr->sin_port, arg ? data_len : 0, arg ? data + instr_len : NULL) == -1)
        return -1;
    dg->resend_timeout_cb = req_timeout;

    return 0;
}

int exec_dg (const dgram *dg, void *data)
{
    clientdata *cdata = data;
    switch (dg->request) {
        case RRES_AUTH:
            dgram_print_status(dg);
            if (dg->status == SUCCESS)
                cdata->is_auth = true;
            else  // exec, print & quit
                if (ask_auth(cdata) == -1)
                    return -1;
            print_prompt(cdata);
            break;
        case RRES_READ:
            if (dg->status == SUCCESS)
                print_read_res(dg);
            else
                dgram_print_status(dg);
            print_prompt(cdata);
            break;
        case RRES_WRITE:
            dgram_print_status(dg);
            print_prompt(cdata);
            break;
        case RRES_DELETE:
            dgram_print_status(dg);
            print_prompt(cdata);
            break;
        case RRES_LOGOUT:
            dgram_print_status(dg);
            if (dg->status == SUCCESS)
                return -1; // quitter le prog
            break;
        case ACK:
            break;
        case PING:
            break;
        default:
            fprintf(stderr, "Paquet reçu avec requete non gérée par le client: %d\n", dg->request);
    }

    return 0;
}

int send_auth (const char *login, const char *password, clientdata *cdata)
{
    char buf[DG_DATA_MAX];
    int res = snprintf(buf, DG_DATA_MAX, "%s:%s", login, password);
    if (res >= DG_DATA_MAX) {
        fprintf(stderr, "authentificate: snprintf truncate\n");
        return -1;
    } else if (res < 0) {
        perror("snprintf");
        return -1;
    }

    dgram *dg;
    if (dgram_create_send(cdata->sck, &cdata->dgsent, &dg, cdata->id_counter ++, CREQ_AUTH, NORMAL, cdata->relais_saddr->sin_addr.s_addr, cdata->relais_saddr->sin_port, res, buf) == -1)
        return -1;
    dg->resend_timeout_cb = req_timeout;

    return 0;
}

void print_read_res (const dgram *dg)
{
    bool empty = false;
    if (strchr(dg->data, ':') == (dg->data + dg->data_len - 1))
        empty = true;
    if (empty)
      printf(CLEAR YELLOW" %s"RESET RED" aucune donnée"RESET"\n", dg->data);
    else
    {
      char temp[DG_DATA_MAX], *tmp;
      strncpy(temp, dg->data, DG_DATA_MAX);
      char *field = strtok_r(temp, ":", &tmp);
      char *log, *val, *boucle, *test;
      size_t max_pseudo = 0;
      while ((boucle = strtok_r(NULL, ",", &tmp)) != NULL)
      {
        log = strtok_r(boucle, ":", &test);
        if (max_pseudo < strlen(log))
          max_pseudo = strlen(log);
      }

      strncpy(temp, dg->data, DG_DATA_MAX);
      tmp = NULL;
      strtok_r(temp, ":", &tmp);
      bool first_passage = true;
      const size_t field_len = strlen(field);

      while ((boucle = strtok_r(NULL, ",", &tmp)) != NULL)
      {
        log = strtok_r(boucle, ":", &test);
        if (first_passage)
        {
          printf(CLEAR YELLOW " %s" RESET, field);
          for (size_t i = 0; i < 4; i++)
            printf(" ");
          first_passage = false;
        }
        else
        {
          size_t total = 5 + field_len;
          for (size_t i = 0; i < total; i++)
            printf(" ");
        }
        printf(PURPLE"%s" RESET, log);
        size_t marge = max_pseudo - strlen(log) + 3;
        for (size_t i = 0; i < marge; i++)
          printf(" ");
        val = test;
        printf("%s\n", val);
      }
    }
}

void print_prompt (const clientdata *cdata)
{
    if (!cdata->is_auth)
        return;
    printf("\033[33m[%s]\033[0m ", cdata->login);
    if (fflush(stdout) == -1)
        perror("fflush");
}

bool req_timeout (const dgram *dg)
{
    (void) dg;
    dgram tmpdg;
    tmpdg.status = ERR_NOREPLY;
    dgram_print_status(&tmpdg);
    return false;
}

void print_help ()
{
    printf(PURPLE " [AIDE] Requêtes disponibles:" RESET "\n\tlire <champs1>[,...]\n\tecrire <champs1>:<valeur1>[,...]\n\tsupprimer\n\tclear\n\tbye\n");
}

void signal_handler (int sig)
{
    if (sig != SIGINT)
        return;

    if (!cdata_global->is_auth)
        exit(EXIT_FAILURE);

    printf("\n");
    dgram *dg;
    if (dgram_create_send(cdata_global->sck, &cdata_global->dgsent, &dg, cdata_global->id_counter ++, CREQ_LOGOUT, NORMAL, cdata_global->relais_saddr->sin_addr.s_addr, cdata_global->relais_saddr->sin_port, 0, NULL) == -1)
        return;
    dg->resend_timeout_cb = req_timeout;
}

int ask_auth (clientdata *cdata)
{
    // get login and mdp
    char login[LOGIN_MAX];
    char password[LOGIN_MAX];
    size_t password_len = 0;

    printf(BOLD LOGIN_COLOR " Login: " RESET);
    fgets(login, LOGIN_MAX, stdin);
    login[strlen(login) - 1] = '\0';

    bool quit = false;
    char c;
    unsigned i;
    system ("/bin/stty raw");
    errno = 0;

    while (1) {
        printf(CLEAR BOLD LOGIN_COLOR " Password: " RESET);
        for (i = 0; i < password_len; i ++)
            printf("*");

        if (quit)
            break;

        switch (c = getchar()) {
            case '\r':
                quit = true;
                break;
            case '\n':
                quit = true;
                break;
            case 127: // del char
                if (password_len > 0)
                    password[-- password_len] = '\0';
                break;
            case EOF:
                if (errno != 0)
                    perror("getchar");
                return -1;
            default:
                password[password_len] = c;
                password_len ++;
        }

        if (quit || (password_len >= LOGIN_MAX))
            password[password_len] = '\0';
    }

    system ("/bin/stty cooked");
    strncpy(cdata->login, login, LOGIN_MAX);
    printf("\n > Tentative de connexion au serveur ...\n");
    if (send_auth(login, password, cdata) == -1)
        return -1;

    return 0;
}
