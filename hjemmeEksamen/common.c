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