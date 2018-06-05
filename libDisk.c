#include "libDisk.h"
#include <string.h>
#include <fcntl.h>

int diskNum = 0;
disk *head;

int openDisk(char *filename, int nBytes) {
	int fd;

	if (nBytes % BLOCKSIZE != 0) {
		
	}
}


