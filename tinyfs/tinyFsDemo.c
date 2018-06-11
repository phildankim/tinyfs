#include "libDisk.h"
#include "libTinyFS.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


int main() {
	// TESTING MKFS
	int error;
	printf("----- TESTING tfs_mkfs() -----");
	char *disk1 = "test.txt"; 
	int res = tfs_mkfs(disk1, DEFAULT_DISK_SIZE);
	if (res<0){
		errorHandling(res);
	}
	printSB();
	
	// TESTING OPENFILE
	printf("\n----- TESTING tfs_openFile()  with filename a.txt-----");
	char *fname3 = "a.txt";
	int fd3 = tfs_openFile(fname3);
	if (fd3<0){
		errorHandling(fd3);
	}
	printf("-File descriptor for %s is %d\n", fname3, fd3);
	printRT();
	if ((error = printIN(fd3))<0){
		errorHandling(error);
	}

	// Test for multiple files
	printf("\n----- TESTING tfs_openFile()  with filename b.txt-----");
	char *fname4 = "b.txt";
	int fd4 = tfs_openFile(fname4);
	if (fd4<0){
		errorHandling(fd4);
	}
	printf("-File descriptor for %s is %d\n", fname4, fd4);
	printRT();
	error = printIN(fd4);
	if (error <0){
		errorHandling(error);
	}

	// Test if existing file can be found
	error = tfs_openFile("a.txt");
	if (error <0){
		errorHandling(error);
	}
	// TESTING MOUNT
	printf("\n----- TESTING tfs_mount() & tfs_unmount() -----");
	char *disk2 = "tinyFSDisk";

	// Test for mounting while already mounted
	error = tfs_mount(disk2);
	if (error <0){
		errorHandling(error);
	}

	// Test for mounting a non-existing file system
	error = tfs_unmount();
	if (error <0){
		errorHandling(error);
	}
	error = tfs_mount(disk2);
	if (error <0){
		errorHandling(error);
	}

	// Repeating all the steps for first mkfs
	// Diff these two files should return nothing
	// error = tfs_mkfs(disk2, DEFAULT_DISK_SIZE);
	// if (error <0){
	// 	errorHandling(error);
	// }
	fd3 = tfs_openFile(fname3);
	if (fd3<0){
		errorHandling(fd3);
	}
	printf("-File descriptor for %s is %d\n", fname3, fd3);
	printRT();
	error = printIN(fd3);
	if (error<0){
		errorHandling(error);
	}
	fd4 = tfs_openFile(fname4);
	if (fd4<0){
		errorHandling(fd4);
	}
	printf("-File descriptor for %s is %d\n", fname4, fd4);
	printRT();
	error = printIN(fd3);
	if (error<0){
		errorHandling(error);
	}
	error = printIN(fd4);
	if (error<0){
		errorHandling(error);
	}

	// Test if mounting/unmounting persists file system data
	error = tfs_unmount();
	if (error <0){
		errorHandling(error);
	}
	error = tfs_mount(disk1);
	if (error <0){
		errorHandling(error);
	}
	fd3 = tfs_openFile(fname3); // should open files remain open?
	if (fd3<0){
		errorHandling(fd3);
	}
	fd4 = tfs_openFile(fname4);
	if (fd4<0){
		errorHandling(fd4);
	}
	printRT();

	// TESTING WRITEFILE
	printf("\n----- TESTING tfs_writeFile() -----");
	char writeBuf[512];
	printf("\n] WRITING TO %s\n", fname3);
	printf("\n-Size of write buffer: %lu\n", sizeof(writeBuf));
	memset(writeBuf, 1, 512);

	res = tfs_writeFile(fd3, writeBuf, 512);

	if (res<0){
		errorHandling(res);
	}
	error = printIN(fd3);
	if (error<0){
		errorHandling(error);
	}

	memset(writeBuf, 2, sizeof(writeBuf));
	res = tfs_writeFile(fd3, writeBuf, DEFAULT_DB_SIZE);
	if (res<0){
		errorHandling(res);
	}
	error = printIN(fd3);
	if (error<0){
		errorHandling(error);
	}

	error = printAllFB(0);
	if (error<0){
		errorHandling(error);
	}

	char *writeBuf2 = "abbbbbbdb";
	printf("\n] WRITING TO %s\n", fname4);
	printf("\n-Size of write buffer: %lu\n", sizeof(writeBuf2));
	res = tfs_writeFile(fd4, writeBuf2, DEFAULT_DB_SIZE);
	if (res<0){
		errorHandling(res);
	}
	error = printIN(fd4);
	if (error<0){
		errorHandling(error);
	}
	error = printAllFB(0);
	if (error<0){
		errorHandling(error);
	}

	printf("\n----- TESTING tfs_readByte() & tfs_seek() -----");
	char *emptyBuf;
	printf("\n] Testing readByte\n");
	printf("\n-Should read first byte (in hex) of string: %s\n", writeBuf2);
	error = tfs_readByte(fd4, emptyBuf);
	if (error<0){
		errorHandling(error);
	}
	printf("\n-Should read second byte (in hex) of string: %s\n", writeBuf2);
	error = tfs_readByte(fd4, emptyBuf);
	if (error<0){
		errorHandling(error);
	}

	printf("\n] Testing seek\n");
	printf("-Should seek to byte 6, expecting d\n");
	error = tfs_seek(fd4, 7);
	if (error<0){
		errorHandling(error);
	}
	error = tfs_readByte(fd4, emptyBuf);
	if (error<0){
		errorHandling(error);
	}

	printf("\n*****Testing part 3 functions ******\n");
	printf("\n----- TESTING tfs_rename() with filename a.txt, renaming to c.txt -----");	
	error = tfs_rename("c.txt", "a.txt");
	if (error<0){
		errorHandling(error);
	}

	printf("\n----- TESTING tfs_readdir() -----");	
	error = tfs_readdir();
	if (error<0){
		errorHandling(error);
	}

	printf("\n----- TESTING tfs_writeByte() -----\n");
	error = tfs_writeByte(fd4, 7, '1');
	if (error<0){
		errorHandling(error);
	}
	error = tfs_readByte(fd4, emptyBuf);
	if (error<0){
		errorHandling(error);
	}

	printf("\n----- TESTING timestamps for FD # 1 -----");
	tfs_readFileInfo(1);

	printf("\n----- TESTING Read-Only Ability -----");
	tfs_makeRO("b.txt");
	tfs_makeRW("b.txt");


	printf("\n----- TESTING tfs_closeFile() -----");
	error = tfs_closeFile(fd3);
	if (error<0){
		errorHandling(error);
	}
	error = tfs_writeFile(fd3, writeBuf2, DEFAULT_DB_SIZE);
	if (error<0){
		errorHandling(error);
	}

	printf("\n----- TESTING tfs_deleteFile() -----");
	error = tfs_deleteFile(fd3);
	if (error<0){
		errorHandling(error);
	}
	error = tfs_writeFile(fd3, writeBuf2, DEFAULT_DB_SIZE);
	if (error<0){
		errorHandling(error);
	}

	return 0;
}