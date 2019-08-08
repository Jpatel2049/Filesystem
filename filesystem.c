#include "filesystem.h"

entry NULL_ENTRY = {0};

/* Gets the metadata for a file . */
entry * get_meta(char *filepath, drive *D) {
  /* Creates a copy of the filepath. */
  char * copy = (char *) malloc(sizeof(char *) * strlen(filepath) + 1);
  strcpy(copy, filepath);

    if (strcmp(copy, "") == 0) {
        /* Returns the metadata of the root dir. */
        entry *r = (entry *) malloc(sizeof(entry));
        r->type = dir;
        r->start = 0;
        return r;
    }

    /* Pointer to a null entry struct. */
    entry *meta = &NULL_ENTRY;

    int found = 0, i, block;
    f_type last = dir;

    /* Splits path into tokens and iterates through them. */
    for (char *token = strtok(copy, "/"); token != NULL; token = strtok(NULL, "/")) {
        /* Returns NULL if last in file. */
        if (last == file) {
          free(copy);
          return NULL;
        }
        /* Iterates through the current directory. */
        for (block = meta->start; block != -1 && !found; block = D->FAT[block]) {
            for (i = 0; i < BLOCK_SIZE / sizeof(entry) && !found; i++) {
                if (strcmp(D->data[block].dir[i].name, token) == 0) {
                    /* Name matches. */
                    last = D->data[block].dir[i].type;
                    meta = &(D->data[block].dir[i]);
                    found++;
                }
            }
        }
        /* Current token not found, file DNE. */
        if (found == 0) {
          free(copy);
          return NULL;
        }
        found = 0;
    }
    /* File was not found. */
    if (meta == &NULL_ENTRY) {
      free(copy);
      return NULL;
    }
    /* Returns the last pointer found. */
    free(copy);
    return meta;
}

/* Creates a file. */
void create_file(char *filepath, f_type type, drive *D) {
  int i, j;
    /* Creates a copy of the filepath. */
    char * copy = (char *) malloc(sizeof(char *) * strlen(filepath) + 1);
    strcpy(copy, filepath);

    /* Checks if file already exists. */
    if (get_meta(copy, D) != NULL) {
        printf("File/Directory already exists.\n");
        free(copy);
        return;
    }

    /* Splits the name of the file. */
    char *name = strrchr(copy, '/');

    /* Invalid syntax. */
    if (name == NULL) {
      free(copy);
      return;
    } else {
      *name = '\0';
      name = name + 1;
    }

    /* Beginning of directory. */
    int dirStart;

    entry *dirMeta = get_meta(copy, D);

    /* Checks if file exists. */
    if (dirMeta == NULL) {
        printf("Directory DNE.\n");
        free(copy);
        return;
    } else {
       /* Sets the start block. */
       dirStart = dirMeta->start;
    }

    /* Allocates block for the new file. */
    int fileStart = allocate_block(D);
    if (fileStart == -1) {
      free(copy);
      printf("Unable to allocate block.\n");
      return;
    }

    /* Finds first empty slot in the directory and fills it in. */
    for (j = 0; dirStart != -1; dirStart = D->FAT[dirStart]) {
        for (i = 0; i < BLOCK_SIZE / sizeof(entry); i++) {
            if (D->data[dirStart].dir[i].name[0] == '\0') {
                D->data[dirStart].dir[i].start = fileStart;
                D->data[dirStart].dir[i].size_of_file = 0;
                D->data[dirStart].dir[i].type = type;
                strcpy(D->data[dirStart].dir[i].name, name);
                free(copy);
                return;
            }
        }
    }

    int newDirBlock = allocate_block(D);
    if (newDirBlock == -1) {
      free(copy);
      printf("Unable to allocate block.\n");
      return;
    }

    /* Fills in the file for the new block. */
    D->FAT[dirStart] = newDirBlock;
    D->data[newDirBlock].dir[0].start = fileStart;
    D->data[newDirBlock].dir[0].size_of_file = 0;
    D->data[newDirBlock].dir[0].type = type;
    strcpy(D->data[newDirBlock].dir[0].name, name);
    free(copy);
}

