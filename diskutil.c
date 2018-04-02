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
unsigned char *ptr_imgc;
unsigned int *ptr_imgu;

unsigned int *b_address_ptr;

static void printInitVars();
static void intializeVars(unsigned char** waka);
static void printDir(dir_entry **waka);
static void getBlocks(const int dir_blocks);
static void tokenizePath(int *d, char *argv[], char *path, int offset);
static int blockIO(int dir_blocks, int entry_size, int IO, char *argv[]);
static void getFreeBlock(unsigned int *newf_blocks, const int blocks_needed, const int dir_blocks);
int disklist(int argc, char* argv[]);
int diskinfo(int argc, char* argv[]);
int diskput(int argc, char* argv[], size_t file_len);
int diskget(int argc, char* argv[]);

//  Gets all block addresses from a dir starting at b_address_ptr[0] 
static void getBlocks(const int dir_blocks){
	for (int i = 1 ; i < dir_blocks ; i++){
		unsigned int offset = *(b_address_ptr+i-1)*sizeof(int);
		memcpy(&*(b_address_ptr+i), ptr_imgc+(fatstart*bsize)+offset, sizeof(int));
		*(b_address_ptr+i) = ntohl(*(b_address_ptr+i));
	}
}

// disklist helper method, prints general info from filesystem
static void printInitVars(){
	printf("Block size: %d\n", bsize);
	printf("Block count: %u\n", bcount);
	printf("FAT starts: %u\n", fatstart);
	printf("FAT blocks: %u\n", fatblocks);
	printf("Root directory start: %u\n", rootstart);
	printf("Root directory blocks: %u\n", rootblocks);
}

//disklist helper method, reads information from superblock 
static void intializeVars(unsigned char** waka){
	unsigned char *ptr_imgc = *waka;

	//Block size
	memcpy(&bsize, ptr_imgc+SYSTEM_ID_LEN, BLOCK_SIZE_LEN);
	bsize = htons(bsize);
	//Block count
	memcpy(&bcount, ptr_imgc+SYSTEM_ID_LEN+BLOCK_SIZE_LEN, FILE_SYS_SIZE);
	bcount = ntohl(bcount);
	// FAT starts
	memcpy(&fatstart, ptr_imgc+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE, B_FAT_START);
	fatstart = ntohl(fatstart);
	//FAT blocks
	memcpy(&fatblocks, ptr_imgc+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE+B_FAT_START, NUM_B_FAT);
	fatblocks = htonl(fatblocks);
	//Root dir start
	memcpy(&rootstart, ptr_imgc+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE+B_FAT_START+NUM_B_FAT, B_ROOT_START);
	rootstart = htonl(rootstart);
	//Root directory blocks
	memcpy(&rootblocks, ptr_imgc+SYSTEM_ID_LEN+BLOCK_SIZE_LEN+FILE_SYS_SIZE+B_FAT_START+NUM_B_FAT+B_ROOT_START, NUM_B_ROOT);
	rootblocks = htonl(rootblocks);
}

//disklist helper method, prints directory (given as arg) metadata on screen
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

