#include "libDisk.h"
#include "tinyFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int createDB(int inodeNum, int *size, char *buffer, int pos);
int insertDB(struct inode *inodePtr, FileExtent *new);
int writeToDB(FileExtent *db, char *buffer, int *size, int pos);

superblock *sb = NULL;
ResourceTableEntry rt[DEFAULT_RT_SIZE];

int mountedDisk = UNMOUNTED; // this is for mount/unmount, keeps track of which disk to operate on

/**********************************************************************************/
/////////////////////////
/// STRUCT OPERATIONS ///
/////////////////////////
/**********************************************************************************/

int createRTEntry(char *fname, uint8_t inodeNum) { // incomplete, also no way of knowing if we have enough space in RT for more
	int error;
	printf("\n] CREATING NEW RT ENTRY\n");
	int res;
	int currRTSize = sb->rtSize;
	rt[currRTSize].fd = currRTSize;
	res = rt[currRTSize].fd;
	strcpy(rt[currRTSize].fname, fname);
	rt[currRTSize].opened = 0; /*not yet opened */
	rt[currRTSize].inodeNum = inodeNum; 
	rt[currRTSize].blockOffset = 0;
	rt[currRTSize].byteOffset = 0;
	sb->rtSize += 1;
	error = writeBlock(mountedDisk, 0, sb);
	if (error < 0){
		return error;
	}
	return res;
}

fileDescriptor searchRT(char *fname){
	printf("\n] SEARCHING FOR RT ENTRY WITH FNAME %s\n", fname);
	int i = 0;
	while (i < sb->rtSize) {
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

int buildRT() {
	printf("\n] BUILDING RESOURCE TABLE\n");

	if (sb->rootNode == 0) { // inodes have not been initialized
		
		return ROOT_NOT_INITIALIZED;
	}

	inode *inodePtr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, sb->rootNode, inodePtr);

	while (inodePtr->next != 0) {	
		printf("-Next inode at: %d\n", inodePtr->next);
		createRTEntry(inodePtr->fname, inodePtr->blockNum);
		readBlock(mountedDisk, inodePtr->next, inodePtr);
	}
	// one more for the last inode
	createRTEntry(inodePtr->fname, inodePtr->blockNum);
	printf("-Resource Table successfully built.\n");
	free(inodePtr);
}

void printRT() {
	int i;
	ResourceTableEntry ptr;

	for (i = 0; i < sb->rtSize; i++) {
		ptr = rt[i];
		printf("\n] PRINTING RT FOR %s:\n", ptr.fname);
		printf("-fname: %s\n", ptr.fname);
		printf("-fd: %d\n", ptr.fd);
		printf("-inode: %d\n", ptr.inodeNum);
		printf("-opened: %d\n", ptr.opened);
	}
}

int initSB(int fb) {
	int errNO;
	printf("\n] INITIALIZING SUPERBLOCK\n");
	sb = calloc(1, BLOCKSIZE);
	sb->blockType = SUPERBLOCK;
	sb->magicN = MAGIC_N;
	sb->nextFB = 1;
	sb->rootNode = 0;
	sb->numFreeBlocks = fb;
	sb->rtSize = 0;
	errNO = writeBlock(mountedDisk, 0, sb);
	if (errNO < 0) {
		printf("!!! writeBlock() failed\n");
		return errNO;
	}
	printf("-Initialization of SB to disk %d complete\n", mountedDisk);
}

int updateSB(int bNum) { 
	int error;
	printf("\n] UPDATING SUPER BLOCK\n");

	if (sb->numFreeBlocks <= 0) {
		printf("!!! no more FBs\n");
		return NO_FREE_BLOCKS;
	}

	freeblock *ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, bNum, ptr);
	int next = ptr->nextBlockNum;
	sb->nextFB = next;
	
	sb->numFreeBlocks -= 1;
	printf("-Free blocks remaining: %d\n", sb->numFreeBlocks);
	error = writeBlock(mountedDisk, 0, sb);
	if (error < 0) {
		printf("in updateSB() -- write failed\n");
		return error;
	}
	printf("-Successfully updated SB next freeBlock: %d\n", next);
	free(ptr);
}

