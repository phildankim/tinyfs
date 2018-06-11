#include <stdint.h>
#include <time.h>

/* The default size of the disk and file system block */
#define BLOCKSIZE 256

/* Your program should use a 10240 Byte disk size giving you 40 blocks total. 
This is a default size. You must be able to support different possible values */
#define DEFAULT_DISK_SIZE 10240 

/* this is arbitray, we currently have no way of knowing if 
these numbers will be enough (or too much) */
#define DEFAULT_RT_SIZE 15    
#define DATA_HEADER_OFFSET 4
#define DEFAULT_DB_SIZE 252

/* use this name for a default disk file name */
#define DEFAULT_DISK_NAME “tinyFSDisk” 	
typedef int fileDescriptor;

#define SUPERBLOCK 1
#define INODE 2
#define FILE_EXTENT 3
#define FREE_BLOCK 4
#define UNMOUNTED -1
#define FNAME_LIMIT 8
#define MAGIC_N 0x45

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


int initSB();
int initFBList(int nBytes);
int insertIN(uint8_t new);
int errorCheck(int errno);
int tfs_mkfs(char *filename, int nBytes);
int tfs_mount(char *filename);
int tfs_unmount(void);
fileDescriptor tfs_openFile(char *name);
int tfs_closeFile(fileDescriptor FD);
int tfs_writeFile(fileDescriptor FD, char *buffer, int size);
int tfs_deleteFile(fileDescriptor FD);
int tfs_readByte(fileDescriptor FD, char *buffer);
int tfs_seek(fileDescriptor FD, int offset);

int tfs_rename(char *newfilename, char *oldfilename);  /* part 3 function */
int tfs_readdir(); /* part 3 function */
int tfs_makeRO(char *name);
int tfs_makeRW(char *name);
int tfs_writeByte(fileDescriptor FD, int offset, unsigned char data);
int tfs_readFileInfo(fileDescriptor FD);

typedef struct FileExtent {
	uint8_t blockType;
	uint8_t magicN;
	uint8_t blockNum;
	uint8_t next;
	char data[DEFAULT_DB_SIZE];
} FileExtent;

typedef struct inode {
	uint8_t blockType;
	uint8_t magicN;
	uint8_t blockNum;
	char fname[9];
	uint8_t data;
	uint8_t next; // linked list "pointer"
	char emptyOffset[BLOCKSIZE - 38];
	time_t creationTime;
	time_t accessTime;
	time_t modificationTime;
} inode;

typedef struct superblock {
	uint8_t blockType;
	uint8_t magicN;
	uint8_t nextFB;
	uint8_t rootNode;
	uint8_t numFreeBlocks;
	uint8_t rtSize;
	char emptyOffset[BLOCKSIZE - 6];
} superblock;

typedef struct freeblock {
	uint8_t blockType;
	uint8_t magicN;
	uint8_t blockNum;
	uint8_t nextBlockNum;
	char emptyOffset[BLOCKSIZE - 4];
} freeblock; 

typedef struct ResourceTableEntry { /*index = fd*/
	char fname[9];
	fileDescriptor fd; // this is the blockNum of the root data block correspondent to the file
	int inodeNum;
	int blockOffset;
	int byteOffset;
	int fsize;
	int opened; /* 0 if unopened, 1 if opened */
	int deleted; /*0 if deleted, 1 if present */
	int readOnly; /*0 if not read only, 1 if read only */
} ResourceTableEntry;