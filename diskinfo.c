#include <stdio.h> 
#include <stdint.h> 
#include <stdlib.h> 
#include <sys/stat.h> 
#include <unistd.h>
#include <string.h> 
#include <limits.h> 
#include <assert.h> 
#include <time.h> 

#define SYSTEM_ID_LEN 8 // File system identifier 
#define BLOCK_SIZE_LEN 2

int main(int argc, char* argv[]) {
	FILE *fp;
	unsigned char *sys_id; //File system idenfier - 8 bytes
	unsigned char *block_size;

	if (argc != 2){
		printf("Error - Usage: [file_name] [disk_image]\n");
		return EXIT_FAILURE;
	}

	fp = fopen(argv[1], "rb");
	if (fp == NULL){
		printf("Error - Failed opening the file\n");
		return EXIT_FAILURE;
	} else{
		printf("Disk image file [%s] opened...\n\n", argv[1]);
	}

	printf("Super block information:\n");
	//File system identifier
	sys_id = (unsigned char *)calloc((SYSTEM_ID_LEN+1), sizeof(char));

	fread(sys_id, sizeof(char), SYSTEM_ID_LEN, fp);
	sys_id[SYSTEM_ID_LEN+1] = '\0';
	printf("Sys_id:%s\n", sys_id);

	block_size = (unsigned char *)calloc((BLOCK_SIZE_LEN+1), sizeof(char));
	int bytes = fread(block_size, 1, BLOCK_SIZE_LEN, fp);
	block_size[BLOCK_SIZE_LEN+1] = '\0';
	printf("bytes read:%d\n");

	printf("Block size:%s\n", block_size);


	return EXIT_SUCCESS;
}