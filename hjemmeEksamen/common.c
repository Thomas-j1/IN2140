#include "common.h"

void check_error(int ret, char *msg)
{
    if (ret == -1)
    {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

void check_malloc_error(void *ptr)
{
    if (ptr == NULL)
    {
        fprintf(stderr, "Malloc error");
        exit(EXIT_FAILURE);
    }
}

float convert_loss_probability(int n)
{
    float res = (float)n;
    res = res / 100.0;
    return res;
}

void setup_loss_probability(const char *arg)
{
    unsigned short loss_probability;
    float floss_probability;
    srand48(time(NULL));
    loss_probability = atoi(arg);
    floss_probability = convert_loss_probability(loss_probability);
    set_loss_probability(floss_probability);
    if (DEBUG)
        printf("setup loss probability: %f\n\n", floss_probability);
}

void send_message(int so, struct sockaddr_in dest_addr, char *msg)
{
    int rc;

    rc = sendto(so,
                msg,
                strlen(msg),
                0,
                (struct sockaddr *)&dest_addr,
                sizeof(struct sockaddr_in));
    check_error(rc, "sendto");
    if (DEBUG)
        printf("Sent message: %s\n", msg);
}

void send_loss_message(int so, struct sockaddr_in dest_addr, char *msg)
{
    int rc;

    if (DEBUG)
    {
        printf(YEL "Attempting to send message: %s\n", msg);
        printf(RED);
        fflush(stdout);
    }

    rc = send_packet(so,
                     msg,
                     strlen(msg),
                     0,
                     (struct sockaddr *)&dest_addr,
                     sizeof(struct sockaddr_in));
    if (DEBUG)
    {
        printf(RESET);
        fflush(stdout);
    }
    check_error(rc, "sendto");
}

void send_ok(int so, struct sockaddr_in dest_addr, char *number)
{
    char ackBuf[BUFSIZE];

    sprintf(ackBuf, "ACK %s OK", number);
    send_loss_message(so, dest_addr, ackBuf);
}