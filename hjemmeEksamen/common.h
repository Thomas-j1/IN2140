#ifndef MY_COMMONS
#define MY_COMMONS

#include "preCode/send_packet.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

void check_error(int ret, char *msg);

int convertLossProbability();

#endif