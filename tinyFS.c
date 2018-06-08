#include "libDisk.h"
#include "tinyFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int createDB(int inodeNum);
int insertDB(struct inode *inodePtr, FileExtent *new);

superblock *sb = NULL;
ResourceTableEntry rt[DEFAULT_RT_SIZE];
/*OpenFileTable ot[DEFAULT_OT_SIZE];*/

int mountedDisk = UNMOUNTED; // this is for mount/unmount, keeps track of which disk to operate on
int numFreeBlocks = 40;
int rtSize = 0;

/////////////////////////
/// STRUCT OPERATIONS ///
/////////////////////////

int createRTEntry(char *fname, uint8_t inodeNum) { // incomplete, also no way of knowing if we have enough space in RT for more
	int res;
	rt[rtSize].fd = rtSize;
	res = rt[rtSize].fd;
	strcpy(rt[rtSize].fname, fname);
	rt[rtSize].opened = 0; /*not yet opened */
	rt[rtSize].inodeNum = inodeNum; 
	rt[rtSize].blockOffset = 0;
	rt[rtSize].byteOffset = 0;
	rtSize += 1;

	return res;
}

fileDescriptor searchRT(char *fname){
	int i = 0;
	while (i < rtSize) {
		if (strcmp(rt[i].fname, fname) == 0) {
			return i; /*return fd when found */
		}
		else {
			i += 1;
		}
	}
	fileDescriptor notFound = -1;
	return notFound;
}

void printRT() {
	int i;
	ResourceTableEntry ptr;
	printf("\n] Printing RT:\n");
	for (i = 0; i < rtSize; i++) {
		ptr = rt[i];
		printf("\nfname: %s\n", ptr.fname);
		printf("fd: %d\n", ptr.fd);
		printf("inode: %d\n", ptr.inodeNum);
		printf("opened: %d\n", ptr.opened);
	}
}

void initSB() {
	if (sb == NULL) {
		sb = calloc(1, BLOCKSIZE);
		sb->blockType = SUPERBLOCK;
		sb->magicN = MAGIC_N;
		sb->nextFB = 1;
		sb->rootNode = 0;
		writeBlock(mountedDisk, 0, sb);
	}
}

/* reads in FB at given bNum spot,
   takes that FB's "next" FB,
   then update SB's next FB to that
*/
void updateSB(int bNum) { 
	if (numFreeBlocks <= 0) {
		printf("in updateSB() -- no more FBs\n");
		return;
	}
	freeblock *ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, bNum, ptr);
	int next = ptr->nextBlockNum;
	sb->nextFB = next;

	numFreeBlocks -= 1;

	writeBlock(mountedDisk, 0, ptr);
	free(ptr);
}

void initFBList(int nBytes) {
	int bNum = nBytes / BLOCKSIZE;
	int i;
	for (i = 0; i < bNum; i++) {
		freeblock *newFB = calloc(bNum, BLOCKSIZE);
		newFB->blockType = FREE_BLOCK;
		newFB->magicN = MAGIC_N;
		newFB->blockNum = i;
		newFB->nextBlockNum = i + 1;
		writeBlock(mountedDisk, i, newFB);
	}
}

void printSB() {
	printf("] Printing super block\n");
	printf("Block type: %d\n", sb->blockType);
	printf("Next free block: %d\n", sb->nextFB);
	printf("Root inode: %d\n", sb->rootNode);
}

void printFB(freeblock *fb) {
	printf("] Printing free block\n");
	printf("Block type:  %d\n", fb->blockType);
	printf("Block num: %d\n", fb->blockNum);
	printf("Next block num: %d\n\n", fb->nextBlockNum);
}

void printAllFB() {
	freeblock *ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, sb->nextFB, ptr);

	while (ptr->nextBlockNum < numFreeBlocks) {
		printFB(ptr);
		readBlock(mountedDisk, ptr->nextBlockNum, ptr);
	}
	free(ptr);
}

