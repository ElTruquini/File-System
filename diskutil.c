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
#include <inttypes.h>

#define SYSTEM_ID_LEN 8 // File system identifier 
#define BLOCK_SIZE_LEN 2
#define FILE_SYS_SIZE 4 // File system size
#define B_FAT_START 4 	// Block where FAT starts
#define NUM_B_FAT 4 	// Number of blocks in fat
#define B_ROOT_START 4 	// Block where root dirtectoy starts
#define NUM_B_ROOT 4 	// Number of blocks in root dir

typedef struct __attribute__((__packed__))dir_entry_timedate_t{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
}dir_timedate;

typedef struct __attribute__((__packed__))dir_entry_t{
	uint8_t status;
	uint32_t starting_block;
	uint32_t block_count;
	uint32_t size;
	struct dir_entry_timedate_t create_time;
	struct dir_entry_timedate_t modify_time;
	uint8_t filename[31];
	uint8_t unused[6];
}dir_entry;

unsigned int bsize = 0,
			bcount = 0,
			fatstart = 0, 
			fatblocks = 0,
			rootstart = 0,
			rootblocks = 0;

void printInitVars(){

	printf("Block size: %d\n", bsize);
	printf("Block count: %u\n", bcount);
	printf("FAT starts: %u\n", fatstart);
	printf("FAT blocks: %u\n", fatblocks);
	printf("Root directory start: %u\n", rootstart);
	printf("Root directory blocks: %u\n", rootblocks);
}

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

int disklist(int argc, char* argv[], unsigned char ** waka){
	unsigned char *ptr = *waka;
	if (argc != 3){
		perror("Usage: [file_name] [disk_image] [directory]\n");
		return EXIT_FAILURE;
	}

	intializeVars(&ptr);
	printInitVars(); //TODO: REMOVE - JUST FOR TESTING

	dir_entry *dir = (dir_entry *)calloc(1,sizeof(dir_entry));
	
	int dir_blocks = rootblocks; // TODO: THIS WILL HAVE THE NUMBER OF BLOCKS FOR THE FILE
	
	int entry_size = bsize/sizeof(dir_entry);
	printf("dir_blocks:%d | entry_size:%d\n", dir_blocks, entry_size);
	

	for (int i = 0 ; i < 1/*dir_blocks*/ ; i++){
		for (int j = 0 ; j < entry_size ; j++){
			memcpy(&*dir, ptr+(rootstart*bsize)+(i*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
			// printf("\nhex:%x(%d) | Entry [j:%d] | dir->status:%d\n",(rootstart*bsize)+(i*bsize)+(j*sizeof(*dir)), (rootstart*bsize)+(i*bsize)+(j*sizeof(*dir)), j, dir->status);
			if(dir->status != 0){
				if (dir->status == 3){
					printf("F ");
				}else  { 
					printf("D ");
				}
				printf("%10lu ",(unsigned long)htonl(dir->size));
				printf("%29s ", dir->filename);
				printf("%04u/", htons(dir->modify_time.year));
				printf("%02d/", dir->modify_time.month);
				printf("%02d ", dir->modify_time.day);
				printf("%02d:", dir->modify_time.hour);
				printf("%02d:", dir->modify_time.minute);
				printf("%02d", dir->modify_time.second);
				printf("\n");
			}else {}
		}
		printf("\n=====Total blocks:%d | End of block i:%d=====\n",dir_blocks, i);

	}
	int i = 0;
	printf("\n");
	unsigned long b_address[dir_blocks];

	int offset =(fatstart*bsize)+1;
	memcpy(&b_address+i, ptr+offset, sizeof(int));
	printf("Before htonl:%08x\n",(unsigned long) *(b_address+i));
	*(b_address+i) = (unsigned long)htonl(*(b_address+i));
	printf("After htonl:%08x\n", (unsigned long) *(b_address+i));

	printf("Entry:%x (%d) | B_address:%x (%d)\n",(unsigned int)(offset), (unsigned int)(offset), (unsigned int)*(b_address+i), (unsigned int)*(b_address+i));


	//Padding file name to left
	// char buffer[31];
	// int ctr = 0;
	// for(int z = 0 ; z < 31 ; z++){
	// 	if (dir->filename[z] != 0){
	// 		// printf("Appending:%c\n",dir->filename[z]);
	// 		buffer[ctr] = dir->filename[z];
	// 		ctr++;
	// 	}
	// }
	// buffer[ctr] = '\0';
	// printf("%29s", buffer);

	return EXIT_SUCCESS;
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

	intializeVars(&ptr);
	printInitVars();

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
		retval = diskinfo(argc, argv, &ptr);
	#elif defined(PART2)
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