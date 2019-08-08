#include "filesystem.h"

int main(int args, char * argv[]) {
  /* Drive name not provided. */
  if(args < 2) {
    printf("Please enter drive name. Exiting...\n");
    exit(0);
  } else {
    /* Opens the drive. */
    int fd = open(argv[1], O_RDWR);
    if(fd == -1) {
      printf("Error opening drive. Exiting...\n");
      exit(1);
    }
    /* Maps the memory. */
    drive *D = mmap(0, sizeof(drive), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(D == MAP_FAILED) {
      printf("Error with mmap. Exiting...\n");
      exit(1);
    }
     close(fd);

     /* Sets the start of the root directory. */
    D->FAT[0] = -1;

    /* Tests the filesystem. */

    /* Create a sample directory and two test files. */
    create_file("/dir", dir, D);
    create_file("/dir/test1.txt", file, D);
    create_file("/dir/test2.txt", file, D);

    char *sample = "A long time ago in a galaxy far, far away ....";

    /* Opens test1 file. */
    filePointer *f;
    f = open_file("/dir/test1.txt", D);
    /* Writes the sample string to test1. */
    write_file(f, sample, strlen(sample), D);
    /* Closes test1. */
    close_file(f);
    /* Opens test1 file. */
    f = open_file("/dir/test1.txt", D);
    /* Reads test1 file. */
    char output[strlen(sample) + 1];
    read_file(output, f, strlen(sample), D);
    /* Closes test1. */
    close_file(f);
    /* Prints the contents of test1. */
    printf("%s\n", output);
    /* Reads test1 (this should fail). */
    char output2[strlen(sample)];
    read_file(output2, f, strlen(sample), D);
    printf("There should be nothing after this >> %s\n", output2);
    /* Deletes test1. */
    delete_file("/dir/test1.txt", D);
    /* Opens test1 (this should fail). */
    open_file("/dir/test1.txt", D);

    /* Opens test2 file. */
    f = open_file("/dir/test2.txt", D);
    /* Writes to test2 file. */
    write_file(f, "Sample Text", 12, D);
    /* Closes test2 file. */
    close_file(f);

    /* Unmaps memory. */
    if(munmap(D, sizeof(drive)) == -1) {
      printf("Problem with unmapping.\n");
      exit(1);
    }
  }
return 0;
}
