diskmake: libDisk.c libDisk.h
	gcc -Wall -g libDisk.c 

testmake: libDisk.c libDisk.h libDiskTest.c
	gcc -Wall -g libDiskTest.c
