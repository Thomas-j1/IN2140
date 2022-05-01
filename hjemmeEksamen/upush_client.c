#include "common.h"

#define MAXMSGSIZE 1400
#define HEARTBEAT 10

typedef struct client
{
    struct client *next;
    struct sockaddr_in dest_addr;
    int last_number;
    char nick[];
} client;

client *root;
int packet_number = 0;
char const *my_nick;
char current_nick[MAXNICKSIZE];
char client_packet[MAXBUFSIZE];
char server_packet[BUFSIZE];
struct sockaddr_in last_client_addr, server_addr;

// states
int await_ack = 0;
int await_client = 0;
int await_server = 0;
int retransmit_tries = 0;

// local methods

void reset_states()
{
    await_ack = 0;
    await_client = 0;
    await_server = 0;
    retransmit_tries = 0;
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

void init_root()
{
    root = malloc(sizeof(client));
    check_malloc_error(root);
    root->next = NULL;
    root->nick[0] = 0;
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
    new->last_number = -1;
    new->next = NULL;

    tmp->next = new;
}

// sending methods
void register_with_server(const char *nick, int so, struct sockaddr_in dest_addr)
{
    char buf[BUFSIZE];
    sprintf(buf, "PKT %d REG %s", packet_number++, nick);
    // send_loss_message(so, dest_addr, buf); is supposed to be here but will never register with server
    send_message(so, dest_addr, buf);
    await_ack = -1;
}

void send_client_message(int so, struct sockaddr_in dest_addr)
{
    await_client = 1;
    await_ack = 1; // wait response
    send_loss_message(so, dest_addr, client_packet);
    last_client_addr = dest_addr;
}

void update_client_packet(char number, char *nick, char *msg)
{
    sprintf(client_packet, "PKT %d FROM %s TO %s MSG %s", number, my_nick, nick, msg);
}

void send_server_message(int so)
{
    await_server = 1;
    await_ack = 1; // wait response
    send_loss_message(so, server_addr, server_packet);
}

void update_server_packet(char *nick)
{
    sprintf(server_packet, "PKT %d LOOKUP %s", packet_number++, nick);
}

void send_heartbeat(int so)
{
    sprintf(server_packet, "PKT %d BEAT %s", packet_number++, my_nick);
    send_server_message(so);
}

int resend_last_packet(int so)
{
    if (await_client) // waiting for client state
    {
        if (retransmit_tries == 1) // lookup client again
        {
            update_server_packet(current_nick);
            send_server_message(so);
        }
        else if (retransmit_tries > 1 && await_server) // no response on lookup
        {
            fprintf(stderr, "NICK %s UNREACHABLE\n", current_nick);
            reset_states();
            return 1;
        }
        else if (retransmit_tries > 2) // lost packets after successful lookup
        {
            fprintf(stderr, "NICK %s UNREACHABLE\n", current_nick);
            reset_states();
            return 1;
        }
        else
        {
            send_client_message(so, last_client_addr);
        }
    }
    else // only awaiting server lookup
    {
        if (retransmit_tries >= 2)
        {
            fprintf(stderr, "COULD NOT REACH SERVER AFTER 3 tries\nAborting...\n");
            return 0; // QUIT
        }
        send_server_message(so);
    }
    retransmit_tries++;
    return 1;
}

// handling methods
void handle_pkt(int so, struct sockaddr_in dest_addr, char *type, char *number)
{
    if (strcmp(type, "PKT")) // pkt with message from other client
    {
        return;
    }
    char ack_buf[BUFSIZE];

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

    if (!fNick || !mNick || strlen(msg) < 1) // wrong format
    {
        sprintf(ack_buf, "ACK %s WRONG FORMAT", number);
        send_loss_message(so, dest_addr, ack_buf);
        fprintf(stderr, "Missing PKT data\n");
        return;
    }
    else if (strcmp(mNick, my_nick)) // wrong name
    {
        sprintf(ack_buf, "ACK %s WRONG NAME", number);
        send_loss_message(so, dest_addr, ack_buf);
        return;
    }

    send_ok(so, dest_addr, number);
    client *found = find_client(fNick);
    if (!found) // client unknown
    {
        add_to_clients(dest_addr, fNick);
    }
    else
    {
        // duplicate message?
        int n = atoi(number);
        if (found->last_number != n)
        {
            printf("%s: %s\n", fNick, msg);
            found->last_number = n;
        }
    }
}

void handle_ack(int so, char *type)
{
    if (strcmp(type, "ACK"))
    {
        return;
    }
    // handle ack from number

    char *action = strtok(NULL, " ");
    if (!action)
    {
        fprintf(stderr, "Missing action data\n");
        return;
    }

    if (!strcmp(action, "OK")) // ack ok
    {
        reset_states();
    }
    else if (!strcmp(action, "NOT")) // did not find client
    {
        // send_ok(so, dest_addr, number);
        fprintf(stderr, "NICK %s NOT REGISTERED\n", current_nick);
        reset_states();
    }
    else if (!strcmp(action, "NICK")) // found nickname
    {

        char *nick = strtok(NULL, " ");
        strtok(NULL, " "); // IP
        char *ip = strtok(NULL, " ");
        strtok(NULL, " "); // PORT
        char *port = strtok(NULL, " ");
        if (!nick || !ip || !port)
        {
            fprintf(stderr, "Missing NICK data\n");
            return;
        }
        await_server = 0;
        if (!await_client)
        {
            reset_states();
        }

        struct sockaddr_in current_dest_addr = create_sockaddr(ip, port);

        client *found = find_client(nick);
        if (!found) // client unknown
        {
            add_to_clients(current_dest_addr, nick);
        }
        else
        {
            found->dest_addr = current_dest_addr;
        }
        send_client_message(so, current_dest_addr);
    }
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
    if (DEBUG)
        printf("Received %d bytes: %s\n\n", rc, rbuf);

    char *type = strtok(rbuf, " ");
    char *number = strtok(NULL, " ");

    if (!type || !number)
    {
        fprintf(stderr, "Missing type/number data\n");
        return;
    }
    if (await_ack)
    { // handle ack
        handle_ack(so, type);
    }
    else // handle pkt
    {
        handle_pkt(so, dest_addr, type, number);
    }
}

int handle_stdin(int so)
{
    char buf[MAXBUFSIZE]; // + strlen(nick)
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
        char buf_copy[strlen(buf) + 1];
        char msg[MAXMSGSIZE];
        char *nick;

        strcpy(buf_copy, buf);
        nick = strtok(buf_copy + 1, " ");
        strcpy(current_nick, nick);              // update current_nick
        strcpy(msg, buf + strlen(buf_copy) + 1); // copy rest of message

        client *found = find_client(nick);
        if (found)
        {
            update_client_packet(packet_number++, nick, msg);
            send_client_message(so, found->dest_addr);
        }
        else
        {
            update_server_packet(nick);
            send_server_message(so);
            update_client_packet(packet_number++, nick, msg);
        }
        await_ack = 1; // wait response

        return 1;
    }
}

