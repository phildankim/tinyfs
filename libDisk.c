#include "libDisk.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULTARRAYSIZE 20

// Declarations
int createDisk(int fd, char *fname);
int searchDisk(char *fname);
int diskNum = 0;
int diskSize = 0;
Disk diskArray[DISK_DEFAULT];	

// Disk Emulator Operations
int openDisk(char *filename, int nBytes) {
	int fd;

	if (nBytes % BLOCKSIZE != 0) {
		return -2; // invalid nBytes
	}

	int disk = searchDisk(filename);
	if (disk > 0) {
		return disk;
	}

	if (nBytes > 0) {
		if ((fd = open(filename, O_CREAT | O_RDWR)) == -1) {
			return -1; // invalid file
		}
		if (access(filename, F_OK) != -1) { // file exists
			if (ftruncate(fd, nBytes) == -1) {
				return -1; // invalid file
			}
			int i;
			for (i = 0; i < nBytes; i++) {
				write(fd, "\0", 1);
			}
		}
	}

	if (nBytes == 0) {
		if ((fd = open(filename, O_RDONLY)) == -1) {
			return -1; // invalid file
		}
	}

	return createDisk(fd, filename);
}

int readBlock(int disk, int bNum, void *block) {
	int fd = diskArray[disk].fd;
	if (fd < 2) { // stdin, stdout, stderr
		return -2; // illegal fds
	}

	if (lseek(fd, bNum*BLOCKSIZE, SEEK_SET) == -1) {
		return -1;
	}
	if (read(fd, block, BLOCKSIZE) == -1) {
		return -1;
	}

	return 0;
}

int writeBlock(int disk, int bNum, void *block){
	int fd = diskArray[disk].fd;
	if (fd < 2) { // stdin, stdout, stderr
		return -2; // illegal fds
	}

	if (lseek(fd, bNum*BLOCKSIZE, SEEK_SET) == -1) {
		return -1;
	}
	if (write(fd, block, BLOCKSIZE) == -1) {
		printf("in writeBlock -- write failed\n");
		return -1;
	}

	return 0;
}

void closeDisk(int disk) {
	close(diskArray[disk].fd);
	diskSize -= 1;
}

int createDisk(int fd, char *fname) {
	/*
	if (diskSize == 0 || diskArray == NULL) {
		diskArray = (int*)malloc(sizeof(int) * DEFAULTARRAYSIZE);
	}

	else if (diskSize >= DEFAULTARRAYSIZE) {
		diskArray = (int*)realloc(diskArray, sizeof(int) * diskSize * 2);
	}*/
	int newDisk = diskNum;
	diskArray[newDisk].fd = fd;
	strcpy(diskArray[newDisk].fname, fname); 
	diskNum += 1;
	diskSize += 1;
	//printf("] new disk\nfd: %d\nfilename: %s\n", diskArray[newDisk].fd, diskArray[newDisk].fname);

	return newDisk;
}

int getFD(int disk) {
	return diskArray[disk].fd;
}

int searchDisk(char *fname) {
	int i = 0;
	while (i < diskSize) {
		if (strcmp(diskArray[i].fname, fname) == 0) {
			return i;
		}
		i++;
	}
	return -1;
}

/*
int main() {
	int d1 = openDisk("test1.txt", BLOCKSIZE);
	printf("%d\n", d1);

	int d2 = openDisk("test.txt", BLOCKSIZE);
	printf("%d\n", d2); 	

	char *buf = malloc(BLOCKSIZE);
	char *string = "bbbbbbbbbbb";
	int res = writeBlock(d1, 0, string);
	printf("%s\n", buf);
	openDisk("test1.txt", BLOCKSIZE);
	//writeBlock(d1, 0, string);

	//char *buf2 = "poo";
	//writeBlock(d2, 0, buf2);
	
	return 0;
}
*/