void printSB() {
	printf("\n] PRINTING SUPER BLOCK\n");
	printf("-Block type: %d\n", sb->blockType);
	printf("-Next free block: %d\n", sb->nextFB);
	printf("-Root inode: %d\n", sb->rootNode);
}

int initFBList(int nBytes) {
	int error;
	printf("\n] INITIALIZING FREE BLOCK LIST\n");
	int bNum = nBytes / BLOCKSIZE;
	int i;
	for (i = 0; i < bNum; i++) {
		freeblock *newFB = calloc(bNum, BLOCKSIZE);
		newFB->blockType = FREE_BLOCK;
		newFB->magicN = MAGIC_N;
		newFB->blockNum = i;
		newFB->nextBlockNum = i + 1;
		error = writeBlock(mountedDisk, i, newFB);
		if (error < 0){
			return error;
		}
	}
	freeblock *fb = malloc(BLOCKSIZE);
	readBlock(mountedDisk, bNum-1, fb);
	fb->nextBlockNum = 0;
	writeBlock(mountedDisk, bNum-1, fb);
	return 0;
}

int insertFB(int blockNum) {
	printf("\n] INSERTING FREE BLOCK AT BLOCK %d\n", blockNum);
	int error;
	freeblock *fp = malloc(BLOCKSIZE);
	error = readBlock(mountedDisk, sb->nextFB, fp);
	if (error <0){
		return error;
	}

	while (fp->nextBlockNum != 0) {
		error = readBlock(mountedDisk, fp->nextBlockNum, fp);
		if (error <0){
			return error;
		}
		
	}

	fp->nextBlockNum = blockNum;
	error = writeBlock(mountedDisk, fp->blockNum, fp);

	if (error < 0){
		return error;
	}
	return 0;
}

void printFB(freeblock *fb) {
	printf("\n] PRINTING FREE BLOCK\n");
	printf("-Block type:  %d\n", fb->blockType);
	printf("-Block num: %d\n", fb->blockNum);
	printf("-Next block num: %d\n\n", fb->nextBlockNum);
}

int printAllFB(int flag) {
	printf("\n] PRINTING ALL FREE BLOCKS\nPRINT ALL INFORMATION: %d\n", flag);
	int error;
	freeblock *ptr = malloc(BLOCKSIZE);
	error = readBlock(mountedDisk, sb->nextFB, ptr);

	if (error <0){
		return error;
	}

	if (flag = 1) { // print everything about fbs
		while (ptr->nextBlockNum < sb->numFreeBlocks) {
			printFB(ptr);
			error = readBlock(mountedDisk, ptr->nextBlockNum, ptr);
			if (error <0){
				return error;
			}
		}
	}
	else {
		while (ptr->nextBlockNum < sb->numFreeBlocks) {
			printf("%d ", ptr->nextBlockNum);
			error = readBlock(mountedDisk, ptr->nextBlockNum, ptr);
			if (error <0){
				return error;
			}
		}
	}
	
	free(ptr);
	return 0;
}

int createIN(char *fname) { // returns inode blocknum, also incomplete
	int error;
	printf("\n] CREATING NEW INODE FOR FILE %s\n", fname);
	inode *new = malloc(BLOCKSIZE);
	new->blockType = INODE;
	new->magicN = MAGIC_N;
	strcpy(new->fname, fname);
	new->data = 0; // FileExtent has not been created yet at this point
	new->next = 0; 
	new->creationTime = time(NULL);
	new->accessTime = 0;
	new->modificationTime = 0;

	int nextFB;
	if (sb->rootNode == 0) { // root inode has not yet been initialized
		printf("-Root inode has not been initialized yet.\n-Initializing.\n");
		nextFB = 1; // guaranteed that if inode not init, first FB is at block 1
		new->blockNum = nextFB;
		sb->rootNode = nextFB;
		writeBlock(mountedDisk, 0, sb);
	}
	else {
		nextFB = sb->nextFB; 
		new->blockNum = nextFB;
	}

	updateSB(nextFB);
	insertIN(new->blockNum);

	error = writeBlock(mountedDisk, new->blockNum, new);
	if (error < 0) {
		printf("!!! writeBlock failed -- exiting\n");
		return error;
	}
	return new->blockNum;
}

