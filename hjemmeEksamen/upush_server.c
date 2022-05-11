#include "common.h"

#define STALE 30     // 30
#define CLEANFREQ 60 // 60

/**
 * @brief data struct for clients
 *
 */
typedef struct client
{
    int port;
    char ip[INET_ADDRSTRLEN];
    time_t time_last;
    char nick[20];
} client;

client *clients;
int client_count = 0;

/**
 * @brief checks clients for nick
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
    if (exists < 0 || (curr_time - clients[exists].time_last) > STALE)
    {
        sprintf(response, "ACK %s NOT FOUND", number);
    }
    else
    {
        sprintf(response, "ACK %s NICK %s IP %s PORT %d",
                number, clients[exists].nick, clients[exists].ip, clients[exists].port);
    }
}

/**
 * @brief overwrites values at client at index i,
 * with values at i+1 until i = client_count
 * then sets client_count-1 and reallocs with new smaller size
 */
void remove_client(int i)
{
    void *tmp;

    for (int j = i; j < client_count; j++)
    {
        if (j < client_count - 1)
        {
            clients[j].port = clients[j + 1].port;
            memcpy(clients[j].ip, clients[j + 1].ip, INET_ADDRSTRLEN);
            clients[j].time_last = clients[j + 1].time_last;
            strcpy(clients[j].nick, clients[j + 1].nick);
        }
    }
    client_count--;
    if (client_count)
    {
        tmp = realloc(clients, sizeof(client) * (client_count));
        check_malloc_error(tmp);
        clients = (client *)tmp;
    }
}

void remove_outdated_clients()
{
    time_t curr_time = time(NULL);

    for (int i = 0; i < client_count; i++)
    {
        if (curr_time - clients[i].time_last > STALE)
        {
            if (DEBUG)
                printf("Removing stale client %s\n", clients[i].nick);
            remove_client(i);
        }
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
    if (exists < 0) // add new client
    {
        void *tmp;
        tmp = realloc(clients, sizeof(client) * (++client_count));
        check_malloc_error(tmp);
        clients = (client *)tmp;
        i = client_count - 1;
        strcpy(clients[i].nick, nick);
    }
    else // overwrite existing nick/client
    {
        i = exists;
    }

    memcpy(clients[i].ip, inet_ntoa(client_addr.sin_addr), INET_ADDRSTRLEN);
    clients[i].port = (int)ntohs(client_addr.sin_port);
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
        printf(" Client %s:\n  ip: %s\n  port: %d\n  time: %li\n",
               clients[i].nick, clients[i].ip, clients[i].port, clients[i].time_last);
    }
}

/**
 * @brief handle response according to buffer
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

    if (!pkt || !number || !operation || !nick)
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

    int so, rc;
    unsigned short port;
    char buf[BUFSIZE], *response;
    // struct in_addr ipadress;
    struct sockaddr_in my_addr, client_addr;
    fd_set my_set;
    // struct timeval tv;
    socklen_t socklen = sizeof(struct sockaddr_in);
    time_t last_clean;

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
    last_clean = time(NULL);

    memset(buf, 0, BUFSIZE);
    while (strcmp(buf, "QUIT"))
    {
        FD_SET(so, &my_set);
        FD_SET(STDIN_FILENO, &my_set);
        rc = select(FD_SETSIZE + 1, &my_set, NULL, NULL, NULL); //&tv);
        check_error(rc, "select");

        if (FD_ISSET(so, &my_set)) // socket
        {

            rc = recvfrom(so, buf, BUFSIZE - 1, 0, (struct sockaddr *)&client_addr, &socklen);
            check_error(rc, "recvfrom");

            buf[rc] = '\0';
            buf[strcspn(buf, "\n")] = 0; // if /n overwrite with 0
            if (DEBUG)
                printf(GRN "Received %d bytes: %s\n" RESET, rc, buf);

            response = handle_response(buf, client_addr);

            if (response[0]) // response contains at least 1 char
            {
                send_loss_message(so, client_addr, response);
                if (DEBUG)
                    printf("\n");
            }

            free(response);
        }
        if (time(NULL) - last_clean > CLEANFREQ) // remove stale entries
        {
            remove_outdated_clients();
            last_clean = time(NULL);
        }
        if (FD_ISSET(STDIN_FILENO, &my_set)) // stdin
        {
            fgets(buf, BUFSIZE, stdin);
            buf[strcspn(buf, "\n")] = 0; // if /n overwrite with 0
        }
    }

    if (DEBUG)
        print_clients();

    close(so);
    free(clients);

    return EXIT_SUCCESS;
}
