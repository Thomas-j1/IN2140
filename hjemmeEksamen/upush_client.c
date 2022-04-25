#include "common.h"

int main(int argc, char const *argv[])
{
    int so, rc;
    unsigned short port;
    char buf[BUFSIZE];
    fd_set set;
    struct in_addr ip_addr;
    struct sockaddr_in my_addr, dest_addr;

    if (argc < 2)
    {
        printf("Usage: ./upush_client <port>\n");
        return EXIT_SUCCESS;
    }

    port = atoi(argv[1]);

    so = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(so, "socket");

    rc = inet_pton(AF_INET, "127.0.0.1", &ip_addr);
    check_error(rc, "inet_pton");
    if (rc == 0)
    {
        fprintf(stderr, "IP address not valid\n");
        return EXIT_FAILURE;
    }

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr = ip_addr;

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(4321);
    dest_addr.sin_addr = ip_addr;

    rc = bind(so, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
    check_error(rc, "bind");

    strcpy(buf, argv[2]);
    rc = sendto(so,
                buf,
                strlen(buf),
                0,
                (struct sockaddr *)&dest_addr,
                sizeof(struct sockaddr_in));
    check_error(rc, "sendto");

    FD_ZERO(&set);
    printf("Welcome to msn. Write quit to leave\n\n");

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
            rc = read(so, buf, BUFSIZE - 1);
            check_error(rc, "read");
            buf[rc] = '\0';
            printf("Server: %s\n", buf);
        }
    }

    close(so);

    return EXIT_SUCCESS;
}