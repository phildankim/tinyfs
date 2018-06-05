#include "libDisk.h"
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULTARRAYSIZE 20

// Declarations
int createDisk(int fd);
int diskNum = 0;
int diskSize = 0;
int *diskArray = NULL;	

// Disk Emulator Operations
int openDisk(char *filename, int nBytes) {
	int fd;

	if (nBytes % BLOCKSIZE != 0) {
		return -2; // invalid nBytes
	}

	if (nBytes > 0) {
		if ((fd = open(filename, O_RDWR)) == -1) {
			return -1; // invalid file
		}
		if (access(filename, F_OK) != -1) { // file exists
			if (ftruncate(fd, nBytes) == -1) {
				return -1; // invalid file
			}
		}
	}

	if (nBytes == 0) {
		if ((fd = open(filename, O_RDONLY)) == -1) {
			return -1; // invalid file
		}
	}

	return createDisk(fd);
}

int readBlock(int disk, int bNum, void *block) {
	int fd = diskArray[disk];

	if (lseek(fd, bNum*BLOCKSIZE, SEEK_SET) == -1) {
		return -1;
	}
	if (read(fd, block, BLOCKSIZE) == -1) {
		return -1;
	}

	return 0;
}

int writeBlock(int disk, int bNum, void *block){
	int fd = diskArray[disk];

	if (lseek(fd, bNum*BLOCKSIZE, SEEK_SET) == -1) {
		return -1;
	}
	if (write(fd, block, BLOCKSIZE) == -1) {
		return -1;
	}

	return 0;
}

void closeDisk(int disk) {
	close(diskArray[disk]);
}

int createDisk(int fd) {
	if (diskSize == 0 || diskArray == NULL) {
		diskArray = (int*)malloc(sizeof(int) * DEFAULTARRAYSIZE);
	}

	else if (diskSize >= DEFAULTARRAYSIZE) {
		diskArray = (int*)realloc(diskArray, sizeof(int) * diskSize * 2);
	}

	int newDisk = diskNum;
	diskArray[newDisk] = fd;
	diskNum += 1;
	diskSize += 1;

	return newDisk;
}
/*
int main() {
	int d1 = openDisk("test.txt", BLOCKSIZE);
	printf("%d\n", d1);
	int d2 = openDisk("test1.txt", BLOCKSIZE);
	printf("%d\n", d2);
	char *buf = malloc(sizeof(int) * BLOCKSIZE);
	int res = readBlock(d1, 1, buf);
	printf("%s\n", buf);
	char *buf2 = "poo";
	writeBlock(d2, 1, buf);
	
	return 0;
}
*/