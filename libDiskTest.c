#include "libDisk.h"

int main() {
   int d1 = openDisk("test.txt", BLOCKSIZE);
   printf("%d\n", d1);
   int d2 = openDisk("test1.txt", BLOCKSIZE);
   printf("%d\n", d2);
   char *buf = malloc(sizeof(int) * BLOCKSIZE);
   int res = readBlock(d1, 1, buf);
   printf("%s\n", buf);
   char *buf2 = "poo";
   writeBlock(d2, 1, buf);
   
   return 0;
}