int insertIN(uint8_t new) {
	int error;
	printf("\n] INSERTING INODE\n");
	inode *ptr = malloc(BLOCKSIZE);

	error = readBlock(mountedDisk, sb->rootNode, ptr);
	if (error < 0) {
		printf("in insertIN -- readBlock failed\n");
		return error;
	}

	while (ptr->next != 0) {
		error = readBlock(mountedDisk, ptr->next, ptr);
		if (error < 0){
			return error;
		}
		printf("ptr->next: %d\n", ptr->next);
	}

	ptr->next = new;
	error = writeBlock(mountedDisk, ptr->blockNum, ptr);
	if (error < 0) {
		return error;
	}
	printf("-New inode successfully insert at block %d\n", new);
	free(ptr);
	return 0;
}

void printIN(fileDescriptor fd) {
	
	inode *ptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, rt[fd].inodeNum, ptr);
	printf("\n] PRINTING INODE FOR %s:\n", ptr->fname);
	printf("-Block num: %d\n", ptr->blockNum);
	printf("-File name: %s\n", ptr->fname);
	printf("-Block num of first data block: %d\n", ptr->data);
	printf("-Block num of next inode: %d\n", ptr->next);

	free(ptr);
}

int createDB(int inodeNum, int *size, char *buffer, int pos) {
	printf("\n] CREATING NEW DATA BLOCK\n");
	int error;

	inode *ptr;
	ptr = malloc(BLOCKSIZE);
	error = readBlock(mountedDisk, inodeNum, ptr);
	if (error <0){
		return error;
	}

	int nextFB;
	nextFB = sb->nextFB;
	error = updateSB(nextFB);

	// create and init new data block
	FileExtent *new = malloc(BLOCKSIZE);
	new->blockType = FILE_EXTENT;
	new->magicN = MAGIC_N;
	new->blockNum = nextFB;
	new->next = 0;

	// guaranteed to always write to db upon creation
	writeToDB(new, buffer, size, pos);
	printf("-Size of buffer after writing: %d\n", *size);

	// write new data block to disk
	error = writeBlock(mountedDisk, new->blockNum, new);
	if (error <0){
		return error;
	}
	insertDB(ptr, new);
	free(new);
	return new->blockNum;
}

int writeToDB(FileExtent *db, char *buffer, int *size, int pos) {
	printf("\n] WRITING TO DATA BLOCK\n");
	int writeAt = pos * DEFAULT_DB_SIZE;

	if (*size > DEFAULT_DB_SIZE) {
		memcpy(db->data, &buffer[writeAt], DEFAULT_DB_SIZE);
		*size -= DEFAULT_DB_SIZE;
		printf("size is: %d\n",*size);
		printf("-Successfully wrote to DB: %d bytes\n", DEFAULT_DB_SIZE);
		return 0;
	}	
	else {
		memcpy(db->data, &buffer[writeAt], *size);
		*size = 0;
		printf("-Successfully wrote to DB: %d bytes\n", *size);
		return 0;
	}
}

int insertDB(inode *inodePtr, FileExtent *new) {
	printf("\n] INSERTING DATA BLOCK\n");
	int error;
	// if db hasn't been init
	if (inodePtr->data == 0) {
		inodePtr->data = new->blockNum;
		error = writeBlock(mountedDisk, inodePtr->blockNum, inodePtr);
		if (error <0){
			return error;
		}
		return 0;
	}
	else {
		FileExtent *dbPtr = malloc(BLOCKSIZE);
		error = readBlock(mountedDisk, inodePtr->data, dbPtr);
		if (error <0){
			return error;
		}

		//int i = 0;
		while (dbPtr->next != 0 ) {
			//printf("next db: %d\n", dbPtr->next);
			error = readBlock(mountedDisk, dbPtr->next, dbPtr);
			if (error <0){
				return error;
			}
			//i++;
		}

		// this is the condition that should always hit
		if (dbPtr->next == 0) {
			dbPtr->next = new->blockNum;

			// write new DB
			error = writeBlock(mountedDisk, dbPtr->next, new);
			if (error <0){
				return error;
			}
			// update previous DB
			error = writeBlock(mountedDisk, dbPtr->blockNum, dbPtr);
			if (error <0){
				return error;
			}
			free(dbPtr);
			return 0;
		}
		
	}
	return 0;
}

