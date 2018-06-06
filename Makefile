all: tinyfs

tinyfs:
	gcc libDisk.c libDisk.h tinyFS.c tinyFS.h -g
