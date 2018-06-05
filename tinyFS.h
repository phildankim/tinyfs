/* The default size of the disk and file system block */
#define BLOCKSIZE 256

/* Your program should use a 10240 Byte disk size giving you 40 blocks total. 
This is a default size. You must be able to support different possible values */
#define DEFAULT_DISK_SIZE 10240 

/* use this name for a default disk file name */
#define DEFAULT_DISK_NAME “tinyFSDisk” 	
typedef int fileDescriptor;

typedef struct FileExtent {
	int type;
	int num;
	void *next;
} FileExtent;

typedef struct inode {
	char *filename;
	int fileType; //0 for regular file, 1 for dir
	struct FileExtent *data;
	struct inode **dirNodes;
	int inodeNum;
} inode;

typedef struct superblock {
	char blockType;
	char magicN;
	inode *root;

} superblock;
