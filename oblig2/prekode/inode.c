#include "allocation.h"
#include "inode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BLOCKSIZE 4096

struct inode *create_file(struct inode *parent, char *name, char readonly, int size_in_bytes)
{
    return NULL;
}

struct inode *create_dir(struct inode *parent, char *name)
{
    return NULL;
}

struct inode *find_inode_by_name(struct inode *parent, char *name)
{
    return NULL;
}

int fileBlockNumber = 0;

struct inode *loadNodesHelper(FILE *fp)
{

    struct inode *node = malloc(sizeof(struct inode));
    if (node == NULL)
    {
        /* node can't be used because allocation failed */
        exit(EXIT_FAILURE);
    }
    char name_len[sizeof(int)];
    fread(&node->id, sizeof(int), 1, fp);
    fread(name_len, sizeof(int), 1, fp);
    node->name = malloc((int)*name_len + 1);
    fread(node->name, *name_len, 1, fp);
    fread(&node->is_directory, sizeof(char), 1, fp);
    fread(&node->is_readonly, sizeof(char), 1, fp);
    fread(&node->filesize, sizeof(int), 1, fp);
    fread(&node->num_entries, sizeof(int), 1, fp);
    size_t *oppforing = malloc(sizeof(size_t) * node->num_entries);
    node->entries = malloc(64 * node->num_entries);
    fread(oppforing, node->num_entries * sizeof(size_t), 1, fp);
    if (node->is_directory)
    {
        for (int i = 0; i < node->num_entries; i++)
        {
            struct inode *child = malloc(sizeof(struct inode));
            if (child == NULL)
            {
                /* node can't be used because allocation failed */
                exit(EXIT_FAILURE);
            }
            child = loadNodesHelper(fp);
            node->entries[i] = (size_t)child;
        }
    }
    else
    {
        for (int i = 0; i < node->num_entries; i++)
        {
            int *blockNumber = malloc(BLOCKSIZE);
            blockNumber = fileBlockNumber++;
            node->entries[i] = (size_t)blockNumber;
        }
    }
    return node;
}

struct inode *load_inodes()
{
    FILE *fp;
    fp = fopen("superblock", "rb");
    if (fp == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    struct inode *root = malloc(sizeof(struct inode));
    if (root == NULL)
    {
        /* node can't be used because allocation failed */
        exit(EXIT_FAILURE);
    }
    root = loadNodesHelper(fp);

    return root;
}

void fs_shutdown(struct inode *inode)
{
}

/* This static variable is used to change the indentation while debug_fs
 * is walking through the tree of inodes and prints information.
 */
static int indent = 0;

void debug_fs(struct inode *node)
{
    if (node == NULL)
        return;
    for (int i = 0; i < indent; i++)
        printf("  ");
    if (node->is_directory)
    {
        printf("%s (id %d)\n", node->name, node->id);
        indent++;
        for (int i = 0; i < node->num_entries; i++)
        {
            struct inode *child = (struct inode *)node->entries[i];
            debug_fs(child);
        }
        indent--;
    }
    else
    {
        printf("%s (id %d size %db blocks ", node->name, node->id, node->filesize);
        for (int i = 0; i < node->num_entries; i++)
        {
            printf("%d ", (int)node->entries[i]);
        }
        printf(")\n");
    }
}
