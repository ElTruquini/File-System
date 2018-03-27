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
static void getblocks(const int dir_blocks);
static void tokenizePath(int *d, char *argv[], char *path);
static int blockIO(int dir_blocks, int entry_size, int IO, char *argv[]);
int disklist(int argc, char* argv[]);
int diskinfo(int argc, char* argv[]);
int diskput(int argc, char* argv[]);
int diskget(int argc, char* argv[]);




static void getblocks(const int dir_blocks){
	// for(int i = 0 ; i < dir_blocks ; i++) printf("GETBLOCKS initial - b_address_arr[%d]:0x%08x (%u) \n", i, *(b_address_ptr+i), *(b_address_ptr+i));
	// printf("GETBLOCKS: Getting all Root addresses: %d blocks\n", dir_blocks);
	for (int i = 1 ; i < dir_blocks ; i++){
		unsigned int offset = *(b_address_ptr+i-1)*sizeof(int);
		memcpy(&*(b_address_ptr+i), ptr+(fatstart*bsize)+offset, sizeof(int));
		*(b_address_ptr+i) = ntohl(*(b_address_ptr+i));
		// printf("Offset:0x%x (%u)| ptr:0x%x (%d) | b_address_ptr:%08x (%d)\n", offset, offset,(fatstart*bsize)+offset,(fatstart*bsize)+offset, *(b_address_ptr+i), *(b_address_ptr+i));
	}
	// for(int i = 0 ; i < dir_blocks ; i++) printf("GETBLOCKS final - b_address_arr[%d]:0x%08x (%u) \n", i, *(b_address_ptr+i), *(b_address_ptr+i));

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

static void tokenizePath(int *dir_depth, char *argv[], char *path){
	const char s[2] = "/";
	char *token;
	token = strtok(argv[2], s);
	strcpy(path+(*dir_depth)*FILE_NAME_LEN, token);
	*dir_depth += 1;

	while (1){
		token = strtok(NULL, s);
		if (token == NULL){
			break;
		}
		strcpy(path+(*dir_depth)*FILE_NAME_LEN, token);
		*dir_depth += 1;
	}
}

// Visits (dir_blocks) block locations from b_address_ptr,
// if IO == 1, block contents will be written to file args[3]
// if IO == 0, block content will print on screen
static int blockIO(int dir_blocks, int entry_size,  int IO, char *args[] ){
	printf("blockIO | dir_blocks:%d |entry_size:%d | bsize:%d\n", dir_blocks, entry_size, bsize);
	for(int i = 0 ; i < dir_blocks ; i++) printf("blockIO b_address_ptr+%d:0x%x (%u)  \n", i, *(b_address_ptr+i), *(b_address_ptr+i));

    // Write blocks to file
	if(IO == 1){
    	FILE* f = fopen(args[3], "wb");
		for (int i = 0 ; i < dir_blocks ; i++){
			unsigned int offset = (*(b_address_ptr+i)*bsize);
			// printf("offset:%u - %08x\n", offset, offset);
			if (fwrite(ptr+offset, sizeof(char), bsize, f) != bsize){
				fprintf(stderr, "fwrite() error occurred\n" );
				return EXIT_FAILURE;
			}
		}	
		printf("File [%s] succesfully copied to [%s]\n", args[2], args[3]);
		fclose(f);
	}

    //Printf blocks to content
	if(IO == 0){
		dir_entry *dir = (dir_entry *)calloc(1,sizeof(dir_entry));
		for (int i = 0 ; i < dir_blocks ; i++){
			for (int j = 0 ; j < entry_size ; j++){
				memcpy(&*dir, ptr+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
				// printf("hex:%x(%lu) | Entry: [j:%d] | status:%d\n",(unsigned int)((rootstart*bsize)+(i*bsize)+(j*sizeof(*dir))),(rootstart*bsize)+(i*bsize)+(j*sizeof(*dir)), j, dir->status);
				if(dir->status != 0){
					printDir(&dir);
				}else {
				}
			}
			// printf("\n=====Total blocks:%d | End of block i:%d=====\n",dir_blocks, i);
		}
		free(dir);
	}


	return EXIT_SUCCESS;
}



int disklist(int argc, char* argv[]){
	char *path = (char*)malloc(10*FILE_NAME_LEN*sizeof(char));
	dir_entry *dir = (dir_entry *)calloc(1,sizeof(dir_entry));
	if (dir == NULL || path == NULL){
		fprintf(stderr, "disklist - malloc(path, dir)  failed: insufficient memory!\n");
		return EXIT_FAILURE;
	}
	int entry_size, 
		dir_blocks = 0,
		root_flag = 0,
		dir_depth = 0;

	if (argc != 3){
		fprintf(stderr,"disklist - Usage: [file_name] [disk_image] [path]\n");
		return EXIT_FAILURE;
	}

	intializeVars(&ptr);
	printInitVars(); //TODO: REMOVE - JUST FOR TESTING

	//Must be intialized after intializeVars
	entry_size = bsize/sizeof(dir_entry);


	if (strcmp(argv[2], "/") == 0){
		root_flag = 1;
	}else{ 
		tokenizePath(&dir_depth, argv, path);
		// for (int i = 0 ; i < dir_depth ; i++) printf("Path[%d]:%s | dir_depth:%d \n", i, path+i*FILE_NAME_LEN, dir_depth);
	}
	
	int ctr = 0, 
		got_blocks = 0, 
		path_found = 0;;
	while (!got_blocks){
		//Getting root blocks addresses
		if (ctr == 0){
			dir_blocks = rootblocks; 
			b_address_ptr = (unsigned int *)malloc(dir_blocks*sizeof(unsigned int));
			if (b_address_ptr == NULL){
				fprintf(stderr, "malloc(b_address_ptr) failed: insufficient memory!\n");
        		return EXIT_FAILURE;
			}
			memset(b_address_ptr, 0, dir_blocks*sizeof(unsigned int));
			*b_address_ptr = rootstart;
			getblocks(dir_blocks);	
		}
		// for(int i = 0 ; i < dir_blocks ; i++) printf("POST getblocks- b_address_ptr+%d:0x%x (%u)  \n", i, *(b_address_ptr+i), *(b_address_ptr+i));
		if (root_flag){
			got_blocks = 1;
			path_found = 1;
		}else{
			path_found = 0;
			// Loops through all the dir blocks and compares  file_name with search path for each entry, 
			// If success(dir found), it retrieves all the new block addresses and stores in b_address_ptr
			while (!path_found){
				// printf("else dir_blocks:%d \n", dir_blocks);
				for (int i = 0 ; i < dir_blocks && !path_found ; i++){
					for (int j = 0 ; j < entry_size ; j++){
						memcpy(&*dir, ptr+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
						// printf("COMPARING filename:%s ?= path:%s\n", (char *)dir->filename, path+ctr*FILE_NAME_LEN);
						if(strcmp((char*)dir->filename, path+ctr*FILE_NAME_LEN) == 0){
							// printf("COMPARING FOUND! %s | new dir_blocks:%d | new first_block:%08x (%d)\n", (char*)dir->filename, htonl((int)dir->block_count), ntohl((int)dir->starting_block), ntohl((int)dir->starting_block));
							dir_blocks = htonl((unsigned int)dir->block_count);
							free(b_address_ptr);
							b_address_ptr = (unsigned int *)calloc(dir_blocks, sizeof(unsigned int));
							if (b_address_ptr == NULL){
								fprintf(stderr, "malloc(b_address_ptr) failed: insufficient memory!\n");
				        		return EXIT_FAILURE;
							}
							*b_address_ptr = ntohl((int)dir->starting_block);
							path_found = 1;
							// for(int i = 0 ; i < dir_blocks ; i++) printf("PRE getblocks- b_address_ptr+%d:0x%x (%u)  \n", i, *(b_address_ptr+i), *(b_address_ptr+i));
							getblocks(dir_blocks);	
							break;
						}
					}
				}
				ctr++;
				// printf("dir_depth:%d - ctr:%d\n", dir_depth, ctr);
				if (dir_depth == ctr){
					got_blocks = 1;
					break;
				}
			}
		}
	}
	if (!path_found){
		fprintf(stderr, "Error - Path not found\n");
		return EXIT_FAILURE;
	}
	
	//Visits all block locations from dir, printf struct dir_entry contents
	if (blockIO(dir_blocks, entry_size, 0, argv) == EXIT_FAILURE){
		fprintf(stderr, "Error ocurred blockIO()\n");
		return EXIT_FAILURE;
	}

	free(path);
	free(b_address_ptr);
	free(dir);

	return EXIT_SUCCESS;
}



int diskinfo(int argc, char* argv[]){
	int buffer = 0,
		reserved = 0,
		free = 0,
		allocated = 0;

	if (argc != 2){
		fprintf(stderr,"diskget - Usage: [file_name] [disk_image]\n");
		return EXIT_FAILURE;
	}

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


int diskput(int argc, char* argv[]){
	char *path = (char*)malloc(10*FILE_NAME_LEN*sizeof(char));
	dir_entry *dir = (dir_entry *)calloc(1,sizeof(dir_entry));
	if (dir == NULL || path == NULL){
		fprintf(stderr, "diskput - malloc(path, dir)  failed: insufficient memory!\n");
		return EXIT_FAILURE;
	}
	int entry_size, 
		dir_blocks = 0,
		root_flag = 0,
		dir_depth = 0;

	if (argc != 4){
		fprintf(stderr,"diskput - Usage: [file_name] [disk_image] [source_file] [dest_path]\n");
		return EXIT_FAILURE;
	}



	return EXIT_SUCCESS;
}

int diskget(int argc, char* argv[]){
	char *path = (char*)malloc(10*FILE_NAME_LEN*sizeof(char));
	dir_entry *dir = (dir_entry *)calloc(1,sizeof(dir_entry));
	if (dir == NULL || path == NULL){
		fprintf(stderr, "diskget - malloc(path, dir)  failed: insufficient memory!\n");
		return EXIT_FAILURE;
	}
	int entry_size, 
		dir_blocks = 0,
		root_flag = 0,
		dir_depth = 0;

	if (argc != 4){
		fprintf(stderr,"diskget - Usage: [file_name] [disk_image] [path] [dest_file_name]\n");
		return EXIT_FAILURE;
	}

	intializeVars(&ptr);
	printInitVars(); //TODO: REMOVE - JUST FOR TESTING

	//Must be intialized after intializeVars
	entry_size = bsize/sizeof(dir_entry);

	if (strcmp(argv[2], "/") == 0){
		root_flag = 1;
	}else{ 
		tokenizePath(&dir_depth, argv, path);
		// for (int i = 0 ; i < dir_depth ; i++) printf("Path[%d]:%s | dir_depth:%d \n", i, path+i*FILE_NAME_LEN, dir_depth);
	}

	int ctr = 0, 
		got_blocks = 0, 
		path_found = 0;;
	while (!got_blocks){
		//Getting root blocks addresses
		if (ctr == 0){
			dir_blocks = rootblocks; 
			b_address_ptr = (unsigned int *)malloc(dir_blocks*sizeof(unsigned int));
			if (b_address_ptr == NULL){
				fprintf(stderr, "malloc(b_address_ptr) failed: insufficient memory!\n");
        		return EXIT_FAILURE;
			}
			memset(b_address_ptr, 0, dir_blocks*sizeof(unsigned int));
			*b_address_ptr = rootstart;
			getblocks(dir_blocks);	
		}
		// for(int i = 0 ; i < dir_blocks ; i++) printf("POST getblocks- b_address_ptr+%d:0x%x (%u)  \n", i, *(b_address_ptr+i), *(b_address_ptr+i));
		if (root_flag){
			got_blocks = 1;
			path_found = 1;
		}else{
			path_found = 0;
			// Loops through all the dir blocks and compares  file_name with search path for each entry, 
			// If success(dir found), it retrieves all the new block addresses and stores in b_address_ptr
			while (!path_found){
				// printf("else dir_blocks:%d \n", dir_blocks);
				for (int i = 0 ; i < dir_blocks && !path_found ; i++){
					for (int j = 0 ; j < entry_size ; j++){
						memcpy(&*dir, ptr+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
						// printf("COMPARING filename:%s ?= path:%s\n", (char *)dir->filename, path+ctr*FILE_NAME_LEN);
						if(strcmp((char*)dir->filename, path+ctr*FILE_NAME_LEN) == 0){
							// printf("COMPARING FOUND! %s | new dir_blocks:%d | new first_block:%08x (%d)\n", (char*)dir->filename, htonl((int)dir->block_count), ntohl((int)dir->starting_block), ntohl((int)dir->starting_block));
							dir_blocks = htonl((unsigned int)dir->block_count);
							free(b_address_ptr);
							b_address_ptr = (unsigned int *)calloc(dir_blocks, sizeof(unsigned int));
							if (b_address_ptr == NULL){
								fprintf(stderr, "malloc(b_address_ptr) failed: insufficient memory!\n");
				        		return EXIT_FAILURE;
							}
							*b_address_ptr = ntohl((int)dir->starting_block);
							path_found = 1;
							// for(int i = 0 ; i < dir_blocks ; i++) printf("PRE getblocks- b_address_ptr+%d:0x%x (%u)  \n", i, *(b_address_ptr+i), *(b_address_ptr+i));
							getblocks(dir_blocks);	
							break;
						}
					}
				}
				ctr++;
				// printf("dir_depth:%d - ctr:%d\n", dir_depth, ctr);
				if (dir_depth == ctr){
					got_blocks = 1;
					break;
				}
			}
		}
	}
	if (!path_found){
		fprintf(stderr, "Error - Path not found\n");
		return EXIT_FAILURE;
	}
	for(int i = 0 ; i < dir_blocks ; i++) printf("POST getblocks- b_address_ptr+%d:0x%x (%u)  \n", i, *(b_address_ptr+i), *(b_address_ptr+i));
	printf("\n");

	//Visits all block locations from dir, printf struct dir_entry contents
	if (blockIO(dir_blocks, entry_size, 1, argv) == EXIT_FAILURE){
		fprintf(stderr, "Error ocurred blockIO()\n");
		return EXIT_FAILURE;
	}
	free(path);
	free(dir);
	free(b_address_ptr);

	return EXIT_SUCCESS;
}


int main(int argc, char* argv[]) {
	struct stat file_size;
	
	int file, 
		retval = EXIT_SUCCESS;

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
		retval = diskinfo(argc, argv);
	#elif defined(PART2)
		retval = disklist(argc, argv);
	#elif defined(PART3)
		retval = diskget(argc,argv);
	#elif defined(PART4)
		retval = diskput(argc, argv);
	#else
	#	error "PART[1234] must be defined"
	#endif
		return retval;

	
	munmap(ptr, file_size.st_size);
	close(file);

	return retval;
}