int createIN(char *fname) { // returns inode blocknum, also incomplete
	inode *new = malloc(BLOCKSIZE);
	new->blockType = INODE;
	new->magicN = MAGIC_N;
	strcpy(new->fname, fname);
	new->fSize = 0;
	new->data = 0; // FileExtent has not been created yet at this point
	new->next = 0; 

	int nextFB;
	if (sb->rootNode == 0) { // root inode has not yet been initialized
		nextFB = 1; // guaranteed that if inode not init, first FB is at block 1
		new->blockNum = nextFB;
		sb->rootNode = nextFB;
		printf("in creatIN(): \nsb->rootNode = %d\n", sb->rootNode);
	}
	else {
		nextFB = sb->nextFB; 
		new->blockNum = nextFB;
	}

	updateSB(nextFB);
	insertIN(new->blockNum);

	if (writeBlock(mountedDisk, new->blockNum, new) < 0) {
		printf("in createIN - writeBlock failed -- exiting\n");
		return -7;
	}
	return new->blockNum;
}

void insertIN(uint8_t new) {
	inode *ptr = malloc(BLOCKSIZE);
	if (readBlock(mountedDisk, sb->rootNode, ptr) < 0) {
		printf("in insertIN -- readBlock failed\n");
	}

	while (ptr->next != 0) {
		readBlock(mountedDisk, ptr->next, ptr);
		printf("ptr->next: %d\n", ptr->next);
	}
	ptr->next = new;
	writeBlock(mountedDisk, ptr->blockNum, ptr);
	printf("inode inserted correctly at bNum: %d\n", ptr->next);
	free(ptr);
}

void printIN(fileDescriptor fd) {
	printf("Printing inode:\n");
	inode *ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, rt[fd].inodeNum, ptr);
	printf("Block num: %d\n", ptr->blockNum);
	printf("File name: %s\n", ptr->fname);
	printf("File size: %d\n", ptr->fSize);
	printf("Block num of first data block: %d\n", ptr->data);
	printf("Block num of next inode: %d\n\n", ptr->next);

	free(ptr);
}

int createDB(int inodeNum) {
	printf("in createDB()\n");
	inode *ptr;
	ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, inodeNum, ptr);

	int nextFB = sb->nextFB;
	FileExtent *new = malloc(BLOCKSIZE);
	new->blockType = FILE_EXTENT;
	new->magicN = MAGIC_N;
	new->blockNum = nextFB;
	new->next = 0;

	updateSB(nextFB);
	writeBlock(mountedDisk, new->blockNum, new);
	insertDB(ptr, new);
	free(new);
}

int insertDB(inode *inodePtr, FileExtent *new) {
	FileExtent *ptr = malloc(BLOCKSIZE);

	if (inodePtr->data == 0) {
		inodePtr->data = new->blockNum;
		writeBlock(mountedDisk, inodePtr->blockNum, inodePtr);
		return 0;
	}
	else {
		readBlock(mountedDisk, inodePtr->data, ptr);
		printf("inode data root: %d\n", inodePtr->data);
		printf("in insertDB\n");
		int i = 0;
		while (ptr->next != 0 || i < 10) {
			printf("ptr->next %d\n", ptr->next);
			readBlock(mountedDisk, ptr->next, ptr);
			printf("lol\n");
			i++;
		}
		if (ptr->next == 0) {
			ptr->next = new->blockNum;
			writeBlock(mountedDisk, ptr->next, new);
			return 0;
		}
		else {
			printf("insertDB -- what the fuck?\n");
			return -12;
		}
		
	}
	free(ptr);
	return 0;
}
/////////////////////////
/// tinyFS OPERATIONS ///
/////////////////////////

