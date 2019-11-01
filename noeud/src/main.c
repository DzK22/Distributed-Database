#include "../headers/noeud.h"

int main (int argc, char **argv)
{
    if (argc != 4) {
        fprintf(stderr, "Utilisation: %s <relais_addr> <relais_port> <champ stocké> <fichier données>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *relais_addr = argv[1];
    const char *relais_port = argv[2];
    const char *field = argv[3];

    int sck = sck_create();
    if (sck == -1)
        return EXIT_FAILURE;

    if (sck_bind(sck, 0) == -1)
        return EXIT_FAILURE;

    nodedata ndata;
    ndata.sck = sck;
    ndata.field = field;
    ndata.nb_infos = 0;
    ndata.datas = malloc(sizeof(user_info) * 1024);
    if (ndata.datas == NULL)
    {
      perror("malloc error");
      return EXIT_FAILURE;
    }
    ndata.dgsent = NULL;
    ndata.dgreceived = NULL;
    ndata.id_counter = 0;
    ndata.meet_success = false;

    if (sck_create_saddr(&ndata.relais_saddr, relais_addr, relais_port) == -1)
        return EXIT_FAILURE;

    // start thread
    pthread_t th;
    thread_targ targ;
    targ.sck = ndata.sck;
    targ.dgsent = &ndata.dgsent;
    targ.dgreceived = &ndata.dgreceived;
    if ((errno = pthread_create(&th, NULL, thread_timeout_loop, &targ)) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    if (send_meet(&ndata) == -1)
        return EXIT_FAILURE;

    sck_wait_for_request(sck, 1200, false, &ndata, sck_can_read);
    return EXIT_FAILURE;
}
