#include "common.h"

#define STALE 30

/**
 * @brief data struct for clients
 *
 */
typedef struct client
{
    int port;
    char ip[INET_ADDRSTRLEN];
    int last_number;
    time_t time_last;
    char nick[160];
} client;

client *clients;
int client_count = 0;

/**
 * @brief checks clients for nick
 *
 * @param nick
 * @return int index of client if exist else -1
 */
int nick_exists(char *nick)
{
    for (int i = 0; i < client_count; i++)
    {
        if (!strcmp(clients[i].nick, nick))
        {
            return i;
        }
    }
    return -1;
}

void lookup_nick(char *nick, char *response, char *number)
{
    int exists = nick_exists(nick);
    time_t curr_time = time(NULL);
    if (exists < 0 || curr_time - clients[exists].time_last > STALE)
    {
        sprintf(response, "ACK %s NOT FOUND", number);
    }
    else
    {
        sprintf(response, "ACK %s NICK %s IP %s PORT %d",
                number, clients[exists].nick, clients[exists].ip, clients[exists].port);
    }
}

char *update_client_time(char *nick)
{
    int exists = nick_exists(nick);
    if (exists > -1)
    {
        clients[exists].time_last = time(NULL);
    }
    return "OK"; // could return error if not exists if in scope of task
}

char *register_client(char *nick, struct sockaddr_in client_addr)
{
    int i;
    char *response;
    int exists = nick_exists(nick);
    if (exists < 0)
    {
        clients = (client *)realloc(clients, sizeof(client) * (++client_count));
        check_malloc_error(clients);
        i = client_count - 1;
        strcpy(clients[i].nick, nick);
    }
    else // overwrite nick
    {
        i = exists;
    }

    memcpy(&clients[i].ip, inet_ntoa(client_addr.sin_addr), INET_ADDRSTRLEN);
    clients[i].port = (int)ntohs(client_addr.sin_port);
    clients[i].last_number = 0;
    clients[i].time_last = time(NULL);

    printf("Registered client %s:\n  ip: %s\n  port: %d\n  time: %li\n",
           clients[i].nick, clients[i].ip, clients[i].port, clients[i].time_last);

    response = "OK";
    return response;
}

void print_clients()
{
    printf("Clients: \n");
    for (int i = 0; i < client_count; i++)
    {
        printf(" Client %s:\n  ip: %s\n  port: %d\n, time: %li",
               clients[i].nick, clients[i].ip, clients[i].port, clients[i].time_last);
    }
}

/**
 * @brief handle response according to buffer
 *
 * @param buf read buffer
 * @param clients clients ptr
 * @param client_addr client socket
 * @return char response to send back to client, must be freed
 */
char *handle_response(char *buf, struct sockaddr_in client_addr)
{
    char *response, buf_copy[strlen(buf) + 1];
    strcpy(buf_copy, buf);
    char *pkt = strtok(buf_copy, " ");
    char *number = strtok(NULL, " ");
    char *operation = strtok(NULL, " ");
    char *nick = strtok(NULL, " ");

    response = malloc(BUFSIZE);
    check_malloc_error(response);
    memset(response, 0, BUFSIZE);

    if (pkt == NULL)
    {
        sprintf(response, "WRONG FORMAT");
    }
    else if (!strcmp(pkt, "ACK"))
    {
        response[0] = 0;
    }
    else if (!strcmp(operation, "REG"))
    {
        sprintf(response, "ACK %s %s", number, register_client(nick, client_addr));
    }
    else if (!strcmp(operation, "LOOKUP"))
    {
        lookup_nick(nick, response, number);
    }
    else if (!strcmp(operation, "BEAT"))
    {
        sprintf(response, "ACK %s %s", number, update_client_time(nick));
    }
    else
    {
        sprintf(response, "ACK %s ERROR", number);
    }

    return response;
}

int main(int argc, char const *argv[])
{

    int so, rc, wc;
    unsigned short port;
    char buf[BUFSIZE], *response;
    // struct in_addr ipadress;
    struct sockaddr_in my_addr, client_addr;
    fd_set my_set;
    // struct timeval tv;
    socklen_t socklen = sizeof(struct sockaddr_in);

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

    FD_ZERO(&my_set);

    memset(buf, 0, BUFSIZE);
    while (strcmp(buf, "QUIT"))
    {
        FD_SET(so, &my_set);
        FD_SET(STDIN_FILENO, &my_set);
        rc = select(FD_SETSIZE + 1, &my_set, NULL, NULL, NULL); //&tv);
        check_error(rc, "select");

        if (FD_ISSET(so, &my_set))
        {

            rc = recvfrom(so, buf, BUFSIZE - 1, 0, (struct sockaddr *)&client_addr, &socklen);
            check_error(rc, "recvfrom");

            buf[rc] = '\0';
            buf[strcspn(buf, "\n")] = 0; // if /n overwrite with 0
            if (DEBUG)
                printf("Received %d bytes: %s\n", rc, buf);

            response = handle_response(buf, client_addr);

            if (response[0]) // response contains at least 1 char
            {
                send_message(so, client_addr, response);
                if (DEBUG)
                    printf("\n");
            }

            free(response);
        }
        if (FD_ISSET(STDIN_FILENO, &my_set))
        {
            fgets(buf, BUFSIZE, stdin);
            buf[strcspn(buf, "\n")] = 0; // if /n overwrite with 0
        }
    }

    if (DEBUG)
        print_clients(clients);

    close(so);
    free(clients);
    return EXIT_SUCCESS;
}
