#ifndef MY_COMMONS
#define MY_COMMONS

#include "preCode/send_packet.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define BUFSIZE 250
#define MAXBUFSIZE 1600

/**
 * @brief checks return value for error
 *  & if error calls perror with msg and exits
 *
 * @param ret return value
 * @param msg perror
 */
void check_error(int ret, char *msg);

/**
 * @brief checks malloc if malloc return is NULL,
 * if true prints error to stderr & exits
 *
 * @param ptr
 */
void check_malloc_error(void *ptr);

/**
 * @brief convert percentage to float
 *
 * @param n int number
 * @return float probability between 0 & 1
 */
float convert_loss_probability(int n);

/**
 * @brief Set the Loss Probability
 *
 * @param arg loss probability argument from main
 */
void setup_loss_probability(const char *arg);

void send_message(int so, struct sockaddr_in dest_addr, char *msg);

void send_ok(int so, struct sockaddr_in dest_addr, char *number);

#endif