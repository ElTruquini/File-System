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

int main(int argc, char* argv[]) {
	struct stat file_size;
	unsigned char id_arr[SYSTEM_ID_LEN+1]; 
	unsigned int bsize = 0,
				bcount = 0,
				fatstart = 0, 
				fatblocks = 0,
				rootstart = 0,
				rootblocks = 0;
	unsigned char *ptr;
	int file,
		buffer = 0,
		reserved = 0,
		free = 0,
		allocated = 0;

	if (argc != 2){
		perror("Usage: [file_name] [disk_image]\n");
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

	printf("Super block information:\n");

	//File system identifier
	memcpy(&id_arr, ptr, SYSTEM_ID_LEN);
	id_arr[SYSTEM_ID_LEN] = '\0';
	// printf("SYS ID:%s\n", id_arr);

	//Block size
	memcpy(&bsize, ptr+SYSTEM_ID_LEN, BLOCK_SIZE_LEN);
	bsize = htons(bsize);
	printf("Block size: %d\n", bsize);

	//Block count
	memcpy(&bcount, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN, FILE_SYS_SIZE);
	bcount = ntohl(bcount);
	printf("Block count: %u\n", bcount);

	// FAT starts
	memcpy(&fatstart, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE, B_FAT_START);
	fatstart = ntohl(fatstart);
	printf("FAT starts: %u\n", fatstart);

	//FAT blocks
	memcpy(&fatblocks, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE+B_FAT_START, NUM_B_FAT);
	fatblocks = htonl(fatblocks);
	printf("FAT blocks: %u\n", fatblocks);

 	//Root dir start
	memcpy(&rootstart, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE+B_FAT_START+NUM_B_FAT, B_ROOT_START);
	rootstart = htonl(rootstart);
	printf("Root directory start: %u\n", rootstart);

	//Root directory blocks
	memcpy(&rootblocks, ptr+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE+B_FAT_START+NUM_B_FAT+B_ROOT_START, NUM_B_ROOT);
	rootblocks = htonl(rootblocks);
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
	printf("\nFat information:\n");
	printf("Free blocks: %d\n", free);
	printf("Reserved blocks: %d\n", reserved);
	printf("Allocated blocks: %d\n", allocated);
	
	munmap(ptr, file_size.st_size);
	close(file);

	return EXIT_SUCCESS;
}