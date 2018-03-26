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
#define MAX_HEIGHT 10	// Maximum tree directory height
#define FILE_NAME_LEN 31

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
unsigned char *ptr;
unsigned int *b_address_ptr;


static void printInitVars();
static void intializeVars(unsigned char** waka);
static void printDir(dir_entry **waka);
static int disklist(int argc, char* argv[]);
int diskinfo(int argc, char* argv[], unsigned char ** waka);
// static int diskput(int argc, char* argv[], unsigned char ** waka);
// static int diskget(int argc, char* argv[], unsigned char ** waka);
static void getblocks(const int dir_blocks);



static void getblocks(const int dir_blocks){

	for(int i = 0 ; i < dir_blocks ; i++) printf("GETBLOCKS initial - b_address_arr[%d]:0x%08x (%u) \n", i, *(b_address_ptr+i), *(b_address_ptr+i));

	printf("GETBLOCKS: Getting all Root addresses: %d blocks\n", dir_blocks);

	for (int i = 1 ; i < dir_blocks ; i++){


		unsigned int offset = *(b_address_ptr+i-1)*sizeof(int);
		memcpy(&*(b_address_ptr+i), ptr+(fatstart*bsize)+offset, sizeof(int));
		*(b_address_ptr+i) = ntohl(*(b_address_ptr+i));
		// printf("Offset:%x (%u) | After htonl:%08x\n", offset, offset, *(b_address_ptr+i-1));
	}

}

static void printInitVars(){

	printf("Block size: %d\n", bsize);
	printf("Block count: %u\n", bcount);
	printf("FAT starts: %u\n", fatstart);
	printf("FAT blocks: %u\n", fatblocks);
	printf("Root directory start: %u\n", rootstart);
	printf("Root directory blocks: %u\n", rootblocks);
}

static void intializeVars(unsigned char** waka){
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

static int disklist(int argc, char* argv[]){
	char *path = (char*)malloc(10*FILE_NAME_LEN*sizeof(char));
	dir_entry *dir = (dir_entry *)calloc(1,sizeof(dir_entry));
	int entry_size, 
		dir_blocks = 0,
		root_flag = 0,
		dir_depth = 0;

	if (argc != 3){
		perror("disklist - Usage: [file_name] [disk_image] [directory]\n");
		return EXIT_FAILURE;
	}

	intializeVars(&ptr);
	printInitVars(); //TODO: REMOVE - JUST FOR TESTING

	entry_size = bsize/sizeof(dir_entry);


	// Searching through Root
	if (strcmp(argv[2], "/") == 0){
		printf("\ndisklist - Listing Root directory\n");
		root_flag = 1;

	//Tokenizing search path
	}else{ 
		printf("disklist - Listing else: %s\n", argv[2]);
		const char s[2] = "/";
		char *token;
		token = strtok(argv[2], s);
		strcpy(path+dir_depth*FILE_NAME_LEN, token);
		dir_depth++;
				while (1){
			token = strtok(NULL, s);
			if (token == NULL){
				break;
			}
			strcpy(path+dir_depth*FILE_NAME_LEN, token);
			dir_depth++;
		}
		for (int i = 0 ; i < dir_depth ; i++) printf("Path[%d]:%s\n", i, path+i*FILE_NAME_LEN);
	}



	// Getting block addresses
	int ctr = 0, got_blocks = 0;
	while (!got_blocks){
		//Getting root blocks addresses
		if (ctr == 0){
			dir_blocks = rootblocks; 
			b_address_ptr = (unsigned int *)malloc(dir_blocks*sizeof(unsigned int));
			memset(b_address_ptr, 0, dir_blocks*sizeof(unsigned int));
			for(int i = 0 ; i < dir_blocks ; i++) printf("PRE getblocks- b_address_ptr+%d:0x%x (%u)  \n", i, *(b_address_ptr+i), *(b_address_ptr+i));
			*b_address_ptr = rootstart;

			getblocks(dir_blocks);	
			for(int i = 0 ; i < dir_blocks ; i++) printf("POST getblocks- b_address_ptr+%d:0x%x (%u)  \n", i, *(b_address_ptr+i), *(b_address_ptr+i));
			return EXIT_SUCCESS;
		}

		if (root_flag){
			got_blocks = 1;
		}else{
			int path_found = 0;
			while (!path_found){

				printf("else dir_blocks:%d\n", dir_blocks);
				for (int i = 0 ; i < dir_blocks && !path_found ; i++){
					for (int j = 0 ; j < entry_size ; j++){
						memcpy(&*dir, ptr+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
						printf("COMPARING filename:%s ?= path:%s\n", (char *)dir->filename, path+ctr*FILE_NAME_LEN);


						if(strcmp((char*)dir->filename, path+ctr*FILE_NAME_LEN) == 0){
							printf("FOUND %s | new dir_blocks:%d | new first_block:%08x (%d)\n",
									(char*)dir->filename, htonl((int)dir->block_count), ntohl((int)dir->starting_block), ntohl((int)dir->starting_block));
							dir_blocks = htonl((unsigned int)dir->block_count);
							free(b_address_ptr);
							b_address_ptr = (unsigned int *)calloc(dir_blocks, sizeof(unsigned int));
							*b_address_ptr = ntohl((int)dir->starting_block);
							path_found = 1;
							break;
						}
					}
				}
				
			}

		}
		ctr++;
		printf("dir_depth-1:%d - ctr:%d\n", dir_depth-1, ctr);
		if (dir_depth-1 == ctr){
			got_blocks = 1;
		}
	}

	printf("disklist - Final b_address_ptr: 0x%08x (%d) = 0x%08x byte\n", *b_address_ptr, *b_address_ptr, *b_address_ptr*bsize );
		return EXIT_SUCCESS;

	printf("\n");

	// Data blocks: Visits all block locations from dir, prints contents
	for (int i = 0 ; i < dir_blocks ; i++){
		for (int j = 0 ; j < entry_size ; j++){
			memcpy(&*dir, ptr+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
			// printf("hex:%x(%lu) | Entry: [j:%d] | status:%d\n",(unsigned int)((rootstart*bsize)+(i*bsize)+(j*sizeof(*dir))),
			// 			 (rootstart*bsize)+(i*bsize)+(j*sizeof(*dir)), j, dir->status);
			if(dir->status != 0){
				printDir(&dir);
			}else {
			}
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
		retval = disklist(argc, argv);
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