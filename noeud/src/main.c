#include "../headers/noeud.h"

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "Utilisation: %s <relais_ip> <relais_port> <champ stocké> <fichier données>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *relais_ip = argv[1];
    const char *relais_port = argv[2];
    const char *field = argv[3];
    const char *datafile = argv[4];

    int sock = create_socket();
    if (sock == -1)
        return EXIT_FAILURE;

    node_data ndata;
    if (fill_node_data(sock, field, datafile, relais_ip, relais_port, &ndata) == -1)
        return EXIT_FAILURE;

    if (can_bind(sock) == -1)
        return EXIT_FAILURE;

    if (remember_relais_addr(&ndata) == -1)
        return EXIT_FAILURE;

    if (meet_relais(&ndata) == -1)
        return EXIT_FAILURE;

    wait_for_request(&ndata);

    return EXIT_FAILURE;
}
