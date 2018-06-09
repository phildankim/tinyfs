#include "libDisk.h"
#include "tinyFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int createDB(int inodeNum, int size, char *buffer, int pos);
int insertDB(struct inode *inodePtr, FileExtent *new);
int writeToDB(FileExtent *db, char *buffer, int *size, int pos);

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
	printf("\n] CREATING NEW RT ENTRY\n");
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
	printf("\n] SEARCHING FOR RT ENTRY WITH FNAME %s\n", fname);
	int i = 0;
	while (i < rtSize) {
		if (strcmp(rt[i].fname, fname) == 0) {
			printf("-RT Entry found at index %d\n", i);
			return i; /*return fd when found */
		}
		else {
			i += 1;
		}
	}
	fileDescriptor notFound = -1;
	printf("-RT Entry not found.\n");
	return notFound;
}

void printRT() {
	int i;
	ResourceTableEntry ptr;

	for (i = 0; i < rtSize; i++) {
		ptr = rt[i];
		printf("\n] PRINTING RT FOR %s:\n", ptr.fname);
		printf("-fname: %s\n", ptr.fname);
		printf("-fd: %d\n", ptr.fd);
		printf("-inode: %d\n", ptr.inodeNum);
		printf("-opened: %d\n", ptr.opened);
	}
}

void initSB() {
	if (sb == NULL) {
		printf("\n] INITIALIZING SUPERBLOCK\n");
		sb = calloc(1, BLOCKSIZE);
		sb->blockType = SUPERBLOCK;
		sb->magicN = MAGIC_N;
		sb->nextFB = 1;
		sb->rootNode = 0;
		if (writeBlock(mountedDisk, 0, sb) < 0) {
			printf("!!! writeBlock() failed\n");
			return;
		}
		printf("-Initialization of SB to disk %d complete\n", mountedDisk);
	}
}

void updateSB(int bNum) { 
	printf("\n] UPDATING SUPER BLOCK\n");

	if (numFreeBlocks <= 0) {
		printf("!!! no more FBs\n");
		return;
	}

	freeblock *ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, bNum, ptr);
	int next = ptr->nextBlockNum;
	sb->nextFB = next;
	
	numFreeBlocks -= 1;

	if (writeBlock(mountedDisk, 0, sb) < 0) {
		printf("in updateSB() -- write failed\n");
		return;
	}
	printf("-Successfully updated SB next freeBlock: %d\n", next);
	free(ptr);
}

void initFBList(int nBytes) {
	printf("\n] INITIALIZING FREE BLOCK LIST\n");
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
	printf("\n] PRINTING SUPER BLOCK\n");
	printf("-Block type: %d\n", sb->blockType);
	printf("-Next free block: %d\n", sb->nextFB);
	printf("-Root inode: %d\n", sb->rootNode);
}

void printFB(freeblock *fb) {
	printf("\n] PRINTING FREE BLOCK\n");
	printf("-Block type:  %d\n", fb->blockType);
	printf("-Block num: %d\n", fb->blockNum);
	printf("-Next block num: %d\n\n", fb->nextBlockNum);
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
	printf("\n] CREATING NEW INODE FOR FILE %s\n", fname);
	inode *new = malloc(BLOCKSIZE);
	new->blockType = INODE;
	new->magicN = MAGIC_N;
	strcpy(new->fname, fname);
	new->fSize = 0;
	new->data = 0; // FileExtent has not been created yet at this point
	new->next = 0; 

	int nextFB;
	if (sb->rootNode == 0) { // root inode has not yet been initialized
		printf("-Root inode has not been initialized yet.\n-Initializing.\n");
		nextFB = 1; // guaranteed that if inode not init, first FB is at block 1
		new->blockNum = nextFB;
		sb->rootNode = nextFB;
	}
	else {
		nextFB = sb->nextFB; 
		new->blockNum = nextFB;
	}

	updateSB(nextFB);
	insertIN(new->blockNum);

	if (writeBlock(mountedDisk, new->blockNum, new) < 0) {
		printf("!!! writeBlock failed -- exiting\n");
		return -7;
	}
	return new->blockNum;
}

void insertIN(uint8_t new) {
	printf("\n] INSERTING INODE\n");
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
	printf("-New inode successfully insert at block %d\n", new);
	free(ptr);
}

void printIN(fileDescriptor fd) {
	
	inode *ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, rt[fd].inodeNum, ptr);
	printf("\n] PRINTING INODE FOR %s:\n", ptr->fname);
	printf("-Block num: %d\n", ptr->blockNum);
	printf("-File name: %s\n", ptr->fname);
	printf("-File size: %d\n", ptr->fSize);
	printf("-Block num of first data block: %d\n", ptr->data);
	printf("-Block num of next inode: %d\n", ptr->next);

	free(ptr);
}

int createDB(int inodeNum, int size, char *buffer, int pos) {
	printf("\n] CREATING NEW DATA BLOCK\n");

	int nextFB = sb->nextFB;
	updateSB(nextFB);
	FileExtent *new = malloc(BLOCKSIZE);
	new->blockType = FILE_EXTENT;
	new->magicN = MAGIC_N;
	new->blockNum = nextFB;
	new->next = 0;

	writeToDB(new, buffer, &size, pos);

	inode *ptr;
	ptr = malloc(BLOCKSIZE);

	if (readBlock(mountedDisk, inodeNum, ptr) < 0) {
		printf("-readBlock failed -- exiting\n");
		return -1;
	}
	
	if (writeBlock(mountedDisk, new->blockNum, new) < 0) {
		printf("-writeBlock failed -- exiting\n");
		return -1;
	}
	insertDB(ptr, new);
	free(new);
	return 0;
}

