// Written by: Daniel Olaya - V00855054
// Date: March, 2018,
// CSC360 - Operative Systems

#include <stdio.h> 
#include <stdint.h> 
#include <stdlib.h> 
#include <sys/stat.h> 
#include <unistd.h>
#include <string.h> 
#include <limits.h> 
#include <assert.h> 
#include <time.h> 
#include <arpa/inet.h> 
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "diskinfo.h"


// extern unsigned int bsize, bcount, fatstart, fatblocks, rootstart, rootblocks;
struct __attribute__((__packed__))dir_entry_timedate_t{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
}dir_timedate;

struct __attribute__((__packed__))dir_entry_t{
	uint8_t status;
	uint32_t starting_block;
	uint32_t block_count;
	uint32_t size;
	struct dir_entry_timedate_t modify_time;
	struct dir_entry_timedate_t create_time;
	uint8_t filename[31];
	uint8_t unused[6];
}dir_entry;



int main(int argc, char* argv[]) {
	struct stat file_size;
	unsigned char *ptr;
	int file;


	if (argc != 3){
		fprintf(stderr,"Usage: [file_name] [disk_image] [directory]\n");
		return EXIT_FAILURE;
	}
	file = open(argv[1], O_RDWR);
	if (file < 0){
		perror("Unable to open file\n");
		return EXIT_FAILURE;
	}
	printf("File [%s] opened...\n\n", argv[1]);
	if (fstat(file, &file_size) != 0){
		perror("fstat() error");
		return EXIT_FAILURE;
	}

	ptr = mmap(NULL, file_size.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
	intializeVars(&ptr);

	printf("FROM DISKLISTS\n");
	printf("Block size: %d\n", bsize);
	printf("Block count: %u\n", bcount);
	printf("FAT starts: %u\n", fatstart);
	printf("FAT blocks: %u\n", fatblocks);
	printf("Root directory start: %u\n", rootstart);
	printf("Root directory blocks: %u\n", rootblocks);


	dir_entry *dir1 = (dir_entry *)calloc(1, sizeof(dir_entry));

	printf("%02x - %d\n", ptr[rootstart*bsize], rootstart*bsize);




}