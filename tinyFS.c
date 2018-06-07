#include "libDisk.h"
#include "tinyFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	rt[rtSize].inodeNum = inodeNum; //this is not complete
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
	printf("In updateSB(): \n");
	freeblock *ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, bNum, ptr);
	int next = ptr->nextBlockNum;
	sb->nextFB = next;

	printf("new nextFB: %d\n", sb->nextFB);

	numFreeBlocks -= 1;

	printf("numFreeBlocks: %d\n", numFreeBlocks);
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
	new->data = -1; // FileExtent has not been created yet at this point
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
	if (writeBlock(mountedDisk, new->blockNum, new) < 0) {
		printf("in createIN - writeBlock failed -- exiting\n");
		return -7;
	}
	insertIN(new->blockNum);
	return new->blockNum;
}

void insertIN(uint8_t new) {
	inode *ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, sb->rootNode, ptr);
	while (ptr->next != 0) {
		readBlock(mountedDisk, ptr->next, ptr);
	}
	ptr->next = new;
	printf("inode inserted correctly at bNum: %d\n", ptr->next);
	free(ptr);
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
	// acquire data block (fileExtent) remaining size
	// allocate correct number of FileExtent depending on how much space is needed ^^
	// writeBlock(mountedDisk, ..., buffer);
	// update superblock
	// update inode
	// update resource table??
	return 0;
}

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD);

/* Reads one byte from the file and copies it to buffer, 
using the current file pointer location and incrementing it by one upon success. 
If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer);

/* change the file pointer location to offset (absolute). Returns success/error codes.*/
int tfs_seek(fileDescriptor FD, int offset) { /*not done (obviously)*/
	



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