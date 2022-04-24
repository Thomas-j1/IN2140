#include "common.h"

/**
 * @brief data struct for clients
 *
 */
typedef struct client
{
    int port;
    char ip[INET_ADDRSTRLEN];
    char nick[160];
} client;

client *clients;
int numberOfClients = 0;

char *lookup_nick(char *nick, char *number)
{
    /* char response[40];
    if (lookup_nick(clients, nick, number))
    {
        sprintf(response, "ACK %s OK", number);
    }
    else
    {
        sprintf(response, "ACK number NICK nick IP address PORT port", number);
    } */
    return 0;
}

char *register_client(char *nick, struct sockaddr_in client_addr)
{
    char *response;
    clients = (client *)realloc(clients, sizeof(client) * (++numberOfClients));
    check_malloc_error(clients);
    int currIndex = numberOfClients - 1;

    strcpy(clients[currIndex].nick, nick);
    memcpy(&clients[currIndex].ip, inet_ntoa(client_addr.sin_addr), INET_ADDRSTRLEN);
    clients[currIndex].port = (int)ntohs(client_addr.sin_port);

    printf("Registered client %s:\n  ip: %s\n  port: %d\n",
           clients[currIndex].nick, clients[currIndex].ip, clients[currIndex].port);

    response = "done";
    return response;
}

void print_clients()
{
    printf("Clients: \n");
    for (int i = 0; i < numberOfClients; i++)
        printf(" Client %s:\n  ip: %s\n  port: %d\n", clients[i].nick, clients[i].ip, clients[i].port);
}

/**
 * @brief handle response according to buffer
 *
 * @param buf read buffer
 * @param clients clients ptr
 * @param client_addr client socket
 * @return char response to send back to client
 */
char *handle_response(char *buf, struct sockaddr_in client_addr)
{
    char *response, mBuf[strlen(buf) + 1];
    strcpy(mBuf, buf);
    char *pkt = strtok(mBuf, " ");
    char *number = strtok(NULL, " ");
    char *operation = strtok(NULL, " ");
    char *nick = strtok(NULL, " ");

    if (!strcmp(operation, "REG"))
    {
        response = register_client(nick, client_addr);
    }
    else if (!strcmp(operation, "LOOKUP"))
    {
        response = lookup_nick(nick, number);
    }
    else
    {
        response = "WRONG FORMAT";
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

    memset(buf, 0, BUFSIZE);
    while (strcmp(buf, "q"))
    {
        rc = recvfrom(so, buf, BUFSIZE - 1, 0, (struct sockaddr *)&client_addr, &socklen);
        check_error(rc, "recvfrom");

        buf[rc] = '\0';
        buf[strcspn(buf, "\n")] = 0; // if /n overwrite with 0
        printf("Received %d bytes: %s\n", rc, buf);
        if (strcmp(buf, "q"))
        {
            response = handle_response(buf, client_addr);
            printf("Sending response: %s\n\n", response);
        }
    }

    print_clients(clients);

    close(so);
    free(clients);
    return EXIT_SUCCESS;
}