int deallocateDB(int newBlock) {
	int error;
	printf("\n] DEALLOCATING DATABLOCK AT %d\n", newBlock);
	freeblock *newFB = calloc(BLOCKSIZE, sizeof(char));
	newFB->blockType = FREE_BLOCK;
	newFB->magicN = MAGIC_N;
	newFB->blockNum = newBlock;
	newFB->nextBlockNum = 0;
	insertFB(newFB->blockNum);
	error = writeBlock(mountedDisk, newFB->blockNum, newFB);

	sb->numFreeBlocks += 1;
	writeBlock(mountedDisk, 0, sb);
	if (error <0){
		return error;
	}
}



/**********************************************************************************/

/////////////////////////
/// PART 3 OPERATIONS ///
/////////////////////////

/**********************************************************************************/

/* renames a file.  New name should be passed in. */
int tfs_rename(char *filename, int FD) {
	inode *inodePtr = malloc(BLOCKSIZE);
	int num;
	int error;
	int length = sizeof(filename);
	if (length > 8){
		printf("tfs_rename -- file name too long\n");
		return INVALID_FNAME;
	} 

	num = rt[FD].inodeNum;

	error = readBlock(mountedDisk, num, inodePtr); /* grab inode */
	if (error < 0){
		return error;
	}
	strcpy(rt[FD].fname, filename);
	strcpy(inodePtr->fname, filename);

	return 0;
} 

/* lists all the files and directories on the disk */
int tfs_readdir() {
	int i;

	if (sizeof(rt == 0)){
		printf("No files currently on the disk.\n");
		return NO_FILES_ON_DISK;
	}
	for (i=0; i<sizeof(rt); i++){
		if (rt[i].deleted != 0){ /* make sure file not deleted */
			printf("%s\n", rt[i].fname);
		}
		
	}
	return 0;
}

/* makes the file read only. If a file is RO, all tfs_write() and tfs_deleteFile() functions that try to use it fail. */ 
int tfs_makeRO(char *name) {
	fileDescriptor fd = searchRT(name); //check if file already in table
	if (fd < 0){
		printf("tfs_makeRO -- file does not exist.\n");
		return INVALID_FD;
	}

	rt[fd].readOnly = 1;

	return 0;

}
/* makes the file read-write */
int tfs_makeRW(char *name) {
	fileDescriptor fd = searchRT(name); //check if file already in table
	if (fd < 0){
		printf("tfs_makeRW -- file does not exist.\n");
		return INVALID_FD;
	}

	rt[fd].readOnly = 0;

	return 0;
}


/*a function that can write one byte to an exact position inside the file. */
int tfs_writeByte(fileDescriptor FD, int offset, unsigned char data) {
	int current;
	int error;
	int blockNum;
	int byte;
	int i = 0;

	if (!rt[FD].opened) {
		printf("in tfs_writeByte() -- file not opened\n");
		return -10;
	}
	//FileExtent ptr = malloc(BLOCKSIZE);
	FileExtent *head = malloc(BLOCKSIZE);
	inode *inodePtr = malloc(BLOCKSIZE);

	current = rt[FD].inodeNum;

	error = readBlock(mountedDisk, current, inodePtr); /* grab inode */
	if (error <0){
		return error;
	}
	error = readBlock(mountedDisk, inodePtr->data, head); /* grab first fileextent */
	if (error <0){
		return error;
	}

	if (rt[FD].fsize <= offset) {
		printf("in tfs_writeByte() -- offset greater than filesize\n");
		return INVALID_OFFSET;
	}

	blockNum = floor(offset / DEFAULT_DB_SIZE); 
	byte = offset % DEFAULT_DB_SIZE;

	while (head->blockNum != blockNum){
		error = readBlock(mountedDisk, head->next, head);
		if (error <0){
			return error;
		}
	}

	/*write the byte at the offset ? */

	free(head);
	free(inodePtr);
	return 0;
}

