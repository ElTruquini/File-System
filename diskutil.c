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

#define SYSTEM_ID_LEN 8 // File system identifier 
#define BLOCK_SIZE_LEN 2
#define FILE_SYS_SIZE 4 // File system size
#define B_FAT_START 4 	// Block where FAT starts
#define NUM_B_FAT 4 	// Number of blocks in fat
#define B_ROOT_START 4 	// Block where root dirtectoy starts
#define NUM_B_ROOT 4 	// Number of blocks in root dir

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

unsigned int bsize = 0,
			bcount = 0,
			fatstart = 0, 
			fatblocks = 0,
			rootstart = 0,
			rootblocks = 0;

void intializeVars(unsigned char** waka){
	unsigned char *ptr = *waka;

	//Block size
	memcpy(&bsize, ptr+SYSTEM_ID_LEN, BLOCK_SIZE_LEN);
	bsize = htons(bsize);
	//Block count
	memcpy(&bcount, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN, FILE_SYS_SIZE);
	bcount = ntohl(bcount);
	// FAT starts
	memcpy(&fatstart, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE, B_FAT_START);
	fatstart = ntohl(fatstart);
	//FAT blocks
	memcpy(&fatblocks, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE+B_FAT_START, NUM_B_FAT);
	fatblocks = htonl(fatblocks);
 	//Root dir start
	memcpy(&rootstart, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE+B_FAT_START+NUM_B_FAT, B_ROOT_START);
	rootstart = htonl(rootstart);
	//Root directory blocks
	memcpy(&rootblocks, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE+B_FAT_START+NUM_B_FAT+B_ROOT_START, NUM_B_ROOT);
	rootblocks = htonl(rootblocks);
}		

int diskinfo(int argc, char* argv[], unsigned char ** waka){
	unsigned char *ptr = *waka;
	int buffer = 0,
		reserved = 0,
		free = 0,
		allocated = 0;

	printf("Super block information:\n");

	// //File system identifier
	// unsigned char id_arr[SYSTEM_ID_LEN+1]; 
	// memcpy(&id_arr, ptr, SYSTEM_ID_LEN);
	// id_arr[SYSTEM_ID_LEN] = '\0';
	// // printf("SYS ID:%s\n", id_arr);

	printf("Block size: %d\n", bsize);
	printf("Block count: %u\n", bcount);
	printf("FAT starts: %u\n", fatstart);
	printf("FAT blocks: %u\n", fatblocks);
	printf("Root directory start: %u\n", rootstart);
	printf("Root directory blocks: %u\n", rootblocks);

	for (int i = (fatstart*bsize); i < ((fatstart*bsize)+(fatblocks*bsize)); i+=4){
		buffer = 0;
		memcpy(&buffer, ptr+i, 4);
		buffer = htonl(buffer);
		// printf ("%x - Buffer is 0x%08x - ", i, buffer);
		if (buffer == 0x00){
			// printf("Free \n");
			free++;	
		}else if(buffer == 0x01){
			// printf("Reserved \n");
			reserved++;
		}else{
			// printf("Allocated \n");
			allocated++;	
		}
	}	
	printf("\nFAT information:\n");
	printf("Free blocks: %d\n", free);
	printf("Reserved blocks: %d\n", reserved);
	printf("Allocated blocks: %d\n", allocated);
	return EXIT_SUCCESS;

	
}
int disklist(int argc, char* argv[], unsigned char ** waka){
	if (argc != 3){
		perror("Usage: [file_name] [disk_image] [directory]\n");
		return EXIT_FAILURE;
	}

	printf("FROM DISKLISTS\n");
	printf("Block size: %d\n", bsize);
	printf("Block count: %u\n", bcount);
	printf("FAT starts: %u\n", fatstart);
	printf("FAT blocks: %u\n", fatblocks);
	printf("Root directory start: %u\n", rootstart);
	printf("Root directory blocks: %u\n", rootblocks);

	// dir_entry *dir1 = (dir_entry *)calloc(1, sizeof(dir_entry));

	// printf("%02x - %d\n", ptr[rootstart*bsize], rootstart*bsize);
	return EXIT_SUCCESS;
}

int diskput(int argc, char* argv[], unsigned char ** waka){
	return EXIT_SUCCESS;
}
int diskget(int argc, char* argv[], unsigned char ** waka){
	return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
	struct stat file_size;
	unsigned char *ptr;
	int file, 
		retval = EXIT_SUCCESS;

	if (argc < 2){
		perror("Usage: [file_name] [disk_image]\n");
		return EXIT_FAILURE;
	}
	file = open(argv[1], O_RDWR);
	if (file < 0){
		fprintf(stderr,"Unable to open file [%s]\n", argv[1]);
		return EXIT_FAILURE;
	}
	printf("File [%s] opened...\n\n", argv[1]);
	if (fstat(file, &file_size) != 0){
		perror("fstat() error");
		return EXIT_FAILURE;
	}

	ptr = mmap(NULL, file_size.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);

	#if defined(PART1)
		intializeVars(&ptr);
		retval = diskinfo(argc, argv, &ptr);
	#elif defined(PART2)
		intializeVars(&ptr);
		retval = disklist(argc, argv, &ptr);
	#elif defined(PART3)
		retval = diskget(argc,argv, &ptr);
	#elif defined(PART4)
		retval = diskput(argc, argv, &ptr);
	#else
	#	error "PART[1234] must be defined"
	#endif
		return retval;

	
	munmap(ptr, file_size.st_size);
	close(file);

	return retval;
}