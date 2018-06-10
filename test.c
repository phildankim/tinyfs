#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#define BLOCKSIZE 256

typedef struct inode {
	
	time_t creationTime;
	time_t accessTime;
	time_t modificationTime;
} inode;

int tfs_rename(char *filename, int FD, inode *node) {
	
    node->creationTime=time(NULL); /* get current cal time */
    printf("%s",asctime( localtime(&node->creationTime) ) );
    return 0;
}

int main () {

	inode *node = malloc(BLOCKSIZE);
	tfs_rename("12345678", 5, node);

	return 0;
}