/* returns the file’s creation time or all info (up to you if you want to make multiple functions) */
int tfs_readFileInfo(fileDescriptor FD) {
	int error;
	inode *inodePtr = malloc(BLOCKSIZE);

	if (FD < 0 || FD > sizeof(rt)){
		printf("in tfs_readFileInfo -- invalid FD\n");
		return INVALID_FD;
	}

	error = readBlock(mountedDisk, rt[FD].inodeNum, inodePtr); 
	if (error <0){
		return error;
	}

	printf("%s",asctime(localtime(&inodePtr->creationTime)));

	if (inodePtr->accessTime != 0){
		printf("%s",asctime(localtime(&inodePtr->accessTime)));
	}

	if (inodePtr->modificationTime != 0){
		printf("%s",asctime(localtime(&inodePtr->modificationTime)));
	}

	return 0;
}
/**********************************************************************************/
/////////////////////////
/// tinyFS OPERATIONS ///
/////////////////////////
/**********************************************************************************/

int tfs_mkfs(char *filename, int nBytes) {
	int error;
	printf("\n] CREATING NEW FILE SYSTEM ON DISK NAME %s\n", filename);
	int disk = openDisk(filename, nBytes);
	if (disk == -1) {
		printf("error - invalid file\n");
		return INVALID_FILE;
	}
	if (disk == -2) {
		printf("error - invalid nBytes\n");
		return INVALID_NBYTES;
	}

	mountedDisk = disk;

	int fb = nBytes / BLOCKSIZE;

	error = initFBList(nBytes);
	if (error <0){
		return error;
	}
	error = initSB(fb);
	if (error <0){
		return error;
	}

	return 0;
}

int tfs_mount(char *filename) {
	int error;
	printf("\n] MOUNTING DISK NAME %s\n", filename);
	if (mountedDisk != UNMOUNTED) {
		printf("-There is already a mounted disk -- exiting\n");
		return INVALID_MOUNT;
	}
	int disk = searchDisk(filename);
	if (disk < 0) {
		printf("in tfs_mount() -- disk not found\n");
		return INVALID_DISK;
	}
	if (disk == -1) {
		printf("error - invalid file\n");
		return INVALID_FILE;
	}
	if (disk == -2) {
		printf("error - invalid nBytes\n");
		return INVALID_NBYTES;
	}
	char buf[BLOCKSIZE];

	error = readBlock(disk, 0, buf);

	if (error <0){
		return error;
	}

	if (buf[1] != MAGIC_N) {
		return ERR_MAGICN; // not the magic number
	}

	mountedDisk = disk;
	printf("-Successfully mounted disk %d\n", mountedDisk);
	sb->rtSize = 0;
	error = writeBlock(mountedDisk, 0, sb);
	if (error <0){
		return error;
	}
	error = buildRT();

	if (error <0){
		return error;
	}

	return 0;
}

int tfs_unmount(void) {
	printf("\n] UNMOUNTING DISK\n");
	if (mountedDisk == UNMOUNTED) {
		printf("nothing to unmount -- exiting\n");
		return INVALID_UNMOUNT;
	}
	mountedDisk = UNMOUNTED;
	//sb = NULL;
	memset(rt, 0, sizeof(rt));
	return 0;
}

fileDescriptor tfs_openFile(char *name) {
	printf("\n] OPENING FILE %s\n", name);
	if (mountedDisk == UNMOUNTED) {
		printf("nothing mounted -- exiting\n");
		return NOTHING_MOUNTED;
	} 

	if (sizeof(name) > FNAME_LIMIT) {
		printf("file name too long -- exiting\n");
		return INVALID_FNAME;
	}

	fileDescriptor fd = searchRT(name); //check if file already in table

	if (fd < 0) { // not exists
		int inodeBlockNum = createIN(name); // create inode
		fd = createRTEntry(name, inodeBlockNum);
		if (fd<0){
			return fd; /*error*/
		}
		rt[fd].opened = 1;
		return fd;
	}
	
	else { // exists
		rt[fd].opened = 1;
		return fd;
	}
}