int writeToDB(FileExtent *db, char *buffer, int *size, int pos) {
	printf("\n] WRITING TO DATA BLOCK\n");
	int writeAt = pos * DEFAULT_DB_SIZE;

	if (*size > DEFAULT_DB_SIZE) {
		memcpy(db->data, &buffer[writeAt], DEFAULT_DB_SIZE);
		*size -= DEFAULT_DB_SIZE;
		return 0;
	}	
	else {
		memcpy(db->data, &buffer[writeAt], *size);
		*size = 0;
		return 0;
	}
}

int insertDB(inode *inodePtr, FileExtent *new) {
	printf("\n] INSERTING DATA BLOCK\n");
	FileExtent *ptr = malloc(BLOCKSIZE);

	if (inodePtr->data == 0) {
		inodePtr->data = new->blockNum;
		writeBlock(mountedDisk, inodePtr->blockNum, inodePtr);
		return 0;
	}
	else {
		readBlock(mountedDisk, inodePtr->data, ptr);
		int i = 0;

		while (ptr->next != 0 || i < 10) {
			readBlock(mountedDisk, ptr->next, ptr);
			i++;
		}

		if (ptr->next == 0) {
			ptr->next = new->blockNum;
			writeBlock(mountedDisk, ptr->next, new);
			return 0;
		}

		else {
			printf("-insertDB -- what the fuck?\n");
			return -12;
		}
		
	}
	free(ptr);
	return 0;
}



/////////////////////////
/// tinyFS OPERATIONS ///
/////////////////////////

int tfs_mkfs(char *filename, int nBytes) {
	printf("\n] CREATING NEW FILE SYSTEM ON DISK NAME %s\n", filename);
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

	initFBList(nBytes);
	initSB();
	return 0;
}

int tfs_mount(char *filename) {
	printf("\n] MOUNTING DISK NAME %s\n", filename);
	if (mountedDisk != UNMOUNTED) {
		printf("-there is already a mounted disk\n");
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
	writeBlock(mountedDisk, 0, sb);
	// to build back the dynamic resource table back up,
	// iterate through inodes and create rt entry
	return 0;
}
int tfs_unmount(void) {
	printf("\n] UNMOUNTING DISK\n");
	if (mountedDisk == UNMOUNTED) {
		printf("nothing to unmount -- exiting\n");
		return -5;
	}
	mountedDisk = UNMOUNTED;
	sb = NULL;
	// memset the entire dynamic resource table
	return 0;
}

fileDescriptor tfs_openFile(char *name) {
	printf("\n] OPENING FILE %s\n", name);
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
	printf("\n] WRITING FILE\n");
	if (!rt[FD].opened) {
		printf("in tfs_writeFile() -- file not opened\n");
		return -9;
	}

	inode *inodePtr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, rt[FD].inodeNum, inodePtr);
	inodePtr->fSize = size;
	writeBlock(mountedDisk, rt[FD].inodeNum, inodePtr);

	int numDataBlocks = 1; // guaranteed to need at least one
	numDataBlocks += floor(size / DEFAULT_DB_SIZE);

	int i;
	int sizeRemaining = size;
	for (i = 0; i < numDataBlocks; i++) {
		createDB(rt[FD].inodeNum, sizeRemaining, buffer, i);
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

	ResourceTableEntry entry = rt[FD];
	inode *inodePtr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, entry.inodeNum, inodePtr); /* grab inode */
	free (inodePtr);
	//rt[FD] = NULL;

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

	blockNum = floor(offset / DEFAULT_DB_SIZE); 
	byte = offset % DEFAULT_DB_SIZE;

	rt[FD].blockOffset = blockNum;
	rt[FD].byteOffset = byte;

	return 0;
}

int main() {
	// TESTING MKFS
	printf("----- TESTING tfs_mkfs() -----");
	char *fname2 = "test.txt"; 
	int res = tfs_mkfs(fname2, DEFAULT_DISK_SIZE);
	printSB();
	
	// TESTING OPENFILE
	printf("\n----- TESTING tfs_openFile() -----");
	char *fname3 = "a.txt";
	int fd3 = tfs_openFile(fname3);
	printf("-File descriptor for %s is %d\n", fname3, fd3);
	printRT();
	printIN(fd3);

	// Test for multiple files
	char *fname4 = "b.txt";
	int fd4 = tfs_openFile(fname4);
	printf("-File descriptor for %s is %d\n", fname4, fd4);
	printRT();
	printIN(fd4);

	// Test if existing file can be found
	tfs_openFile("a.txt");

	/*
	char *fname4 = "b.txt";
	printf("] Opening %s\n", fname4);
	fd = tfs_openFile(fname4);
	printf("fd for file %s: %d\n\n", fname4, fd);
	printIN(fd);
	printSB();
	*/

	return 0;
}