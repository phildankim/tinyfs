#include "libDisk.h"
#include "tinyFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

superblock *sb = NULL;
int mountedDisk = UNMOUNTED; // this is for mount/unmount, keeps track of which disk to operate on
int numFreeBlocks = 40;


void initSB() {
	if (sb == NULL) {
		sb = calloc(1, BLOCKSIZE);
	}
	sb->blockType = SUPERBLOCK;
	sb->magicN = 0x45;
	sb->nextFB = 1;
	writeBlock(mountedDisk, 0, sb);
}

void initFBList(int nBytes) {
	int bNum = nBytes / BLOCKSIZE;
	int i;
	for (i = 0; i < bNum; i++) {
		freeblock *newFB = calloc(bNum, BLOCKSIZE);
		newFB->blockType = FREEBLOCK;
		newFB->magicN = 0x45;
		newFB->blockNum = i;
		newFB->nextBlockNum = i + 1;
		//printf("%d\n", newFB->blockNum);
		writeBlock(mountedDisk, i, newFB);
	}
}

int errorCheck(int errno) {
	if (errno == -1) {
		printf("error - invalid file\n");
		return -1;
	}
	if (errno == -2) {
		printf("error - invalid nBytes\n");
		return -2;
	}
}

/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. 
This function should use the emulated disk library to open the specified file, and upon success, format the file to be mountable. 
This includes initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. 
Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes) {
	int disk = openDisk(filename, nBytes);
	errorCheck(disk);

	mountedDisk = disk;

	//init nBytes of disk to 0x00, this is done by openDisk()
	//initialize freeblocks list
	initFBList(nBytes);
	//initialize superblock
	initSB();
	return 0;
}

/* tfs_mount(char *filename) “mounts” a TinyFS file system located within ‘filename’. 
tfs_unmount(void) “unmounts” the currently mounted file system. 
As part of the mount operation, tfs_mount should verify the file system is the correct type. 
Only one file system may be mounted at a time. 
Use tfs_unmount to cleanly unmount the currently mounted file system. 
Must return a specified success/error code. */
int tfs_mount(char *filename) {
	// this is just a matter of grabbing the fd correspondent to the filename
	// and using that fd to read/write
	if (mountedDisk == UNMOUNTED) {
		tfs_unmount();
	}

	int disk = openDisk(filename, 0);
	errorCheck(disk);
	char buf[BLOCKSIZE];
	readBlock(disk, 0, buf);

	if (buf[1] != 0x45) {
		return -4; // not the magic number
	}

	mountedDisk = disk;
	return 0;
}
int tfs_unmount(void) {
	if (mountedDisk == UNMOUNTED) {
		printf("nothing to unmount -- exiting\n");
		return -5;
	}
	mountedDisk = UNMOUNTED;
	return 0;
}

/* Opens a file for reading and writing on the currently mounted file system. 
Creates a dynamic resource table entry for the file, and returns a file descriptor (integer) 
that can be used to reference this file while the filesystem is mounted. */
fileDescriptor tfs_openFile(char *name);

/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD);

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the file system. 
Sets the file pointer to 0 (the start of file) when done. 
Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD);

/* Reads one byte from the file and copies it to buffer, 
using the current file pointer location and incrementing it by one upon success. 
If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset) {

}

int main() {
	int res = tfs_mkfs("tinyFSDisk", DEFAULT_DISK_SIZE);
	freeblock *buf = malloc(BLOCKSIZE);
	readBlock(mountedDisk, 1, buf);

	while (buf->nextBlockNum < numFreeBlocks) {
		printf("block type: %d\tblock num %d\n", buf->blockType, buf->blockNum);
		readBlock(mountedDisk, buf->nextBlockNum, buf);
	}
	return 0;
}