/* Closes the file, de-allocates all system/disk resources, and removes table entry */
int tfs_closeFile(fileDescriptor FD) {
	if (sizeof(rt) <= FD) {
		printf("in tfs-closeFile() -- invalid FD\n");
		return INVALID_FD; 
	}
	rt[FD].opened = 0;

	return 0;
}

/* Writes buffer ‘buffer’ of size ‘size’, which represents an entire file’s content, to the file system. 
Sets the file pointer to 0 (the start of file) when done. 
Returns success/error codes. */
int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
	// check if FD is open or not
	printf("\n] WRITING FILE\n");
	if (rt[FD].readOnly == 1){
		printf("in tfs_writeFile() -- file is read only, cannot write.\n");
		return READ_ONLY;
	}
	if (!rt[FD].opened) {
		printf("in tfs_writeFile() -- file not opened\n");
		return FILE_NOT_OPEN;
	}

	// set file size for the corresponding RT entry
	rt[FD].fsize = size;
	printf("-inode new fsize: %d\n", rt[FD].fsize);

	int numDataBlocks = 1; 
	numDataBlocks += floor(size / DEFAULT_DB_SIZE);

	int i;
	int sizeRemaining = size;
	printf("-Need to create %d data blocks\n", numDataBlocks);

	// deallocate all data blocks associated with inode before writing
	inode *iptr = malloc(BLOCKSIZE);
	readBlock(mountedDisk, rt[FD].inodeNum, iptr);
	if (iptr->data != 0) {
		printf("first data block at: %d\n", iptr->data);
		FileExtent *fp = malloc(BLOCKSIZE);
		readBlock(mountedDisk, iptr->data, fp);

		while (fp->next != 0) {
			printf("current fp block num: %d\n",fp->blockNum);
			int blockToFree = fp->blockNum;
			readBlock(mountedDisk, fp->next, fp);
			deallocateDB(blockToFree);
		}
		deallocateDB(fp->blockNum);
		free(fp);
	}

	for (i = 0; i < numDataBlocks; i++) {
		int newDBNum = createDB(rt[FD].inodeNum, &sizeRemaining, buffer, i);
		if (i == 0) {
			iptr->data = newDBNum;
			writeBlock(mountedDisk, iptr->blockNum, iptr);
		}
	}
	
	// set file pointer to 0
	rt[FD].blockOffset = 0;
	rt[FD].byteOffset = DATA_HEADER_OFFSET;

	return 0;
}

/* deletes a file and marks its blocks as free on disk. */
int tfs_deleteFile(fileDescriptor FD) {
	int error;
	int current;
	freeblock *newFB;
	FileExtent *head = malloc(BLOCKSIZE);
	FileExtent *previous = malloc(BLOCKSIZE);
	inode *inodePtr = malloc(BLOCKSIZE);
	
	ResourceTableEntry entry = rt[FD];

	if (rt[FD].readOnly == 1){
		printf("in tfs_deleteFile() -- file is read only, cannot delete.\n");
		return READ_ONLY;
	}

	current = rt[FD].inodeNum;
	error = readBlock(mountedDisk, current, inodePtr); /* grab inode */
	if (error <0){
		return error;
	}
	error = readBlock(mountedDisk, inodePtr->data, previous); /* grab first fileextent */
	if (error <0){
		return error;
	}
	error = readBlock(mountedDisk, previous->next, head);
	if (error <0){
		return error;
	}

	// while (head->next != 0){ /*free linked list of blocks) */

	// 	newFB = calloc(previous->blockNum, BLOCKSIZE);
	// 	newFB->blockType = FREE_BLOCK;
	// 	newFB->magicN = MAGIC_N;
	// 	newFB->blockNum = previous->blockNum;
	// 	newFB->nextBlockNum = head->blockNum;
	// 	error = writeBlock(mountedDisk, previous->blockNum, newFB);
	// 	if (error <0){
	// 		return error;
	// 	}
	// 	sb->numFreeBlocks +=1;
	// 	writeBlock(mountedDisk, 0, sb);
	// 	free(previous);
	// 	error = readBlock(mountedDisk, head->next, head);
	// 	if (error <0){
	// 		return error;
	// 	}
	// }

	
	// newFB = calloc(previous->blockNum, BLOCKSIZE);
	// newFB->blockType = FREE_BLOCK;
	// newFB->magicN = MAGIC_N;
	// newFB->blockNum = previous->blockNum;
	// newFB->nextBlockNum = head->blockNum;
	// error = writeBlock(mountedDisk, previous->blockNum, newFB);
	// if (error <0){
	// 	return error;
	// }
	// sb->numFreeBlocks +=1;
	// writeBlock(mountedDisk, 0, sb);
	// free(previous);

	// newFB = calloc(head->blockNum, BLOCKSIZE);
	// newFB->blockType = FREE_BLOCK;
	// newFB->magicN = MAGIC_N;
	// newFB->blockNum = head->blockNum;
	// newFB->nextBlockNum = 0;
	// error = writeBlock(mountedDisk, head->blockNum, newFB);
	// if (error <0){
	// 	return error;
	// }
	// numFreeBlocks +=1;
	// free(head);

	// /*free inode & override with free block */
	// newFB = calloc(inodePtr->blockNum, BLOCKSIZE);
	// newFB->blockType = FREE_BLOCK;
	// newFB->magicN = MAGIC_N;
	// newFB->blockNum = inodePtr->blockNum;
	// newFB->nextBlockNum = 0;
	// error = writeBlock(mountedDisk, inodePtr->blockNum, newFB);
	// if (error <0){
	// 	return error;
	// }
	// numFreeBlocks +=1;
	// free (inodePtr);

	rt[FD].opened = 0;
	rt[FD].deleted = 0;

	return 0;
}

