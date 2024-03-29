#include "common.h"

#define MAXMSGSIZE 1400 // 1400
#define HEARTBEAT 10    // 10

typedef struct client
{
    struct client *next;
    struct sockaddr_in dest_addr;
    int last_number;
    int my_last_number;
    int blocked;
    char nick[];
} client;

struct msg_que
{
    char *msg;
    int tries;
    int is_lookup;
    int in_flight;
    struct sockaddr_in dest_addr;
    struct msg_que *next;
    char nick[];
};

client *client_root;
struct msg_que *que_root;
int server_number = 0;
char const *my_nick;
char current_lookup_nick[MAXNICKSIZE];
int main_event_loop = 1;

struct sockaddr_in server_addr;

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

struct sockaddr_in empty_sockaddr()
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = INADDR_ANY;
    return addr;
}

/**
 * @brief check if nick is in valid format.
 * Exit program with error msg if invalid nick
 */
void validate_nick(char const *nick)
{
    int invalid = 0;
    if (strlen(nick) > MAXNICKSIZE)
    {
        invalid = 1;
    }
    for (size_t i = 0; i < strlen(nick); i++)
    {
        char curr_char = nick[i];
        if (isspace(curr_char) || !isascii(curr_char))
        {
            invalid = 1;
        }
    }
    if (invalid)
    {
        fprintf(stderr, "Nick is not valid\n");
        exit(EXIT_FAILURE);
    }
}

/**
 * @return 0 if msg valid else 1
 */
int msg_not_valid(char *msg)
{
    int invalid = 0;

    if (strlen(msg) > MAXMSGSIZE)
    {
        invalid = 1;
    }
    for (size_t i = 0; i < strlen(msg); i++)
    {
        char curr_char = msg[i];
        if (!isascii(curr_char))
        {
            invalid = 1;
        }
    }
    return invalid;
}

void free_clients()
{
    client *tmp, *tmp2;
    tmp = client_root;
    while (tmp)
    {
        tmp2 = tmp;
        tmp = tmp->next;
        free(tmp2);
    }
}

void free_que()
{
    struct msg_que *tmp, *tmp2;
    tmp = que_root;
    tmp2 = tmp;
    tmp = tmp->next;
    free(tmp2);
    while (tmp)
    {
        tmp2 = tmp;
        tmp = tmp->next;
        free(tmp2->msg);
        free(tmp2);
    }
}

void free_all()
{
    free_clients();
    free_que();
}

void init_roots()
{
    client_root = malloc(sizeof(client) + 1); // + 0 byte for name
    check_malloc_error(client_root);
    client_root->next = NULL;
    client_root->blocked = 1;
    client_root->nick[0] = 0;

    que_root = malloc(sizeof(struct msg_que) + 1);
    check_malloc_error(que_root);
    que_root->nick[0] = 0;
    que_root->next = NULL;
}

/**
 * @brief find and return pointer to client
 * or NULL if client not found
 */