/* Writes data to a file. */
void write_file(filePointer *file, char *data, int len, drive *D) {
    /* Amount already written. */
    int offset = 0;

    /* Amount to be written. */
    int amount;
    if(file->ptr + len > BLOCK_SIZE) {
      amount = BLOCK_SIZE - file->ptr;
    } else {
        amount = len;
    }

    /* Loops until there is no data left to write. */
    while (len > 0) {
        /* Copy amount into block. */
        memcpy(&(D->data[file->curr].file[file->ptr]), data + offset, amount);
        /* Updates length. */
        len -= amount;
        /* Moves file pointer. */
        file->ptr += amount;
        /* Updates offset. */
        offset += amount;
        /* Increase the file size by amount. */
        file->meta->size_of_file += amount;
        /* Checks if end of block was reached. */
        if (file->ptr == BLOCK_SIZE) {
            /* Adds new block. */
            D->FAT[file->curr] = allocate_block(D);
            /* Moves to that block. */
            file->curr = D->FAT[file->curr];
            /* Resets pointer. */
            file->ptr = 0;
        }
        /* Calculates amount to be written. */
        if(len > BLOCK_SIZE) {
          amount = BLOCK_SIZE;
        } else {
            amount = len;
        }
    }
}

/* Opens a file. */
filePointer * open_file(char *filepath, drive *D) {
    /* Creates a copy of the filepath. */
    char * copy = (char *) malloc(sizeof(char *) * strlen(filepath) + 1);
    strcpy(copy, filepath);

    /* Pointer for file. */
    entry *meta = get_meta(copy, D);

    /* If file DNE or is already open. */
    if (meta == NULL) {
        printf("File DNE.\n");
        free(copy);
        return NULL;
    } else if (meta->is_open) {
        free(copy);
        printf("File is already open.\n");
        return NULL;
    }

    /* Creates the file pointer. */
    filePointer *file = (filePointer *) malloc(sizeof(filePointer));
    file->meta = meta;
    file->ptr = 0;
    file->curr = file->meta->start;
    file->meta->is_open = 1;

    /* Returns the file pointer. */
    free(copy);
    return file;
}

/* Reads the data from the file. */
void read_file(char *dest, filePointer *file, int len, drive *D) {
    /* Amount of data already read. */
    int offset = 0;

    /* Clears destination. */
    memset(dest, 0, len);

    /* Amount to be written. */
    int amount;
    if(file->ptr + len > BLOCK_SIZE) {
      amount = BLOCK_SIZE - file->ptr;
    } else {
        amount = len;
    }

    /* Iterates through the blocks. */
    while (len > 0 && file->curr != -1) {
        /* Copy data into destination. */
        memcpy(dest + offset, &(D->data[file->curr].file[file->ptr]), amount);
        /* Updates offset. */
        offset += amount;
        /* Decreases amount to be read. */
        len -= amount;
        /* Updates file pointer. */
        file->ptr += amount;
        /* Checks if the end of the block has been reached. */
        if (file->ptr == BLOCK_SIZE) {
            /* Resets file pointer. */
            file->ptr = 0;
            /* Moves to the next block. */
            file->curr = D->FAT[file->curr];
        }
        /* Calculates amount to be read. */
        if(len > BLOCK_SIZE) {
          amount = BLOCK_SIZE;
        } else {
            amount = len;
        }
    }
}

/* Closes a file. */
void close_file(filePointer *file) {
    /* Sets the file to closed. */
    file->meta->is_open = 0;
    /* Frees the pointer. */
    free(file);
}

/* Deletes a file. */
void delete_file(char *filepath, drive *D) {
  int i, j;
    /* Creates copy of file path. */
    char *copy = (char *) malloc(sizeof(char *) * strlen(filepath) + 1);
    strcpy(copy, filepath);
    /* Gets the file's metadata. */
    entry *file = get_meta(copy, D);
    /* The file is open or DNE. */
    if (file->is_open) {
      printf("File is currently in use.\n");
      free(copy);
      return;
    } else if(file == NULL) {
        printf("File DNE.\n");
        free(copy);
        return;
    }

    /* Sets all of the blocks free. */
    for (i = file->start, j = -1; i != -1; i = j) {
        j = D->FAT[i];
        D->FAT[i] = 0;
    }
    /* Deletes the file entry. */
    memset(file, 0, sizeof(entry));
    /* Frees the copy. */
    free(copy);
}

/* Allocates an empty block and returns the block number. */
int allocate_block(drive *D) {
  int i;
  /* Finds the first empty block. */
  for (i = 1; i < NUM_BLOCKS; i++) {
    /* Empty block found . */
      if (D->FAT[i] == 0) {
        /* Set as occupied. */
        D->FAT[i] = -1;
        /* Returns the block number. */
        return i;
        }
    }
    /* Returns -1 if all blocks are full. */
    return -1;
}
