#include "common.h"

#define MAXMSGSIZE 1400

typedef struct client
{
    struct client *next;
    struct sockaddr_in dest_addr;
    int lastnumber;
    char nick[];
} client;

client *root;
int packetNumber = 0;
struct sockaddr_in server_addr;
char const *my_nick;
char my_msg[MAXMSGSIZE];

void register_with_server(const char *nick, int so, struct sockaddr_in dest_addr)
{
    char buf[BUFSIZE];

    sprintf(buf, "PKT %d REG %s", packetNumber++, nick);

    send_message(so, dest_addr, buf);
}

void free_clients()
{
    client *tmp, *tmp2;
    tmp = root;
    while (tmp)
    {
        tmp2 = tmp;
        tmp = tmp->next;
        free(tmp2);
    }
}

client *find_client(char *nick)
{
    client *tmp = root;

    while (tmp)
    {
        if (!strcmp(tmp->nick, nick))
        {
            return tmp;
        }
        tmp = tmp->next;
    }

    return NULL;
}

void add_to_clients(struct sockaddr_in dest_addr, char *nick)
{
    client *tmp = root;

    while (tmp->next)
    {
        tmp = tmp->next;
    }
    // tmp is last in clients

    client *new = malloc(sizeof(client) + strlen(nick) + 1);
    check_malloc_error(new);
    new->dest_addr = dest_addr;
    strcpy(new->nick, nick);
    new->next = NULL;

    tmp->next = new;
}

void send_client_message(int so, struct sockaddr_in dest_addr, char packetNumber, char *nick, char *msg)
{
    char sbuf[MAXBUFSIZE];

    sprintf(sbuf, "PKT %d FROM %s TO %s MSG %s", packetNumber, my_nick, nick, msg);
    send_message(so, dest_addr, sbuf);
}

struct sockaddr_in create_sockaddr(char *ip, char *port)
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

    return addr;
}

void handle_socket(int so)
{
    int rc;
    char rbuf[MAXBUFSIZE];
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
            send_ok(so, dest_addr, number);
        }
        else if (!strcmp(action, "NICK")) // found nickname
        {
            send_ok(so, dest_addr, number);

            char *nick = strtok(NULL, " ");
            strtok(NULL, " "); // IP
            char *ip = strtok(NULL, " ");
            strtok(NULL, " "); // PORT
            char *port = strtok(NULL, " ");

            struct sockaddr_in current_dest_addr = create_sockaddr(ip, port);
            add_to_clients(current_dest_addr, nick);

            send_client_message(so, current_dest_addr, packetNumber, nick, my_msg);
        }
    }
    else if (!strcmp(type, "PKT"))
    {
        // handle PKT
        strtok(NULL, " "); // FROM
        char *fNick = strtok(NULL, " ");
        strtok(NULL, " "); // TO
        // char *mNick = strtok(NULL, " ");
        strtok(NULL, " "); // mNick
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

        send_ok(so, dest_addr, number);
        add_to_clients(dest_addr, fNick);

        printf("%s: %s\n", fNick, msg);
    }
}

int handle_stdin(int so)
{
    char buf[MAXBUFSIZE], lookBuf[BUFSIZE]; // + strlen(nick)
    // char c;
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
        strcpy(my_msg, buf + strlen(mBuf) + 1); // copy rest of message

        client *found = find_client(nick);

        if (found)
        {
            send_client_message(so, found->dest_addr, packetNumber, nick, my_msg);
        }
        else
        {
            sprintf(lookBuf, "PKT %d LOOKUP %s", packetNumber++, nick);
            send_message(so, server_addr, lookBuf);
        }

        return 1;
    }
}

void init_root()
{
    root = malloc(sizeof(client));
    check_malloc_error(root);
    root->next = NULL;
    root->nick[0] = 0;
}

int main(int argc, char const *argv[])
{
    int so, rc;
    char buf[BUFSIZE];
    char const *nick, *port, *server_ip, *timeout;
    fd_set my_set;
    struct sockaddr_in my_addr;

    init_root();

    if (argc < 6)
    {
        printf("Usage: ./upush_client <nick> <ip-address> <port> <timeout> <loss_probability>\n");
        return EXIT_SUCCESS;
    }

    nick = argv[1];
    my_nick = nick;
    server_ip = argv[2];
    port = argv[3];
    timeout = argv[4];
    setup_loss_probability(argv[5]);

    so = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(so, "socket");

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(0);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    server_addr = create_sockaddr((char *)server_ip, (char *)port);

    rc = bind(so, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
    check_error(rc, "bind");

    register_with_server(nick, so, server_addr);

    FD_ZERO(&my_set);
    printf("Welcome to msn. Write QUIT to leave\n\n");

    memset(buf, 0, BUFSIZE);
    int main_event_loop = 1;
    while (main_event_loop)
    {
        FD_SET(STDIN_FILENO, &my_set);
        FD_SET(so, &my_set);
        rc = select(FD_SETSIZE, &my_set, NULL, NULL, NULL);
        check_error(rc, "select");

        if (FD_ISSET(STDIN_FILENO, &my_set))
        {
            main_event_loop = handle_stdin(so);
        }
        else if (FD_ISSET(so, &my_set))
        {
            handle_socket(so);
        }
    }

    close(so);
    free_clients();

    return EXIT_SUCCESS;
}