// main method
int main(int argc, char const *argv[])
{
    int so, rc, main_event_loop;
    char buf[BUFSIZE];
    int timeout;
    char const *nick, *port, *server_ip;
    fd_set my_set;
    struct sockaddr_in my_addr;
    struct timeval tv;
    time_t last_beat, curr_time;

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
    timeout = atoi(argv[4]);
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
    printf("Messaging service usage:<@user> <msg>\n\n");

    last_beat = time(NULL);
    memset(buf, 0, BUFSIZE);
    main_event_loop = 1;
    while (main_event_loop)
    {
        tv.tv_sec = timeout;
        tv.tv_usec = 0;
        if (!await_ack && !await_client) // block reading from stdin
        {
            FD_SET(STDIN_FILENO, &my_set);
        }
        else
        {
            FD_CLR(STDIN_FILENO, &my_set);
        }
        FD_SET(so, &my_set);
        rc = select(FD_SETSIZE, &my_set, NULL, NULL, &tv);
        check_error(rc, "select");
        if (rc == 0) // fds are empty after timeout
        {
            if (await_ack < 0) // could not register with server
            {
                fprintf(stderr, "Could not register with server\nABORTING...\n");
                main_event_loop = 0;
            }
            else if (await_ack) // resend
            {
                main_event_loop = resend_last_packet(so);
            }
        }
        else if (FD_ISSET(STDIN_FILENO, &my_set) && !await_ack && !await_client)
        {
            main_event_loop = handle_stdin(so);
        }
        else if (FD_ISSET(so, &my_set))
        {
            handle_socket(so);
        }
        curr_time = time(NULL);
        if (curr_time - last_beat > HEARTBEAT)
        {
            send_heartbeat(so);
            last_beat = time(NULL);
        }
    }

    close(so);
    free_clients();

    return EXIT_SUCCESS;
}