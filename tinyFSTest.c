#include "tinyFS.h"
#include "libDisk.h"

int main() {
	superblock *sb = NULL;
	ResourceTableEntry rt[DEFAULT_RT_SIZE];
	OpenFileTable ot[DEFAULT_OT_SIZE];

	int mountedDisk = UNMOUNTED; // this is for mount/unmount, keeps track of which disk to operate on
	int numFreeBlocks = 40;
	int rtSize = 0;
	int otSize = 0;

	// TESTING

	printf("-- TESTING tfs_mkfs() --\n");
	int res = tfs_mkfs("tinyFSDisk", DEFAULT_DISK_SIZE);
	freeblock *buf = malloc(BLOCKSIZE);
	readBlock(mountedDisk, 1, buf);
	printf("block type: %d\tblock num %d\n", buf->blockType, buf->blockNum);

	/*
	while (buf->nextBlockNum < 10) {
		printf("block type: %d\tblock num %d\n", buf->blockType, buf->blockNum);
		readBlock(mountedDisk, buf->nextBlockNum, buf);
	}
	*/
	return 0;
}