#include <stdint.h>

/* The default size of the disk and file system block */
#define BLOCKSIZE 256

/* Your program should use a 10240 Byte disk size giving you 40 blocks total. 
This is a default size. You must be able to support different possible values */
#define DEFAULT_DISK_SIZE 10240 

/* this is arbitray, we currently have no way of knowing if 
these numbers will be enough (or too much) */
#define DEFAULT_OT_SIZE 10 
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
#define UNINIT -1
#define FNAME_LIMIT 8
#define MAGIC_N 0x45

void initSB();
void initFBList(int nBytes);
void insertIN(uint8_t new);
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

typedef struct FileExtent {
	uint8_t blockType;
	uint8_t magicN;
	uint8_t blockNum;
	uint8_t next;
	char data[BLOCKSIZE - 4];
} FileExtent;

typedef struct inode {
	uint8_t blockType;
	uint8_t magicN;
	uint8_t blockNum;
	char fname[9];
	uint8_t fSize;
	uint8_t data;
	uint8_t next; // linked list "pointer"
	char emptyOffset[BLOCKSIZE - 15];
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
	int opened; /* 0 if unopened, 1 if opened */
} ResourceTableEntry;

/*typedef struct OpenFileTableEntry {
	fileDescriptor fd;
	char fname[9];
} OpenFileTableEntry;*/