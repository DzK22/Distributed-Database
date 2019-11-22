// Auteurs: Danyl El-Kabir et François Grabenstaetter
// Auteurs: Danyl El-Kabir et François Grabenstaetter
#include "../headers/relais.h"

int main (int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: <port>\n");
        return EXIT_FAILURE;
    }

    const char *port = argv[1];

    int sck = sck_create();
    if (sck == -1)
        return EXIT_FAILURE;

    if (sck_bind(sck, atoi(port)) == -1)
        return EXIT_FAILURE;

    privdata *pd = init_privdata();
    relaisdata rdata;
    rdata.sck = sck;
    rdata.dgsent = NULL;
    rdata.dgreceived = NULL;
    rdata.id_counter = 0;
    rdata.pd = pd;

    if (sem_init(&rdata.rsem, 0, 1) == -1) {
        perror("sem_init");
        return EXIT_FAILURE;
    }
    if (sem_init(&rdata.gsem, 0, 1) == -1) {
        perror("sem_init");
        return EXIT_FAILURE;
    }

    // start thread for dgrams
    pthread_t th;
    thread_targ targ;
    targ.sck = rdata.sck;
    targ.dgsent = &rdata.dgsent;
    targ.dgreceived = &rdata.dgreceived;
    targ.gsem = &rdata.gsem;
    if ((errno = pthread_create(&th, NULL, thread_timeout_loop, &targ)) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    // start thread for checking authusers and nodes
    pthread_t rthread;
    if ((errno = pthread_create(&rthread, NULL, rthread_check_loop, &rdata)) != 0) {
        perror("pthread_create");
        return EXIT_FAILURE;
    }

    sck_wait_for_request(sck, 600, false, &rdata, sck_can_read);
    return EXIT_FAILURE;
}
