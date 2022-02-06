#include <stdlib.h>

int stringsum(char *s)
{
    int sum = 0;
    while (*s)
    {
        int val = (int)*s;

        if (val > 64 && val < 91) // uppercase
        {
            val -= 64;
        }
        else if (val > 96 && val < 123) // lowercase
        {
            val -= 96;
        }
        else if (val == 32 || val == 0) // "space" & '\0'
        {
            val = 0;
        }
        else
        {
            return -1;
        }
        sum += val;
        s++;
    }
    return sum;
}

int distance_between(char *s, char c)
{
    int first = 0;
    int last = 0;
    int count = 1;

    while (*s)
    {
        if (*s == c)
        {
            if (!first)
            {
                first = count;
            }
            else
            {
                last = count;
            }
        }
        s++;
        count++;
    }
    if (!last && !first)
    {
        return -1;
    }
    else if (!last)
    {
        return 0;
    }
    else
    {
        return last - first;
    }
}

char *string_between(char *s, char c)
{
    while (*s && *s != c)
    {
        s++;
    }

    int last = distance_between(s, c);

    if (last < 0)
    {
        return NULL;
    }

    if (last < 1)
    {
        last = 1;
    }
    char *mid = malloc(last);

    for (int i = 1; i < last; i++)
    {
        mid[i - 1] = s[i];
    }
    mid[last - 1] = 0;

    return mid;
}

int stringsum2(char *s, int *res)
{
    *res = stringsum(s);
    return 0;
}