/* Reads one byte from the file and copies it to buffer, 
using the current file pointer location and incrementing it by one upon success. 
If the file pointer is already at the end of the file then tfs_readByte() should return an error and not increment the file pointer. */
int tfs_readByte(fileDescriptor FD, char *buffer) {
	int error;
	inode *ptr = malloc(BLOCKSIZE);
	int inodeBNum = rt[FD].inodeNum;
	error = readBlock(mountedDisk, inodeBNum, ptr);
	if (error <0){
		return error;
	}
	int blockOffset = rt[FD].blockOffset;
	int byteOffset = rt[FD].byteOffset;
	int currPos = (blockOffset * BLOCKSIZE) + byteOffset;
	
	if (currPos >= rt[FD].fsize) {
		printf("in readByte() -- file ptr already at end of file\n");
		return INVALID_READ;
	}

	char *fileBuf = malloc(BLOCKSIZE);

	// find the correct logical block num
	// this can probably be decomposed into another function
	// actually, a lot of this shit could lol
	int i = 0;
	FileExtent *fp = malloc(BLOCKSIZE);
	error = readBlock(mountedDisk, ptr->data, fp);
	if (error <0){
		return error;
	}
	while (i < blockOffset) {
		error = readBlock(mountedDisk, fp->next, fp);
		if (error <0){
			return error;
		}
	}

	error = readBlock(mountedDisk, fp->blockNum, fileBuf);
	if (error <0){
		return error;
	}
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
	int error;
	int blockNum;
	int byte;
	int i = 0;

	if (!rt[FD].opened) {
		printf("in tfs_seek() -- file not opened\n");
		return FILE_NOT_OPEN;
	}
	//FileExtent ptr = malloc(BLOCKSIZE);
	FileExtent *head = malloc(BLOCKSIZE);
	inode *inodePtr = malloc(BLOCKSIZE);

	current = rt[FD].inodeNum;

	error = readBlock(mountedDisk, current, inodePtr); /* grab inode */
	if (error <0){
		return error;
	}
	error = readBlock(mountedDisk, inodePtr->data, head); /* grab first fileextent */
	if (error <0){
		return error;
	}
	if (rt[FD].fsize <= offset) {
		printf("in tfs_seek() -- offset greater than filesize\n");
		return INVALID_OFFSET;
	}

	blockNum = floor(offset / DEFAULT_DB_SIZE); 
	byte = offset % DEFAULT_DB_SIZE;

	rt[FD].blockOffset = blockNum;
	rt[FD].byteOffset = byte;

	return 0;
}

