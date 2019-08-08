#ifndef FILESYSTEM_FILESYSTEM_H
#define FILESYSTEM_FILESYSTEM_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/fcntl.h>

#define BLOCK_SIZE 512
#define NUM_BLOCKS (10485760 / (BLOCK_SIZE + sizeof(int)))

typedef enum {
    file, dir
} f_type;

typedef struct {
    char name[35];
    int start;
    int size_of_file;
    f_type type: 1;
    int is_open: 1;
} entry;

typedef union {
    char file[BLOCK_SIZE];
    entry dir[BLOCK_SIZE / sizeof(entry)];
} block;

typedef struct {
    int FAT[NUM_BLOCKS];
    block data[NUM_BLOCKS];
} drive;

typedef struct {
    entry *meta;
    int ptr;
    int curr;
} filePointer;

entry * get_meta(char *, drive *);
void create_file(char *, f_type, drive *);
void write_file(filePointer *, char *, int, drive *);
filePointer * open_file (char *, drive *);
void read_file(char *, filePointer *, int, drive *);
void close_file(filePointer *);
void delete_file(char *, drive *);
int allocate_block(drive *);

#endif
