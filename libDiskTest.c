#include "libDisk.h"
#include "tinyFS.h"

superblock *sb = NULL;
ResourceTableEntry *top = NULL;
int mountedDisk = UNMOUNTED; // this is for mount/unmount, keeps track of which disk to operate on
int numFreeBlocks = 40;

int main() {
   /*
   int d1 = openDisk("test.txt", BLOCKSIZE);
   printf("%d\n", d1);
   int d2 = openDisk("test1.txt", BLOCKSIZE);
   printf("%d\n", d2);
   int d3 = openDisk("test1.txt", BLOCKSIZE);
   printf("%d\n", d3);

   char *buf = malloc(sizeof(int) * BLOCKSIZE);
   int res = readBlock(d1, 0, buf);
   printf("%s\n", buf);

   char *buf2 = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
   writeBlock(d2, 0, buf2);

   int d4 = openDisk("empty", BLOCKSIZE);
   printf("%d\n", d4);
   writeBlock(d4, 0, buf2);
   */

   //d4 = openDisk("empty", BLOCKSIZE);
   return 0;

}