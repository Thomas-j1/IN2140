#include "common.h"

#define BUFSIZE 250

/**
 * @brief data struct for clients
 *
 */
struct client
{
    int port;
    int ip;
    char nick[];
};

int main(int argc, char const *argv[])
{
    int so, rc;
    unsigned short port;
    char buf[BUFSIZE];
    // struct in_addr ipadress;
    struct sockaddr_in my_addr, client_addr;

    if (argc < 3)
    {
        printf("Usage: ./upush_server <port> <loss_probability>\n");
        return EXIT_SUCCESS;
    }

    port = atoi(argv[1]);
    setup_loss_probability(argv[2]);

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    so = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(so, "socket");

    rc = bind(so, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
    check_error(rc, "bind");

    socklen_t socklen = sizeof(struct sockaddr_in);

    while (strcmp(buf, "q\n"))
    {
        rc = recvfrom(so, buf, BUFSIZE - 1, 0, (struct sockaddr *)&client_addr, &socklen);
        check_error(rc, "recvfrom");

        buf[rc] = '\0';
        printf("Received %d bytes: \n%s", rc, buf);
    }

    close(so);
    return EXIT_SUCCESS;
}
