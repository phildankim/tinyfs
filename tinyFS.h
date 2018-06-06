#include <stdint.h>

/* The default size of the disk and file system block */
#define BLOCKSIZE 256

/* Your program should use a 10240 Byte disk size giving you 40 blocks total. 
This is a default size. You must be able to support different possible values */
#define DEFAULT_DISK_SIZE 10240 

/* use this name for a default disk file name */
#define DEFAULT_DISK_NAME “tinyFSDisk” 	
typedef int fileDescriptor;

#define SUPERBLOCK 1
#define INODE 2
#define FILEEXTENT 3
#define FREEBLOCK 4
#define UNMOUNTED -1

void initSB();
void initFBList(int nBytes);
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
	void *next;
	char emptyOffset[BLOCKSIZE - 3];
} FileExtent;

typedef struct inode {
	uint8_t blockType;
	uint8_t magicN;
	uint8_t *filename[9];
	uint8_t fileType; //0 for regular file, 1 for dir
	struct FileExtent *data;
	struct inode **dirNodes;
	char emptyOffset[BLOCKSIZE - 15];
} inode;

typedef struct superblock {
	uint8_t blockType;
	uint8_t magicN;
	uint8_t nextFB;
	inode *root;
	char emptyOffset[BLOCKSIZE - 3];
} superblock;

typedef struct freeblock {
	uint8_t blockType;
	uint8_t magicN;
	uint8_t blockNum;
	uint8_t nextBlockNum;
	char emptyOffset[BLOCKSIZE - 4];
} freeblock; 
