#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[])
{
    if (argc != 3 || strlen(argv[2]) != 1)
    {
        printf("Error: programmet tar en setning og bokstav som argumenter \n");
        return -1;
    }

    char *c = argv[2];
    char *s = argv[1];
    char *tmp = s;

    while (*s)
    {
        switch (*s)
        {
        case 'a':
            *s = *c;
            break;
        case 'e':
            *s = *c;
            break;
        case 'i':
            *s = *c;
            break;
        case 'o':
            *s = *c;
            break;
        case 'u':
            *s = *c;
            break;
        default:
            break;
        }
        s += 1;
    }

    printf("%s \n", tmp);
    return 0;
}