// Tokenizes path given and stores tree structure in path[]
static void tokenizePath(int *dir_depth, char *argv[], char *path, int offset){
	const char s[2] = "/";
	char *token;
	token = strtok(argv[offset], s);
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
	// Write blocks to file
	if(IO == 1){
		FILE* f = fopen(args[3], "wb");
		for (int i = 0 ; i < dir_blocks-1 ; i++){
			unsigned int offset = (*(b_address_ptr+i)*bsize);
			if (fwrite(ptr_imgc+offset, sizeof(char), bsize, f) != bsize){
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
				memcpy(&*dir, ptr_imgc+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
				if(dir->status != 0){
					printDir(&dir);
				}else {
				}
			}
		}
		free(dir);
	}
	return EXIT_SUCCESS;
}

// Displays contents of root directory or a given sub-directory in the file system
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
		fprintf(stderr,"[ERROR] disklist usage: disklist [disk_image] [path]\n");
		return EXIT_FAILURE;
	}

	intializeVars(&ptr_imgc);

	//Must be intialized after intializeVars
	entry_size = bsize/sizeof(dir_entry);


	if (strcmp(argv[2], "/") == 0){
		root_flag = 1;
	}else{ 
		tokenizePath(&dir_depth, argv, path, 2);
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
			getBlocks(dir_blocks);	
		}
		if (root_flag){
			got_blocks = 1;
			path_found = 1;
		}else{
			path_found = 0;
			// Loops through all the dir blocks and compares  file_name with search path for each entry, 
			// If success(dir found), it retrieves all the new block addresses and stores in b_address_ptr
			while (!path_found){
				for (int i = 0 ; i < dir_blocks && !path_found ; i++){
					for (int j = 0 ; j < entry_size ; j++){
						memcpy(&*dir, ptr_imgc+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
						if(strcmp((char*)dir->filename, path+ctr*FILE_NAME_LEN) == 0){
							dir_blocks = htonl((unsigned int)dir->block_count);
							free(b_address_ptr);
							b_address_ptr = (unsigned int *)calloc(dir_blocks, sizeof(unsigned int));
							if (b_address_ptr == NULL){
								fprintf(stderr, "malloc(b_address_ptr) failed: insufficient memory!\n");
								return EXIT_FAILURE;
							}
							*b_address_ptr = ntohl((int)dir->starting_block);
							path_found = 1;
							getBlocks(dir_blocks);	
							break;
						}
					}
				}
				ctr++;
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

// Displays information of the filysystem reading superblock metadata.
int diskinfo(int argc, char* argv[]){
	int buffer = 0,
		reserved = 0,
		free = 0,
		allocated = 0;

	if (argc != 2){
		fprintf(stderr,"[ERROR] diskinfo usage: diskinfo [disk_image]\n");
		return EXIT_FAILURE;
	}

	printf("Super block information:\n");
	intializeVars(&ptr_imgc);
	printInitVars();

	for (int i = (fatstart*bsize); i < ((fatstart*bsize)+(fatblocks*bsize)); i+=4){
		buffer = 0;
		memcpy(&buffer, ptr_imgc+i, 4);
		buffer = htonl(buffer);
		if (buffer == 0x00){
			free++;	
		}else if(buffer == 0x01){
			reserved++;
		}else{
			allocated++;	
		}
	}	
	printf("\nFAT information:\n");
	printf("Free blocks: %d\n", free);
	printf("Reserved blocks: %d\n", reserved);
	printf("Allocated blocks: %d\n", allocated);
	
	return EXIT_SUCCESS;
}

//Diskput helper method, copies binary file to newf_blocks location
static int copyFile(char *args[], unsigned int *blocks, const int num_blocks, size_t fsize){
	FILE* f = fopen(args[2], "rb");
	if (!f){
		fprintf(stderr, "Unable to open file %s", args[2]);
		return EXIT_FAILURE;
	}
	unsigned char *buff = (unsigned char *)malloc(fsize+1);
	if (buff == NULL){
		fprintf(stderr, "malloc(buff) failed: insufficient memory!\n");
		return EXIT_FAILURE;
	}
	int bread = 0, j = 0;
	for (int i = 0 ; i < fsize ; i += bread ){
		memset(buff, 0, bsize);
		bread = fread(buff,sizeof(unsigned char), bsize, f);
		memcpy(ptr_imgc+(*(blocks+j)*bsize), buff, bread);
		j++;
	}
	free(buff);
	printf("File succesfully copied to destination...\n");
	return EXIT_SUCCESS;
}

// Copies a file from the current linux directory into the file system at the root directory or a given sub-directory
int diskput(int argc, char* argv[], size_t file_len){
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
		fprintf(stderr,"[ERROR] diskput usage: diskput [disk_image] [source_file] [dest_path]\n");
		return EXIT_FAILURE;
	}
	intializeVars(&ptr_imgc);

	//Must be intialized after intializeVars
	entry_size = bsize/sizeof(dir_entry);

	if (strcmp(argv[3], "/") == 0){
		root_flag = 1;
	}else{ 
		tokenizePath(&dir_depth, argv, path, 3);
		// for (int i = 0 ; i < dir_depth ; i++) printf("Path[%d]:%s | dir_depth:%d \n", i, path+i*FILE_NAME_LEN, dir_depth);
	}

	struct stat file_size;
	int source_file;
	source_file = open(argv[2], O_RDONLY);
	if (source_file < 0){
		fprintf(stderr,"Unable to open file [%s]\n", argv[2]);
		return EXIT_FAILURE;
	}
	if (fstat(source_file, &file_size) != 0){
		perror("fstat() error\n");
		return EXIT_FAILURE;
	}
	close(source_file);
	int blocks_needed = (file_size.st_size + bsize - 1) / bsize;

	//Getting the number of free blocks for path and compare with needed
	unsigned int *newf_blocks = (unsigned int *)calloc(blocks_needed+1, sizeof(int));
	int ctr = 0, 
		got_blocks = 0;
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
			getBlocks(dir_blocks);	
		}
		//Gets FAT blocks that will be used for new file
		getFreeBlock(newf_blocks, blocks_needed, dir_blocks);

		int found_available = 0;
		//Write file info in first ROOT DIR available 
		for (int i = 0 ; (i < dir_blocks) && !found_available ; i++){
			for (int j = 0 ; j < entry_size ; j++){
				memcpy(&*dir, ptr_imgc+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
				if(dir->status == 0){
					dir->status = 3;
					dir->starting_block = htonl(*(newf_blocks));
					dir->block_count = htonl(blocks_needed+1);
					dir->size = htonl(file_size.st_size);
					strcpy((char*)dir->filename, argv[2]);
					memcpy(ptr_imgc+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)), dir, sizeof(*dir));

					found_available = 1;
					break;
				}else {
				}
			}
		}

		if (copyFile(argv, newf_blocks, blocks_needed, file_size.st_size) == EXIT_FAILURE){
			fprintf(stderr, "Error ocurred blockIO()\n");
			return EXIT_FAILURE;
		}
		if (msync(ptr_imgc, file_len , MS_SYNC ) < 0 ) {
			perror("msync failed with error\n");
			return EXIT_FAILURE;
		}
		if (root_flag){
			got_blocks = 1;
		}

	}
	free(path);
	free(newf_blocks);
	free(dir);
	free(b_address_ptr);
	return EXIT_SUCCESS;
}

