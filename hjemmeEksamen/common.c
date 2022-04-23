#include "common.h"

void check_error(int ret, char *msg)
{
    if (ret == -1)
    {
        perror(msg);
        exit(EXIT_FAILURE);
    }
}

int convertLossProbability()
{
    return 0;
}