#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "the_apple.h"

int locateworm(char *apple)
{
    int count = 0;
    char *p = apple;
    while (*p && *p != 'w')
    {
        p++;
        count++;
    }

    return (*p == 'w') ? count : 0;
}

int removeworm(char *apple)
{
    int start = locateworm(apple);

    if (start)
    {
        char *p = apple + start;
        int count = 0;
        while (*p && *p != 'm')
        {
            *p = ' ';
            count++;
            p++;
        }
        while (*p && *p == 'm')
        {
            *p = ' ';
            count++;
            p++;
        }

        return count;
    }
    else
    {
        return 0;
    }
}

int main(void)
{
    // original eple er en string literal som ikke kan overskrives med mellomrom (immutable)
    // for oppgave b), antar at det er meningen av vi skal lage kopier
    char *appleCopy = strdup(apple);

    int result = locateworm(appleCopy);
    printf("Location: %d\n", result);

    int removed = removeworm(appleCopy);
    printf("Removed: %d\n", removed);

    int removedSecond = removeworm(appleCopy);
    printf("Second remove: %d\n", removedSecond);
    free(appleCopy);
    return 0;
}