// Updates file allocation table for the number of blocks required to store the new file, 
// saves block numbers assigned to new file in newf_blocks[]
static void getFreeBlock(unsigned int *newf_blocks, const int blocks_needed, const int dir_blocks){
	unsigned int next, found = 0;

	for (int i = 0 ; (i < (fatblocks*bsize)/sizeof(int)) && (found != blocks_needed+1) ; i++){
			int offset = ((fatstart*bsize)/sizeof(int))+i;
			memcpy(&next, ptr_imgu+offset, sizeof(int));
			next = ntohl(next);
			if (next == 0){
					*(newf_blocks+found) = i;
					found++;
			}
	}

	
	// Updating FAT table with blocks for new file
	int fat_offset = fatstart * bsize / sizeof(int);
	int i = 0;
	for (i = 0 ; i < blocks_needed ; i ++){
		*(ptr_imgu+fat_offset+*(newf_blocks+i)) = htonl(*(newf_blocks+i+1));
	}
	*(ptr_imgu+fat_offset+*(newf_blocks+i)) = htonl(1099511627775); //Adding end of block value FFFF FFFF
}

// Copies a file from the file system into the current local machine from root or given sub-directory
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
		fprintf(stderr,"[ERROR] diskget usage: diskget [disk_image] [path] [dest_file_name]\n");
		return EXIT_FAILURE;
	}
	intializeVars(&ptr_imgc);

	//Must be intialized after intializeVars
	entry_size = bsize/sizeof(dir_entry);

	if (strcmp(argv[2], "/") == 0){
		root_flag = 1;
	}else{ 
		tokenizePath(&dir_depth, argv, path, 2);
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
			getBlocks(dir_blocks);	
		}
		if (root_flag){
			got_blocks = 1;
			path_found = 1;
		}else{
			path_found = 0;
			// Loops through all the dir blocks and compares  file_name with search path for each entry, 
			// If success(dir found), it retrieves all the new block addresses and stores in b_address_ptr
			while (!path_found){
				for (int i = 0 ; i < dir_blocks && !path_found ; i++){
					for (int j = 0 ; j < entry_size ; j++){
						memcpy(&*dir, ptr_imgc+(*(b_address_ptr+i)*bsize)+(j*sizeof(*dir)) , sizeof(*dir));
						if(strcmp((char*)dir->filename, path+ctr*FILE_NAME_LEN) == 0){
							dir_blocks = htonl((unsigned int)dir->block_count);
							free(b_address_ptr);
							b_address_ptr = (unsigned int *)calloc(dir_blocks, sizeof(unsigned int));
							if (b_address_ptr == NULL){
								fprintf(stderr, "malloc(b_address_ptr) failed: insufficient memory!\n");
								return EXIT_FAILURE;
							}
							*b_address_ptr = ntohl((int)dir->starting_block);
							path_found = 1;
							getBlocks(dir_blocks);	
							break;
						}
					}
				}
				ctr++;
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
	printf("Image file [%s] opened...\n\n", argv[1]);
	if (fstat(file, &file_size) != 0){
		perror("fstat() error");
		return EXIT_FAILURE;
	}

	ptr_imgc = mmap(NULL, file_size.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
	ptr_imgu = mmap(NULL, file_size.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);

	#if defined(PART1)
		retval = diskinfo(argc, argv);
	#elif defined(PART2)
		retval = disklist(argc, argv);
	#elif defined(PART3)
		retval = diskget(argc,argv);
	#elif defined(PART4)
		retval = diskput(argc, argv, file_size.st_size);
	#else
	#	error "PART[1234] must be defined"
	#endif
		return retval;
	
	munmap(ptr_imgc, file_size.st_size);
	munmap(ptr_imgu, file_size.st_size);

	close(file);

	return retval;
}