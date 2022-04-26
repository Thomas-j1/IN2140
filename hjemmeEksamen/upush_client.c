#include "common.h"

#define MAXMSGSIZE 1400

int packetNumber = 0;
struct sockaddr_in current_dest_addr, server_addr;
char const *my_nick;
char currText[MAXMSGSIZE];

void register_with_server(const char *nick, int so, struct sockaddr_in dest_addr)
{
    char buf[BUFSIZE];

    sprintf(buf, "PKT %d REG %s", packetNumber++, nick);

    send_message(so, dest_addr, buf);
}

void create_sock_addr(char *ip, char *port)
{
    struct sockaddr_in addr;
    struct in_addr ip_addr;
    int rc;

    rc = inet_pton(AF_INET, ip, &ip_addr);
    check_error(rc, "inet_pton");
    if (rc == 0)
    {
        fprintf(stderr, "IP address not valid\n");
        exit(EXIT_FAILURE);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr = ip_addr;

    current_dest_addr = addr;
}

void handle_socket(int so)
{
    int rc;
    char rbuf[MAXBUFSIZE], sbuf[MAXBUFSIZE], ackBuf[BUFSIZE];
    struct sockaddr_in dest_addr;
    socklen_t socklen = sizeof(struct sockaddr_in);

    rc = recvfrom(so, rbuf, MAXBUFSIZE - 1, 0, (struct sockaddr *)&dest_addr, &socklen);
    check_error(rc, "recvfrom");

    rbuf[rc] = '\0';
    rbuf[strcspn(rbuf, "\n")] = 0;
    printf("Received %d bytes: %s\n", rc, rbuf);

    char *type = strtok(rbuf, " ");
    char *number = strtok(NULL, " ");

    if (type == NULL || number == NULL)
    {
        send_message(so, dest_addr, "WRONG FORMAT");
    }
    else if (!strcmp(type, "ACK"))
    {
        // handle ack from number
        char *action = strtok(NULL, " ");

        if (!strcmp(action, "OK")) // handle number from who
        {
        }
        else if (!strcmp(action, "NOT")) // did not find client
        {
        }
        else if (!strcmp(action, "NICK")) // found nickname
        {
            char *nick = strtok(NULL, " ");
            strtok(NULL, " "); // IP
            char *ip = strtok(NULL, " ");
            strtok(NULL, " "); // PORT
            char *port = strtok(NULL, " ");

            create_sock_addr(ip, port);
            sprintf(sbuf, "PKT %d FROM %s TO %s MSG %s", packetNumber, my_nick, nick, currText);
            send_message(so, current_dest_addr, sbuf);
        }
    }
    else if (!strcmp(type, "PKT"))
    {
        // handle PKT
        strtok(NULL, " "); // FROM
        char *fNick = strtok(NULL, " ");
        strtok(NULL, " "); // TO
        char *mNick = strtok(NULL, " ");
        strtok(NULL, " "); // MSG
        char msg[MAXMSGSIZE], *msgToken;
        msgToken = strtok(NULL, " ");
        memset(msg, 0, sizeof(msg));
        while (msgToken != NULL)
        {
            sprintf(msg + strlen(msg), "%s ", msgToken);
            msgToken = strtok(NULL, " ");
        }
        msg[strlen(msg) - 1] = 0; // remove trailing whitespace

        sprintf(ackBuf, "ACK %s OK", number);
        send_message(so, dest_addr, ackBuf);

        printf("%s: %s\n", fNick, msg);
    }
}

int handle_stdin(int so)
{
    char buf[MAXBUFSIZE], lookBuf[BUFSIZE]; // + strlen(nick)
    char c;
    fgets(buf, MAXBUFSIZE, stdin);
    buf[strcspn(buf, "\n")] = 0; // if /n overwrite with 0
    // while ((c = getchar()) != '\n' && c != EOF)
    //   ; // remove excess buf

    if (!strcmp(buf, "QUIT"))
    {
        return 0;
    }
    else // look for nick and send message
    {
        char mBuf[strlen(buf) + 1];
        strcpy(mBuf, buf);

        char *nick = strtok(mBuf + 1, " ");
        strcpy(currText, buf + strlen(mBuf) + 1); // copy rest of message

        sprintf(lookBuf, "PKT %d LOOKUP %s", packetNumber++, nick);

        send_message(so, server_addr, lookBuf);

        return 1;
    }
}

int main(int argc, char const *argv[])
{
    int so, rc;
    unsigned short port;
    char buf[BUFSIZE];
    char const *nick, *server_ip, *timeout;
    fd_set set;
    struct in_addr ip_addr;
    struct sockaddr_in my_addr;

    if (argc < 6)
    {
        printf("Usage: ./upush_client <nick> <ip-address> <port> <timeout> <loss_probability>\n");
        return EXIT_SUCCESS;
    }

    nick = argv[1];
    my_nick = nick;
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

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr = ip_addr;

    rc = bind(so, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
    check_error(rc, "bind");

    register_with_server(nick, so, server_addr);

    FD_ZERO(&set);
    printf("Welcome to msn. Write QUIT to leave\n\n");

    memset(buf, 0, BUFSIZE);
    int main_event_loop = 1;
    while (main_event_loop)
    {
        FD_SET(STDIN_FILENO, &set);
        FD_SET(so, &set);
        rc = select(FD_SETSIZE, &set, NULL, NULL, NULL);
        check_error(rc, "select");

        if (FD_ISSET(STDIN_FILENO, &set))
        {
            main_event_loop = handle_stdin(so);
        }
        else if (FD_ISSET(so, &set))
        {
            handle_socket(so);
        }
    }

    close(so);

    return EXIT_SUCCESS;
}