/* Makes a blank TinyFS file system of size nBytes on the file specified by ‘filename’. 
This function should use the emulated disk library to open the specified file, and upon success, format the file to be mountable. 
This includes initializing all data to 0x00, setting magic numbers, initializing and writing the superblock and inodes, etc. 
Must return a specified success/error code. */
int tfs_mkfs(char *filename, int nBytes) {
	int disk = openDisk(filename, nBytes);
	if (disk == -1) {
		printf("error - invalid file\n");
		return -1;
	}
	if (disk == -2) {
		printf("error - invalid nBytes\n");
		return -2;
	}

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
	printf("] Mounting %s\n", filename);
	if (mountedDisk != UNMOUNTED) {
		printf("there is already a mounted disk\n");
		return -8;
	}

	int disk = searchDisk(filename);
	if (disk < 0) {
		printf("in tfs_mount() -- disk not found\n");
		return -7;
	}
	if (disk == -1) {
		printf("error - invalid file\n");
		return -1;
	}
	if (disk == -2) {
		printf("error - invalid nBytes\n");
		return -2;
	}
	char buf[BLOCKSIZE];
	readBlock(disk, 0, buf);

	if (buf[1] != MAGIC_N) {
		return -4; // not the magic number
	}

	mountedDisk = disk;
	return 0;
}
int tfs_unmount(void) {
	printf("] Unmounting disk\n");
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
fileDescriptor tfs_openFile(char *name) {
	if (mountedDisk == UNMOUNTED) {
		printf("nothing mounted -- exiting\n");
		return -5;
	} 

	if (sizeof(name) > FNAME_LIMIT) {
		printf("file name too long -- exiting\n");
		return -6;
	}

	fileDescriptor fd = searchRT(name); //check if file already in table
	if (fd < 0) { // not exists
		int inodeBlockNum = createIN(name); // create inode
		fd = createRTEntry(name, inodeBlockNum);

		/****** SET FILE AS OPEN **************/
		rt[fd].opened = 1;
		return fd;
	}
	else { // exists
		return fd;
	}
}

/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD);

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the file system. 
Sets the file pointer to 0 (the start of file) when done. 
Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
	// check if FD is open or not
	printf("\n -- in tfs_writeFile() --\n");
	if (!rt[FD].opened) {
		printf("in tfs_writeFile() -- file not opened\n");
		return -9;
	}

	inode *inodePtr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, rt[FD].inodeNum, inodePtr);
	inodePtr->fSize = size;
	printf("inode new fsize: %d\n", inodePtr->fSize);
	writeBlock(mountedDisk, rt[FD].inodeNum, inodePtr);

	int numDataBlocks = 1; // guaranteed to need at least one
	numDataBlocks += floor(size / BLOCKSIZE);
	printf("number of data blocks needed: %d\n", numDataBlocks);
	int i;
	for (i = 0; i < numDataBlocks; i++) {
		createDB(rt[FD].inodeNum);
	}
	// how do i read with an offset?
	writeBlock(mountedDisk, inodePtr->data, buffer); // for testing purposes, just writing to the root data block

	// set file pointer to 0
	rt[FD].blockOffset = 0;
	rt[FD].byteOffset = DATA_HEADER_OFFSET;
	// acquire data block (fileExtent) remaining size
	// allocate correct number of FileExtent depending on how much space is needed ^^
	// writeBlock(mountedDisk, ..., buffer);
	// update superblock
	// update inode
	// update resource table??
	return 0;
}

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD) {
	ResourceTableEntry *entry = rt[FD];
	inode *inodePtr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, entry.inodeNum, inodePtr); /* grab inode */
	free (inodePtr);

	free(entry);
	rt[FD] = NULL;

	return 0;
}

