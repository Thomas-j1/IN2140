#include <stdio.h>
#include <stdlib.h>
#include "prekode/inode.h"

void printbits(void *n, int size)
{
    char *num = (char *)n;
    int i, j;

    for (i = size - 1; i >= 0; i--)
    {
        for (j = 7; j >= 0; j--)
        {
            printf("%c", (num[i] & (1 << j)) ? '1' : '0');
        }
        printf(" ");
    }
    printf("\n");
}

void dump_buffer(void *buffer, int buffer_size)
{
    int i;

    for (i = 0; i < buffer_size; ++i)
        printf("%c", ((char *)buffer)[i]);
    printf("\n");
}

void dump_hex_buffer(char *buffer, int buffer_size)
{
    for (int c = 0; c < buffer_size; c++)
    {
        printf("%.2X ", (int)buffer[c]);

        // put an extra space between every 4 bytes
        if (c % 4 == 3)
        {
            printf(" ");
        }

        // Display 16 bytes per line
        if (c % 16 == 15)
        {
            printf("\n");
        }
    }
    // Add an extra line feed for good measure
    printf("\n");
}

/* void dump_inodeData(void *buffer, int buffer_size)
{

    // Add an extra line feed for good measure
    printf("\n");
} */

void ReadFile(char *name)
{
    FILE *file;
    char *buffer;
    unsigned long fileLen;

    // Open file
    file = fopen(name, "rb");
    if (!file)
    {
        fprintf(stderr, "Unable to open file %s", name);
        return;
    }

    // Get file length
    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory
    buffer = (char *)malloc(fileLen + 1);
    if (!buffer)
    {
        fprintf(stderr, "Memory error!");
        fclose(file);
        return;
    }

    // Read file contents into buffer
    fread(buffer, fileLen, 1, file);
    fclose(file);

    dump_buffer(buffer, fileLen + 1);
    printbits(buffer, fileLen + 1);

    free(buffer);
}

void printiNodes(FILE *fp)
{
    struct inode *obuff = malloc(sizeof(struct inode));
    if (obuff == NULL)
    {
        /* obuff can't be used because allocation failed */
        exit(EXIT_FAILURE);
    }
    int *name_len;
    fread(&obuff->id, sizeof(int), 1, fp);
    fread(name_len, sizeof(int), 1, fp);
    obuff->name = malloc(*name_len);
    fread(obuff->name, *name_len, 1, fp);
    fread(&obuff->is_directory, sizeof(char), 1, fp);
    fread(&obuff->is_readonly, sizeof(char), 1, fp);
    fread(&obuff->filesize, sizeof(int), 1, fp);
    fread(&obuff->num_entries, sizeof(int), 1, fp);
    obuff->entries = malloc(sizeof(size_t) * obuff->num_entries);
    fread(obuff->entries, obuff->num_entries * sizeof(size_t), 1, fp);
    if (obuff->is_directory)
    {
        for (int i = 0; i < obuff->num_entries; i++)
        {
            printiNodes(fp);
        }
    }

    // fread(&obuff->entries, sizeof(obuff->entries), 1, fp);
    printf("id: %d, name_len: %d, name: %s, isDir: %d, isReadOnly: %d, fileSize: %d, num_entries: %d\n",
           obuff->id, *name_len, obuff->name, obuff->is_directory, obuff->is_readonly,
           obuff->filesize, obuff->num_entries);
    free(obuff); /* free whatever you allocated after finished using them */
}

void myReadFile(char *filename)
{

    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    struct inode *obuff = malloc(sizeof(struct inode));
    if (obuff == NULL)
    {
        /* obuff can't be used because allocation failed */
        exit(EXIT_FAILURE);
    }
    printiNodes(fp);
    /* fread(&obuff->id, sizeof(int), 1, fp);
    fread(&obuff->name_len, sizeof(int), 1, fp);
    obuff->name = malloc(obuff->name_len);
    fread(obuff->name, obuff->name_len, 1, fp);
    fread(&obuff->is_directory, sizeof(char), 1, fp);
    fread(&obuff->is_readonly, sizeof(char), 1, fp);
    fread(&obuff->filesize, sizeof(int), 1, fp);
    fread(&obuff->num_entries, sizeof(int), 1, fp);
    // fread(&obuff->entries, sizeof(obuff->entries), 1, fp);
    printf("id: %d, name_len: %d, name: %s, isDir: %d, isReadOnly: %d, fileSize: %d, num_entries: %d\n",
           obuff->id, obuff->name_len, obuff->name, obuff->is_directory, obuff->is_readonly,
           obuff->filesize, obuff->num_entries);
    free(obuff); /* free whatever you allocated after finished using them */

    fclose(fp);
}

int main(void)
{
    // ReadFile("prekode/load_example1/superblock");
    myReadFile("prekode/load_example2/superblock");

    return EXIT_SUCCESS;
}