client *find_client(char *nick)
{
    client *tmp = client_root;

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

/**
 * @brief Get the outgoing server packet number
 * and update it
 */
int get_server_number()
{
    if (server_number)
    {
        server_number = 0;
    }
    else
    {
        server_number = 1;
    }
    return server_number;
}

/**
 * @brief Get the outgoing client packet number and update it,
 * if client is unknown checks msg_que for other messages to client
 * & returns opposite of last packet number
 */
int get_client_number(char *nick)
{
    client *found = find_client(nick);
    if (found)
    {
        if (found->my_last_number)
        {
            found->my_last_number = 0;
        }
        else
        {
            found->my_last_number = 1;
        }
        return found->my_last_number;
    }
    else
    { // client not known,check if previus messages to same client
        struct msg_que *tmp = que_root;
        tmp = tmp->next;
        int number = 0;
        while (tmp)
        {
            if (!strcmp(tmp->nick, nick) && !tmp->is_lookup)
            {
                if (number)
                {
                    number = 0;
                }
                else
                {
                    number = 1;
                }
            }
            tmp = tmp->next;
        }
        return number;
    }
}

/**
 * @brief Set block value for client with nick
 */
void set_client_block(char *nick, int value)
{
    client *found = find_client(nick);
    if (found)
    {
        found->blocked = value;
        if (DEBUG)
            printf("set client %s blocked to %d\n", nick, value);
    }
    else
    {
        fprintf(stderr, "Error unknown client\n");
    }
}

void add_to_clients(struct sockaddr_in dest_addr, char *nick, int number)
{
    client *tmp = client_root;

    while (tmp->next)
    {
        tmp = tmp->next;
    }
    // tmp is last in clients

    client *new = malloc(sizeof(client) + strlen(nick) + 1);
    check_malloc_error(new);
    new->dest_addr = dest_addr;
    strcpy(new->nick, nick);
    new->last_number = number;
    new->my_last_number = !get_client_number(nick); // get opposite of last packet
    new->blocked = 0;
    new->next = NULL;

    tmp->next = new;
}

void register_with_server(const char *nick, int so, struct sockaddr_in dest_addr)
{
    char buf[BUFSIZE];
    validate_nick(nick);
    sprintf(buf, "PKT %d REG %s", 0, nick);
    send_loss_message(so, dest_addr, buf);
    main_event_loop = -1;
}

void send_heartbeat(int so)
{
    char server_packet[BUFSIZE];
    sprintf(server_packet, "PKT %d BEAT %s", get_server_number(), my_nick);
    send_loss_message(so, server_addr, server_packet);
}

/**
 * @brief check if there is allready a msg packet to "nick" in flight
 */
int nick_in_flight(char *nick)
{
    struct msg_que *tmp = que_root;
    tmp = tmp->next;

    while (tmp)
    {
        if (!strcmp(tmp->nick, nick) && !tmp->is_lookup && tmp->in_flight)
        {
            return 1;
        }
        tmp = tmp->next;
    }

    return 0;
}

/**
 * @brief check if nick is allready in lookup que
 */
int nick_in_lookup(char *nick)
{
    struct msg_que *tmp = que_root;
    tmp = tmp->next;

    while (tmp)
    {
        if (!strcmp(tmp->nick, nick) && (tmp->is_lookup))
        {
            return 1;
        }
        tmp = tmp->next;
    }

    return 0;
}

/**
 * @brief check if there are any lookup messages in flight except for this nick
 */
int lookup_msg_in_flight(char *exception)
{
    struct msg_que *tmp = que_root;
    tmp = tmp->next;

    while (tmp)
    {
        if (tmp->is_lookup && tmp->in_flight && strcmp(exception, tmp->nick))
        {
            return 1;
        }
        tmp = tmp->next;
    }

    return 0;
}

struct msg_que *add_to_que(char *nick, char *msg, struct sockaddr_in dest_addr, int is_lookup)
{
    struct msg_que *tmp = que_root;

    while (tmp->next)
    {
        tmp = tmp->next;
    }
    // tmp is last in clients

    struct msg_que *new = malloc(sizeof(struct msg_que) + strlen(nick) + 1);
    check_malloc_error(new);
    new->dest_addr = dest_addr;
    strcpy(new->nick, nick);
    new->is_lookup = is_lookup;
    new->in_flight = 0;
    new->tries = 0;
    new->msg = strdup(msg);
    check_malloc_error(new->msg);
    new->next = NULL;

    tmp->next = new;
    return new;
}

void remove_from_que(char *nick, int is_lookup, int in_flight)
{
    struct msg_que *tmp = que_root;
    struct msg_que *last = tmp;
    tmp = tmp->next;
    while (tmp)
    {
        if (!strcmp(tmp->nick, nick) &&
            (tmp->is_lookup == is_lookup) &&
            (tmp->in_flight = in_flight))
        {
            last->next = tmp->next;
            free(tmp->msg);
            free(tmp);
            break;
        }
        else
        {
            last = tmp;
            tmp = tmp->next;
        }
    }
}

void remove_from_que_with_addr(struct sockaddr_in from_addr, int is_lookup, int in_flight)
{
    struct msg_que *tmp = que_root;
    struct msg_que *last = tmp;
    tmp = tmp->next;
    while (tmp)
    {
        if (tmp->dest_addr.sin_addr.s_addr == from_addr.sin_addr.s_addr &&
            tmp->dest_addr.sin_port == from_addr.sin_port &&
            tmp->is_lookup == is_lookup &&
            tmp->in_flight == in_flight)
        {
            last->next = tmp->next;
            free(tmp->msg);
            free(tmp);
            break;
        }
        else
        {
            last = tmp;
            tmp = tmp->next;
        }
    }
}

/**
 * @brief add lookup msg for nick in msg que
 */
struct msg_que *add_server_lookup_packet(char *nick)
{
    char server_packet[BUFSIZE];
    memset(server_packet, 0, BUFSIZE);
    sprintf(server_packet, "PKT %d LOOKUP %s", get_server_number(), nick);
    struct msg_que *packet = add_to_que(nick, server_packet, server_addr, 1);
    return packet;
}

/**
 * @brief add client msg for nick to que with dest_addr
 */
struct msg_que *add_client_message(char *nick, char *msg, struct sockaddr_in dest_addr)
{
    char client_packet[MAXBUFSIZE];
    memset(client_packet, 0, MAXBUFSIZE);
    sprintf(client_packet, "PKT %d FROM %s TO %s MSG %s",
            get_client_number(nick),
            my_nick, nick, msg);

    struct msg_que *packet = add_to_que(nick, client_packet, dest_addr, 0);
    return packet;
}

/**
 * @brief overwrite dest_addr for all messages in que to nick
 */
void set_msg_dest_addr(char *nick, struct sockaddr_in dest_addr)
{
    struct msg_que *tmp = que_root;
    tmp = tmp->next;
    while (tmp)
    {
        if (!strcmp(tmp->nick, nick) && !tmp->is_lookup)
        {
            tmp->dest_addr = dest_addr;
        }
        tmp = tmp->next;
    }
}

/**
 * @brief handle packet in flight
 * @returns next msg in que
 */
struct msg_que *send_client_pkt(int so, struct msg_que *msg, char *nick)
{
    struct msg_que *next = msg->next;
    if (msg->is_lookup)
    {
        if (msg->tries >= 3 && msg->tries < 6) // failed lookup 3 times
        {
            fprintf(stderr, "COULD NOT REACH SERVER AFTER 3 tries\nAborting...\n");
            main_event_loop = 0;
            return 0; // QUIT
        }
        else if (msg->tries == 7) // no response on lookup after 2 failed messages
        {
            fprintf(stderr, "NICK %s UNREACHABLE: No response from server lookup\n",
                    msg->nick);
            // remove this & pkt to nick from msg que
            remove_from_que(msg->nick, 0, 1); // msg pkt
            remove_from_que(msg->nick, 1, 1); // lookup pkt
        }
        else
        {
            send_loss_message(so, server_addr, msg->msg);
            strcpy(current_lookup_nick, nick); // update current_lookup_nick
            msg->tries += 1;
        }
    }
    else
    {
        if (msg->tries == 2) // lookup client again
        {
            struct msg_que *packet = add_server_lookup_packet(msg->nick);
            packet->tries = 6; // set lookup packet tries to 6
            msg->tries++;
        }
        else if (msg->tries > 4) // lost packets after successful lookup
        {
            fprintf(stderr, "NICK %s UNREACHABLE\n", msg->nick);
            // remove this from que
            remove_from_que(msg->nick, 0, 1);
        }
        else // resend last client packet
        {
            send_loss_message(so, msg->dest_addr, msg->msg);
            msg->tries++;
        }
    }
    return next;
}

/**
 * @brief go through message que, and add messages to flight
 */
void add_to_flight(int so)
{
    struct msg_que *tmp = que_root;
    tmp = tmp->next;

    while (tmp)
    {
        if (tmp->is_lookup)
        {
            if (!lookup_msg_in_flight(tmp->nick)) // 1 lookup at the time
            {
                tmp->in_flight = 1;
                tmp = send_client_pkt(so, tmp, tmp->nick);
            }
            else
            {
                tmp = tmp->next;
            }
        }
        else if ((tmp->in_flight || !nick_in_flight(tmp->nick)) && !nick_in_lookup(tmp->nick))
        // only 1 msg in flight per client & not sending if waiting for lookup
        {
            // add to msg_que
            client *found = find_client(tmp->nick);

            if (found)
            {
                tmp->in_flight = 1;
                tmp = send_client_pkt(so, tmp, tmp->nick);
            }
            else // skip until succesfull lookup or package thrown after failed lookup
            {
                tmp = tmp->next;
            }
        }
        else
        {
            tmp = tmp->next;
        }
    }
}

/**
 * @brief handle incoming PKT packet @socket
 * with msg from client
 */
void handle_pkt(int so, struct sockaddr_in dest_addr, char *number)
{
    char ack_buf[BUFSIZE];

    // handle PKT
    strtok(NULL, " "); // FROM
    char *pkt_nick = strtok(NULL, " ");
    strtok(NULL, " "); // TO
    char *dest_nick = strtok(NULL, " ");
    strtok(NULL, " "); // MSG
    char msg[MAXMSGSIZE], *msg_token;
    msg_token = strtok(NULL, " ");
    memset(msg, 0, sizeof(msg));
    while (msg_token != NULL) // add rest of buff to msg
    {
        sprintf(msg + strlen(msg), "%s ", msg_token);
        msg_token = strtok(NULL, " ");
    }
    msg[strlen(msg) - 1] = 0; // remove trailing whitespace

    if (!pkt_nick || !dest_nick || strlen(msg) < 1 || msg_not_valid(msg)) // wrong format
    {
        sprintf(ack_buf, "ACK %s WRONG FORMAT", number);
        send_loss_message(so, dest_addr, ack_buf);
        return;
    }
    else if (strcmp(dest_nick, my_nick)) // wrong name
    {
        sprintf(ack_buf, "ACK %s WRONG NAME", number);
        send_loss_message(so, dest_addr, ack_buf);
        return;
    }

    send_ok(so, dest_addr, number); // send ack ok response
    client *found = find_client(pkt_nick);
    if (!found) // client unknown
    {
        add_to_clients(dest_addr, pkt_nick, atoi(number));
        if (DEBUG) // print with color to easier see with debug
        {
            printf(CYN "%s: %s" RESET "\n", pkt_nick, msg);
        }
        else
        {
            printf("%s: %s\n", pkt_nick, msg);
        }
    }
    else
    {
        // duplicate message?
        int n = atoi(number);
        if (found->last_number != n && !found->blocked) // not duplicate msg or blocked client
        {
            if (DEBUG) // print with color to easier see with debug
            {
                printf(CYN "%s: %s" RESET "\n", pkt_nick, msg);
            }
            else
            {
                printf("%s: %s\n", pkt_nick, msg);
            }
        }
        found->last_number = n;
    }
}

/**
 * @brief handle incoming ACK packet @socket,
 */
void handle_ack(struct sockaddr_in from_addr)
{
    // handle ack from number

    char *action = strtok(NULL, " ");
    if (!action)
    {
        fprintf(stderr, "Missing action data\n");
        return;
    }

    if (!strcmp(action, "OK")) // ack ok
    {
        main_event_loop = 1;                        // for register msg
        remove_from_que_with_addr(from_addr, 0, 1); // remove corresponding packet from que
    }
    else if (!strcmp(action, "NOT")) // did not find client
    {
        fprintf(stderr, "NICK %s NOT REGISTERED\n", current_lookup_nick);
        remove_from_que(current_lookup_nick, 0, 0); // remove nick if in que
        remove_from_que(current_lookup_nick, 0, 1); // remove nick if in flight
        remove_from_que(current_lookup_nick, 1, 1); // remove nick from lookup
    }
    else if (!strcmp(action, "NICK")) // found nickname
    {

        char *nick = strtok(NULL, " ");
        strtok(NULL, " "); // IP
        char *ip = strtok(NULL, " ");
        strtok(NULL, " "); // PORT
        char *port = strtok(NULL, " ");
        if (!nick || !ip || !port) // missing data
        {
            fprintf(stderr, "Missing NICK data\n");
            return;
        }

        remove_from_que(nick, 1, 1);

        struct sockaddr_in current_dest_addr = create_sockaddr(ip, port);
        set_msg_dest_addr(nick, current_dest_addr);

        client *found = find_client(nick);
        if (!found) // client unknown
        {
            add_to_clients(current_dest_addr, nick, -1);
        }
        else // overwrite existing client dest_addr
        {
            found->dest_addr = current_dest_addr;
        }
    }
}

/**
 * @brief read incoming packet @socket
 * and handle packet
 */
void handle_socket(int so)
{
    int rc;
    char buf[MAXBUFSIZE];
    struct sockaddr_in dest_addr;
    socklen_t socklen = sizeof(struct sockaddr_in);

    rc = recvfrom(so, buf, MAXBUFSIZE - 1, 0, (struct sockaddr *)&dest_addr, &socklen);
    check_error(rc, "recvfrom");

    buf[rc] = '\0';
    buf[strcspn(buf, "\n")] = 0;
    if (DEBUG)
        printf(GRN "Received %d bytes: %s\n" RESET "\n", rc, buf);

    char *type = strtok(buf, " ");
    char *number = strtok(NULL, " ");

    if (!type || !number || (strcmp(type, "PKT") && strcmp(type, "ACK")))
    {
        fprintf(stderr, "Recevied packet without type/number, throwing packet\n");
        return;
    }
    if (!strcmp(type, "ACK")) // waiting for response to outgoing packet
    {                         // handle ack
        handle_ack(dest_addr);
    }
    else if (!strcmp(type, "PKT")) // handle pkt with msg
    {
        handle_pkt(so, dest_addr, number);
    }
}

/**
 * @brief handle msg to client from stdin
 */
void handle_stdin_msg(char *buf)
{
    char buf_copy[strlen(buf) + 1];
    char msg[MAXMSGSIZE];
    char *nick;

    strcpy(buf_copy, buf);
    nick = strtok(buf_copy + 1, " "); // +1 removes @
    memset(msg, 0, MAXMSGSIZE);
    memcpy(msg, (buf + strlen(buf_copy) + 1), MAXMSGSIZE - 1); // ignore rest after 1400 bytes

    client *found = find_client(nick);
    if (found)
    {
        if (found->blocked)
        {
            fprintf(stderr, "Client %s blocked, can't send message\n", nick);
            return;
        }

        add_client_message(nick, msg, found->dest_addr);
    }
    else
    {
        if (!nick_in_lookup(nick))
        {
            add_server_lookup_packet(nick);
        }
        add_client_message(nick, msg, empty_sockaddr());
    }
}

/**
 * @brief handle data from stdin
 */
void handle_stdin()
{
    char buf[MAXBUFSIZE];
    int c;
    fgets(buf, MAXBUFSIZE, stdin);
    if (buf[strlen(buf) - 1] == '\n')
    {
        buf[strlen(buf) - 1] = '\0';
    } // if /n overwrite with 0
    else
        while ((c = getchar()) != '\n' && c != EOF)
            ; // remove excess buf

    if (!strcmp(buf, "QUIT"))
    {
        main_event_loop = 0;
    }
    else // look for nick and send message
    {
        if (buf[0] == '@') // send msg @ client
        {
            handle_stdin_msg(buf);
        }
        else
        {
            char *nick, *action;
            char buf_copy[strlen(buf) + 1];
            strcpy(buf_copy, buf);

            action = strtok(buf_copy, " ");
            nick = strtok(NULL, " ");

            if (!action || !nick)
            {
                fprintf(stderr, "WRONG FORMAT\n");
                return;
            }
            if (!strcmp(action, "BLOCK")) // block nick
            {
                set_client_block(nick, 1);
            }
            else if (!strcmp(action, "UNBLOCK")) // unblock nick
            {
                set_client_block(nick, 0);
            }
            else
            {
                fprintf(stderr,
                        "WRONG FORMAT\nUsage:\n <@nick> <msg>\n <BLOCK> <nick>\n <UNBLOCK> <nick>\n");
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    int so, rc;
    char buf[BUFSIZE];
    int timeout;
    char const *nick, *port, *server_ip;
    fd_set my_set;
    struct sockaddr_in my_addr;
    struct timeval tv;
    time_t last_beat, last_send, curr_time;

    init_roots(); // init known clients linked list root

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

    last_send = time(NULL);
    last_beat = time(NULL);
    memset(buf, 0, BUFSIZE);
    while (main_event_loop)
    {
        tv.tv_sec = 1; // timeout select every second to check for sending
        tv.tv_usec = 0;

        FD_SET(STDIN_FILENO, &my_set);
        FD_SET(so, &my_set);
        rc = select(FD_SETSIZE, &my_set, NULL, NULL, &tv);
        check_error(rc, "select");

        curr_time = time(NULL);
        // send/handle messages with timeout interval
        if (curr_time - last_send > timeout)
        {
            if (main_event_loop < 0) // could not register with server
            {
                fprintf(stderr, "Could not register with server\nABORTING...\n");
                break;
            }
            add_to_flight(so);
            last_send = time(NULL);
        }

        if (FD_ISSET(so, &my_set))
        {
            handle_socket(so);
        }
        if (FD_ISSET(STDIN_FILENO, &my_set))
        {
            handle_stdin();
        }
        curr_time = time(NULL);
        if (curr_time - last_beat > HEARTBEAT) // heartbeat
        {
            send_heartbeat(so);
            last_beat = time(NULL);
        }
    }

    close(so);
    free_all();

    return EXIT_SUCCESS;
}