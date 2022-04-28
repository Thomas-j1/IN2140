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

    loss_probability = atoi(arg);
    floss_probability = convert_loss_probability(loss_probability);
    set_loss_probability(floss_probability);
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
}

void send_loss_message(int so, struct sockaddr_in dest_addr, char *msg)
{
    int rc;

    rc = send_packet(so,
                     msg,
                     strlen(msg),
                     0,
                     (struct sockaddr *)&dest_addr,
                     sizeof(struct sockaddr_in));
    check_error(rc, "sendto");
}

void send_ok(int so, struct sockaddr_in dest_addr, char *number)
{
    char ackBuf[BUFSIZE];

    sprintf(ackBuf, "ACK %s OK", number);
    send_message(so, dest_addr, ackBuf);
    printf("\n");
}

int send_message_wait(int so, struct sockaddr_in dest_addr, char *msg, int number)
{
    int resend = 1;
    int tries = 0;
    while (resend && tries < 4)
    {
        send_loss_message(so, dest_addr, msg);
        resend = listen_for_ack(so, number, dest_addr);
        tries++;
    }
    printf("Sent message: %s\n", msg);
    return tries < 4;
}

int listen_for_ack(int so, int expected_number, struct sockaddr_in last_addr)
{
    int rc;
    char buf[BUFSIZE];
    socklen_t socklen = sizeof(struct sockaddr_in);

    rc = recvfrom(so, buf, BUFSIZE - 1, 0, (struct sockaddr *)&last_addr, &socklen);
    check_error(rc, "recvfrom");

    buf[rc] = '\0';
    buf[strcspn(buf, "\n")] = 0;
    printf("Received %d bytes: %s\n", rc, buf);

    strtok(buf, " "); // ack
    int received_number = atoi(strtok(NULL, " "));

    return expected_number == received_number;
}
