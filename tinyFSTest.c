#include "tinyFS.h"
#include "libDisk.h"

int main() {
	int res = tfs_mkfs("tinyFSDisk", DEFAULT_DISK_SIZE);
	freeblock *buf = malloc(BLOCKSIZE);
	readBlock(mountedDisk, 0, buf);
	printf("%d\n", buf->nextBlockNum);

	return 0;
}