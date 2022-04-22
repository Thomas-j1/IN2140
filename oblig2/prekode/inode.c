#include "allocation.h"
#include "inode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

#define BLOCKSIZE 4096
int id = 0;

int isDuplicate(struct inode *parent, char *name)
{
    if (parent == NULL)
        return 0;
    if (parent->is_directory)
    {
        for (int i = 0; i < parent->num_entries; i++)
        {
            struct inode *child = (struct inode *)parent->entries[i];
            if (!strcmp(child->name, name))
            {
                return 1;
            }
        }
    }
    return 0;
}

struct inode *create_file(struct inode *parent, char *name, char readonly, int size_in_bytes)
{
    if (isDuplicate(parent, name))
    {
        return NULL;
    }
    struct inode *new = malloc(sizeof(struct inode));
    if (new == NULL)
    {
        fprintf(stderr, "Malloc error");
        exit(EXIT_FAILURE);
    }
    new->id = id++;
    new->name = strdup(name);
    new->is_directory = 0;
    new->is_readonly = readonly;
    new->filesize = size_in_bytes;
    double numberOfBlocks = (double)size_in_bytes / (double)BLOCKSIZE;
    new->num_entries = ceil(numberOfBlocks);
    new->entries = malloc(sizeof(size_t) * new->num_entries);

    for (int i = 0; i < new->num_entries; i++)
    {
        new->entries[i] = allocate_block();
    }

    if (parent)
    {
        parent->entries = realloc(parent->entries, (parent->num_entries + 1) * 64);
        parent->entries[parent->num_entries] = (size_t) new;
        parent->num_entries++;
    }
    return parent;
}

struct inode *create_dir(struct inode *parent, char *name)
{
    if (isDuplicate(parent, name))
    {
        return NULL;
    }

    struct inode *new = malloc(sizeof(struct inode));
    if (new == NULL)
    {
        fprintf(stderr, "Malloc error");
        exit(EXIT_FAILURE);
    }
    new->id = id++;
    new->name = strdup(name);
    new->is_directory = 1;
    new->is_readonly = 0;
    new->filesize = 0;
    new->num_entries = 0;
    new->entries = malloc(64);

    if (parent)
    {
        parent->entries = realloc(parent->entries, (parent->num_entries + 1) * 64);
        parent->entries[parent->num_entries] = (size_t) new;
        parent->num_entries++;
    }

    return new;
}

struct inode *find_inode_by_name(struct inode *parent, char *name)
{
    if (parent == NULL)
        return NULL;
    if (!strcmp(parent->name, name))
    {
        return parent;
    }
    if (parent->is_directory)
    {
        for (int i = 0; i < parent->num_entries; i++)
        {
            struct inode *child = (struct inode *)parent->entries[i];
            struct inode *found = find_inode_by_name(child, name);
            if (found)
            {
                return found;
            }
        }
    }
    return NULL;
}

struct inode *loadNodesHelper(FILE *fp)
{

    struct inode *node = malloc(sizeof(struct inode));
    if (node == NULL)
    {
        fprintf(stderr, "Malloc error");
        exit(EXIT_FAILURE);
    }

    char name_len[sizeof(int)];
    fread(&node->id, sizeof(int), 1, fp);
    fread(name_len, sizeof(int), 1, fp);
    node->name = malloc((int)*name_len);
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
            struct inode *child = loadNodesHelper(fp);
            node->entries[i] = (size_t)child;
        }
    }
    else
    {
        for (int i = 0; i < node->num_entries; i++)
        {

            node->entries[i] = (size_t)oppforing[i];
        }
    }
    free(oppforing);
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

    struct inode *root = loadNodesHelper(fp);
    fclose(fp);

    return root;
}

void fs_shutdown(struct inode *inode)
{
    if (inode == NULL)
        return;
    if (inode->is_directory)
    {
        for (int i = 0; i < inode->num_entries; i++)
        {
            struct inode *child = (struct inode *)inode->entries[i];
            fs_shutdown(child);
        }
    }
    else
    {
        for (int i = 0; i < inode->num_entries; i++)
        {
            // free(inode->entries[i]);
        }
    }
    free(inode->name);
    free(inode->entries);
    free(inode);
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