void errorHandling(int error){
	if (error = INVALID_FD){
		printf("Error: Invalid File Descriptor.\n");
		printf("Have a nice day :)\n");
	} else if(error = INVALID_FNAME){
		printf("Error: Invalid File Name.\n");
		printf("Have a nice day :)\n");
	} else if(error = ROOT_NOT_INITIALIZED){
		printf("Error: Unable to build resource table...root not initialized.\n");
		printf("Have a nice day :)\n");
	} else if(error = NO_FREE_BLOCKS){
		printf("Error: Unable to update super block...no free blocks available.\n");
		printf("Have a nice day :)\n");
	} else if(error = NO_FILES_ON_DISK){
		printf("Error: There are no files currently on the disk.\n");
		printf("Have a nice day :)\n");
	} else if(error = FILE_NOT_OPEN){
		printf("Error: File is not open. Open before attempting to seek or write.\n");
		printf("Have a nice day :)\n");
	} else if(error = INVALID_OFFSET){
		printf("Error: Unable to build resource table...root not initialized.\n");
		printf("Have a nice day :)\n");
	} else if(error = INVALID_FILE){
		printf("Error: Invalid file.\n");
		printf("Have a nice day :)\n");
	} else if(error = INVALID_NBYTES){
		printf("Error: Invalid nBytes.\n");
		printf("Have a nice day :)\n");
	} else if(error = INVALID_DISK){
		printf("Error: Invalid disk number. Disk not found.\n");
		printf("Have a nice day :)\n");
	} else if(error = INVALID_MOUNT){
		printf("Error: Unable to mount.\n");
		printf("Have a nice day :)\n");
	} else if(error = INVALID_UNMOUNT){
		printf("Error: Unable to unmount.\n");
		printf("Have a nice day :)\n");
	}else if (error = READ_ONLY){
		printf("Error: File is read only. Unable to write or delete.\n");
		printf("Have a nice day :)\n");
	} else if (error = INVALID_READ){
		printf("Error: Unable to read.\n");
		printf("Have a nice day :)\n");
	} else if (error = ERR_MAGICN){
		printf("Error: Magic Number not found.\n");
		printf("Have a nice day :)\n");
	}else if (error = NOTHING_MOUNTED){
		printf("Error: No mounted disk.\n");
		printf("Have a nice day :)\n");
	}

}

int main() {
	// TESTING MKFS
	printf("----- TESTING tfs_mkfs() -----");
	char *disk1 = "test.txt"; 
	int res = tfs_mkfs(disk1, DEFAULT_DISK_SIZE);
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

	// TESTING MOUNT
	printf("\n----- TESTING tfs_mount() & tfs_unmount() -----");
	char *disk2 = "tinyFSDisk";

	// Test for mounting while already mounted
	tfs_mount(disk2);

	// Test for mounting a non-existing file system
	tfs_unmount();
	tfs_mount(disk2);

	// Repeating all the steps for first mkfs
	// Diff these two files should return nothing
	tfs_mkfs(disk2, DEFAULT_DISK_SIZE);
	fd3 = tfs_openFile(fname3);
	printf("-File descriptor for %s is %d\n", fname3, fd3);
	printRT();
	printIN(fd3);
	fd4 = tfs_openFile(fname4);
	printf("-File descriptor for %s is %d\n", fname4, fd4);
	printRT();
	printIN(fd3);
	printIN(fd4);

	// Test if mounting/unmounting persists file system data
	tfs_unmount();
	tfs_mount(disk1);
	fd3 = tfs_openFile(fname3); // should open files remain open?
	fd4 = tfs_openFile(fname4);
	printRT();

	printf("\n----- TESTING tfs_writeFile() -----");
	char writeBuf[512];
	printf("\n-Size of writeBuf: %lu\n", sizeof(writeBuf));
	memset(writeBuf, 1, 512);
	res = tfs_writeFile(fd3, writeBuf, 512);
	printIN(fd3);

	memset(writeBuf, 2, sizeof(writeBuf));
	res = tfs_writeFile(fd3, writeBuf, DEFAULT_DB_SIZE);
	printIN(fd3);

	return 0;
}