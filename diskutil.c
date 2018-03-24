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

static void printDir(dir_entry **waka){
	dir_entry *dir = *waka;

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
}		

static int disklist(int argc, char* argv[], unsigned char ** waka){
	unsigned char *ptr = *waka;
	dir_entry *dir = (dir_entry *)calloc(1,sizeof(dir_entry));
	int entry_size, dir_blocks;
	int root_flag = 0;

	if (argc != 3){
		perror("Usage: [file_name] [disk_image] [directory]\n");
		return EXIT_FAILURE;
	}

	intializeVars(&ptr);
	printInitVars(); //TODO: REMOVE - JUST FOR TESTING

	entry_size = bsize/sizeof(dir_entry);
	// printf("\ndir_blocks:%d | entry_size:%d\n", dir_blocks, entry_size);


	// Checks if Root dir and defines dir_blocks
	if (strcmp(argv[2], "/") == 0){
		root_flag = 1;
		dir_blocks = rootblocks; 

		printf("\nINFO: Listing Root directory\n");
	}else{
		printf("INFO: Listing else\n");

		//TODO: Need to set dir_blocks for else
		//TODO: parse string

		printf("argv[2]:%s\n", argv[2]);
		const char s[2] = "/";



	}

	unsigned int b_address[dir_blocks];
	memset(b_address, 0, sizeof(int)*dir_blocks);
	
	// Sets first address of dir 
	if (root_flag){
		b_address[0] = rootstart; 
	}else{
		//TODO: Need to set b_address[0] for other dir
	}


	//FAT Table: Gets all block locations for dir and stores result in b_address array
	if (root_flag){
		printf("INFO: Getting all Root addresses: %d blocks\n", dir_blocks);
		for (int i = 1 ; i < dir_blocks ; i++){
			int offset = (b_address[i-1]*sizeof(int));
			memcpy(b_address+i, ptr+(fatstart*bsize)+offset, sizeof(int));
			b_address[i] = ntohl(b_address[i]);
			// printf("Offset:%x (%d) | After htonl:%08x\n", offset, offset, b_address[i]);
		}
		for (int i = 0 ; i < dir_blocks ; i ++) printf("INFO: b_address[%i]: %x (%d) = %d byte \n", i, b_address[i], b_address[i], b_address[i]*bsize);
	}else{
		//TODO: WHEN NOT ROOT
	}


	printf("\n");
	// Data blocks: Visits all block locations from dir
	for (int i = 0 ; i < dir_blocks ; i++){
		for (int j = 0 ; j < entry_size ; j++){
			memcpy(&*dir, ptr+(b_address[i]*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
			printf("hex:%x(%lu) | Entry: [j:%d] | status:%d\n",(unsigned int)((rootstart*bsize)+(i*bsize)+(j*sizeof(*dir))),
						 (rootstart*bsize)+(i*bsize)+(j*sizeof(*dir)), j, dir->status);
			if(dir->status != 0){
				printDir(&dir);
			}else {}
		}
		printf("\n=====Total blocks:%d | End of block i:%d=====\n",dir_blocks, i);
	}
	


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