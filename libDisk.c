#include "libDisk.h"
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

int diskNum = 0;
disk *head;

////////// Disk Struct Functions
disk *findDisk(char *filename) { /* may or may not return the correct disk, must check the return value */
	disk *curr = head;
	while ((strcmp(curr->filename, filename) != 0) && (curr->next != NULL)) {
		curr = curr->next;
	}
	return curr;
}

void createDisk(disk *newDisk, char *filename, int nBytes) {
	newDisk->diskNum = diskNum;
	newDisk->next = NULL;
	newDisk->filename = filename;
	newDisk->fsize = nBytes;
	newDisk->currBytes = 0;
	diskNum += 1;
}

int insertDisk(disk *newDisk) {
	disk *temp = head;
	if (temp == NULL) {
		return 0; // root must be created first
	}
	while (temp->next != NULL) {
		temp = temp->next;
	}
	temp->next = newDisk;
	return 1;
}
/////////////


/////////// Disk Emulator Operations
/* This functions opens a regular UNIX file and designates the first nBytes of it as space for the emulated disk. 
nBytes should be a number that is evenly divisible by the block size. 
If nBytes > 0 and there is already a file by the given filename, that disk is resized to nBytes, 
and that file’s contents may be overwritten. If nBytes is 0, an existing disk is opened, and should not be overwritten. 
There is no requirement to maintain integrity of any content beyond nBytes. 
The return value is -1 on failure or a disk number on success. */

int openDisk(char *filename, int nBytes){
	disk *ptr;

	if (nBytes % BLOCKSIZE != 0) {
		return -3; // nBytes not of valid size
	}

	/*check if file is already open using stat
	/if already open, shrink to nbytes size*/
	if (nBytes == 0) {

		// if disk doesn't exist in linked list, then create a new one
		// assumption here is that the file has been opened at some point
		// and a corresponding disk may or may not exist
		if ((ptr = findDisk(filename)) == NULL) { 
			disk *newDisk;
			newDisk = malloc(sizeof(disk));
			createDisk(newDisk, filename, nBytes);
			insertDisk(newDisk);
			return newDisk->diskNum;
		}
		else { // file is still open
			return ptr->diskNum;
		}
	}

	if (nBytes > 0) {	
		if (diskNum == 0) {
			FILE *fd;
			if ((fd = fopen(filename, "r+")) == NULL) {
				return -1; // file open failure
			}
			head = malloc(sizeof(disk));
			createDisk(head, filename, nBytes);
			head->filestream = fd;
			head->fd = fileno(fd);
			return head->diskNum;
		} 
		else {
			ptr = findDisk(filename);
			if (ptr == NULL) { /*file does not exist yet*/
				FILE *fd;
				if ((fd = fopen(filename, "r+")) == NULL) {
					return -1; // file open failure
				}
				disk *newDisk;
				newDisk = malloc(sizeof(disk));
				createDisk(newDisk, filename, nBytes);
				insertDisk(newDisk);
				newDisk->filestream = fd;
				newDisk->fd = fileno(fd);
				return newDisk->diskNum;
			} 
			else { /*file already exists, resize*/
				ptr->currBytes = 0;
				ptr->fsize = nBytes;
				return ptr->diskNum;
			}
		}
	}
	return -1;
}


/* readBlock() reads an entire block of BLOCKSIZE bytes from the open disk (identified by ‘disk’) 
and copies the result into a local buffer (must be at least of BLOCKSIZE bytes). 
The bNum is a logical block number, which must be translated into a byte offset within the disk. 
The translation from logical to physical block is straightforward: bNum=0 is the very first byte of the file. 
bNum=1 is BLOCKSIZE bytes into the disk, bNum=n is n*BLOCKSIZE bytes into the disk. 
On success, it returns 0. -1 or smaller is returned if disk is not available (hasn’t been opened) or any other failures. 
You must define your own error code system. */
int readBlock(int disk, int bNum, void *block) {
	struct disk *ptr;
	ptr = head;

	while ((ptr->diskNum != disk) && (ptr->next != NULL)) {
		ptr = ptr->next;
	}
	if (ptr == NULL) {
		return -1; // disk not found, doesn't exist
	} 

	//disk found
	if (fseek(ptr->filestream, bNum*BLOCKSIZE, SEEK_SET) != 0) { /*seek to the offset point into the disk */
		return -2; /*error */
	}
	if (fread(block, 1, BLOCKSIZE, ptr->filestream) != BLOCKSIZE) {
		return -3; /*error */
	}
	return 0;
}


/* writeBlock() takes disk number ‘disk’ and logical block number ‘bNum’ and writes the content of the buffer ‘block’ to that location. 
BLOCKSIZE bytes will be written from ‘block’ regardless of its actual size. The disk must be open. 
Just as in readBlock(), writeBlock() must translate the logical block bNum to the correct byte position in the file. 
On success, it returns 0. -1 or smaller is returned if disk is not available (i.e. hasn’t been opened) or any other failures. 
You must define your own error code system. */
int writeBlock(int disk, int bNum, void *block){
	struct disk *ptr;
	ptr = head;

	while ((ptr->diskNum != disk) && (ptr->next != NULL)) {
		ptr = ptr->next;
	}
	if (ptr == NULL) {
		return -1; /*disk not available */
	} 
	else { /*disk found */
		if (fseek(ptr->filestream, bNum*BLOCKSIZE, SEEK_SET) != 0) { /*seek to the offset point into the disk */
			return -1; /*error */
		}
		if (fwrite(block, 1, BLOCKSIZE, ptr->filestream) != 256) {
			return -1; /*error */
		}
	}

	return 0;
}

/* closeDisk() takes a disk number ‘disk’ and makes the disk closed to further I/O; 
i.e. any subsequent reads or writes to a closed disk should return an error. 
Closing a disk should also close the underlying file, committing any writes being buffered by the real OS. */
void closeDisk(int disk){
	struct disk *ptr;
	struct disk *previous;
	if (head->diskNum == disk) {

	} 
	else {
		ptr = head->next;
		previous = head;
		while ((ptr->diskNum != disk) && (ptr->next != NULL)) {
			ptr = ptr->next;
			previous = previous->next;
		}
		if ((ptr->next == NULL) && (ptr->diskNum != disk)) {
			 printf("error - disk not available");
		} 
		else { /*disk found */
			fclose(ptr->filestream);
			previous->next = ptr->next; /* "close" disk by removing it from the linked list */
		}
	}
}

int main() {
	int dn = openDisk("test.txt", 256);
	printf("disk num: %d\n",dn);

	void *buf;
	int res = readBlock(dn, 1, buf);
	printf("success? %d\n", res);
}