/* Reads one byte from the file and copies it to buffer, 
using the current file pointer location and incrementing it by one upon success. 
If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer) {
	inode *ptr = malloc(BLOCKSIZE);
	int inodeBNum = rt[FD].inodeNum;
	readBlock(mountedDisk, inodeBNum, ptr);

	int blockOffset = rt[FD].blockOffset;
	int byteOffset = rt[FD].byteOffset;
	int currPos = (blockOffset * BLOCKSIZE) + byteOffset;
	
	if (currPos >= ptr->fSize) {
		printf("in readByte() -- file ptr already at end of file\n");
		return -9;
	}

	char *fileBuf = malloc(BLOCKSIZE);

	// find the correct logical block num
	// this can probably be decomposed into another function
	// actually, a lot of this shit could lol
	int i = 0;
	FileExtent *fp = malloc(BLOCKSIZE);
	readBlock(mountedDisk, ptr->data, fp);
	while (i < blockOffset) {
		readBlock(mountedDisk, fp->next, fp);
	}

	readBlock(mountedDisk, fp->blockNum, fileBuf);
	//strcpy(fileBuf[byteOffset], buffer);
	printf(" -------- readByte string: %s\n", fileBuf);
	printf(" -------- readByte: %x\n", fileBuf[byteOffset]);
	printf(" blockOFfset: %d\n", blockOffset);
	rt[FD].byteOffset += 1;
	return 0;
}

/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset) { 
	printf("] in tfs_seek():\n");
	int current;
	int blockNum;
	int byte;
	int i = 0;

	if (!rt[FD].opened) {
		printf("in tfs_seek() -- file not opened\n");
		return -10;
	}
	//FileExtent ptr = malloc(BLOCKSIZE);
	FileExtent *head = malloc(BLOCKSIZE);
	inode *inodePtr = malloc(BLOCKSIZE);

	current = rt[FD].inodeNum;

	readBlock(mountedDisk, current, inodePtr); /* grab inode */
	readBlock(mountedDisk, inodePtr->data, head); /* grab first fileextent */

	if (inodePtr->fSize <= offset) {
		printf("in tfs_seek() -- offset greater than filesize\n");
		return -9;
	}

	blockNum = floor(offset / BLOCKSIZE); // -1?
	printf("blockOffset: %d\n", blockNum);
	byte = offset % BLOCKSIZE;
	printf("byteOffset: %d\n", byte);

	rt[FD].blockOffset = blockNum;
	rt[FD].byteOffset = byte;

	return 0;
}

int main() {
	// TESTING MKFS
	printf("-- TESTING tfs_mkfs() --\n");
	char *fname1 = "tinyFSDisk";

	printf("\n] Opening disk (file) %s\n", fname1);
	int res = tfs_mkfs(fname1, DEFAULT_DISK_SIZE);
	printf("Disk num for %s: %d\n", fname1, searchDisk(fname1));
	printSB();

	tfs_unmount();
	printf("Currently mounted: %d\n", mountedDisk);
	tfs_mount("tinyFSDisk");
	printf("Currently mounted: %d\n", mountedDisk);

	char *fname2 = "test.txt";
	printf("\n] Opening disk (file) %s\n", fname2);
	res = tfs_mkfs(fname2, DEFAULT_DISK_SIZE);
	printf("Disk num for %s: %d\n", fname2, searchDisk(fname2));
	printSB();
	printf("Currently mounted: %d\n", mountedDisk);

	// TESTING OPENFILE
	printf("\n-- TESTING tfs_openFile() --\n");
	char *fname3 = "a.txt";
	printf("] Opening %s\n", fname3);
	int fd = tfs_openFile(fname3);
	printf("fd for file %s: %d\n\n", fname3, fd);

	char *fname4 = "b.txt";
	printf("] Opening %s\n", fname4);
	fd = tfs_openFile(fname4);
	printf("fd for file %s: %d\n\n", fname4, fd);
	printIN(fd);

	char *writeFileBuf = "1bbbbbbbbbbbbbbbbbbbbbbbbbb";
	char *readBuf = malloc(BLOCKSIZE);
	tfs_writeFile(fd, writeFileBuf, 30);
	tfs_readByte(fd, readBuf);
	printf("readByte: %s\n", readBuf);
	
	//printAllFB();
	//printSB();

	/*
	printf("\n-- TESTING tfs_openFile() --\n");
	fileDescriptor fd = tfs_openFile("a.txt");
	printf("\nfd for a.txt: %d\n", fd);
	fd = tfs_openFile("b.txt");
	printf("\nfd for b.txt: %d\n", fd);
	fd = tfs_openFile("a.txt");
	printf("\nfd for a.txt: %d\n", fd);
	//printSB();
	//printRT();
	*/

	//res = tfs_mkfs("disk2", DEFAULT_DISK_SIZE);
	//printAllFB();
	//printSB();


	return 0;
}