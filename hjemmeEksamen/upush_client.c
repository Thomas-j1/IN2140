#include "common.h"

#define MAXMSGSIZE 1400

int packetNumber = 0;

void register_with_server(const char *nick, int so, struct sockaddr_in dest_addr)
{
    char buf[BUFSIZE];
    int rc;

    sprintf(buf, "PKT %d REG %s", packetNumber++, nick);

    rc = sendto(so,
                buf,
                strlen(buf),
                0,
                (struct sockaddr *)&dest_addr,
                sizeof(struct sockaddr_in));
    check_error(rc, "sendto");
}

void handle_stdin(int so, struct sockaddr_in dest_addr)
{
    char buf[MAXMSGSIZE]; // + strlen(nick)
    int rc;
    fgets(buf, BUFSIZE, stdin);
    buf[strcspn(buf, "\n")] = 0; // if /n overwrite with 0
    rc = sendto(so, buf, strlen(buf), 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
    check_error(rc, "sendto");
}

int main(int argc, char const *argv[])
{
    int so, rc;
    unsigned short port;
    char buf[BUFSIZE];
    char const *nick, *server_ip, *timeout;
    fd_set set;
    struct in_addr ip_addr;
    struct sockaddr_in my_addr, dest_addr;

    if (argc < 6)
    {
        printf("Usage: ./upush_client <nick> <ip-address> <port> <timeout> <loss_probability>\n");
        return EXIT_SUCCESS;
    }

    nick = argv[1];
    server_ip = argv[2];
    port = atoi(argv[3]);
    timeout = argv[4];
    setup_loss_probability(argv[5]);

    so = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(so, "socket");

    rc = inet_pton(AF_INET, server_ip, &ip_addr);
    check_error(rc, "inet_pton");
    if (rc == 0)
    {
        fprintf(stderr, "IP address not valid\n");
        return EXIT_FAILURE;
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(0); // os assigned port
    my_addr.sin_addr = ip_addr;

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    dest_addr.sin_addr = ip_addr;

    rc = bind(so, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
    check_error(rc, "bind");

    register_with_server(nick, so, dest_addr);

    FD_ZERO(&set);
    printf("Welcome to msn. Write quit to leave\n\n");

    memset(buf, 0, BUFSIZE);
    while (strcmp(buf, "q"))
    {
        FD_SET(STDIN_FILENO, &set);
        FD_SET(so, &set);
        rc = select(FD_SETSIZE, &set, NULL, NULL, NULL);
        check_error(rc, "select");

        if (FD_ISSET(STDIN_FILENO, &set))
        {
            fgets(buf, BUFSIZE, stdin);
            buf[strcspn(buf, "\n")] = 0; // if /n overwrite with 0
            rc = sendto(so, buf, strlen(buf), 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr_in));
            check_error(rc, "sendto");
        }
        else if (FD_ISSET(so, &set))
        {
            // handle_socket();
            rc = read(so, buf, BUFSIZE - 1);
            check_error(rc, "read");
            buf[rc] = '\0';
            printf("Server: %s\n", buf);
        }
    }

    close(so);

    return EXIT_SUCCESS;
}