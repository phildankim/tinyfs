#ifndef LIBDISKH
#define LIBDISKH
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


#define BLOCKSIZE 256
#define DISK_DEFAULT 20

/*error codes */
#define INVALID_FD -1
#define INVALID_FNAME -2
#define ROOT_NOT_INITIALIZED -3
#define NO_FREE_BLOCKS -4
#define NO_FILES_ON_DISK -5
#define FILE_NOT_OPEN -6
#define INVALID_OFFSET -7
#define INVALID_FILE -8
#define INVALID_NBYTES -9
#define INVALID_DISK -10
#define INVALID_MOUNT -11
#define INVALID_UNMOUNT -12
#define READ_ONLY -13
#define INVALID_READ -14
#define ERR_MAGICN -15
#define NOTHING_MOUNTED -16
#define FILE_NOT_EXIST -17
#define LSEEK_FAIL -18
#define READ_FAIL -19
#define WRITE_FAIL -20

int getFD(int disk);
int searchDisk(char *fname);

typedef struct Disk {
	char fname[9];
	int fd;
} Disk;


//fileDescriptor retrieveFD(int disk);

/* This functions opens a regular UNIX file and designates the first nBytes of it as space for the emulated disk. 
nBytes should be a number that is evenly divisible by the block size. 
If nBytes > 0 and there is already a file by the given filename, that disk is resized to nBytes, 
and that file’s contents may be overwritten. If nBytes is 0, an existing disk is opened, and should not be overwritten. 
There is no requirement to maintain integrity of any content beyond nBytes. 
The return value is -1 on failure or a disk number on success. */
int openDisk(char *filename, int nBytes);

/* readBlock() reads an entire block of BLOCKSIZE bytes from the open disk (identified by ‘disk’) 
and copies the result into a local buffer (must be at least of BLOCKSIZE bytes). 
The bNum is a logical block number, which must be translated into a byte offset within the disk. 
The translation from logical to physical block is straightforward: bNum=0 is the very first byte of the file. 
bNum=1 is BLOCKSIZE bytes into the disk, bNum=n is n*BLOCKSIZE bytes into the disk. 
On success, it returns 0. -1 or smaller is returned if disk is not available (hasn’t been opened) or any other failures. 
You must define your own error code system. */
int readBlock(int disk, int bNum, void *block);

/* writeBlock() takes disk number ‘disk’ and logical block number ‘bNum’ and writes the content of the buffer ‘block’ to that location. 
BLOCKSIZE bytes will be written from ‘block’ regardless of its actual size. The disk must be open. 
Just as in readBlock(), writeBlock() must translate the logical block bNum to the correct byte position in the file. 
On success, it returns 0. -1 or smaller is returned if disk is not available (i.e. hasn’t been opened) or any other failures. 
You must define your own error code system. */
int writeBlock(int disk, int bNum, void *block);

/* closeDisk() takes a disk number ‘disk’ and makes the disk closed to further I/O; 
i.e. any subsequent reads or writes to a closed disk should return an error. 
Closing a disk should also close the underlying file, committing any writes being buffered by the real OS. */
void closeDisk(int disk);

#endif