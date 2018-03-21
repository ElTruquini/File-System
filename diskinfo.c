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

#define SYSTEM_ID_LEN 8 // File system identifier 
#define BLOCK_SIZE_LEN 2

int main(int argc, char* argv[]) {
	FILE *fp;
	int i;
	unsigned char id_arr[SYSTEM_ID_LEN+1], bsize_arr[BLOCK_SIZE_LEN], bcount_arr[sizeof(int)], 
					fatstart_arr[sizeof(int)], fatblocks_arr[sizeof(int)], rootstart_arr[sizeof(int)],
					rootblocks_arr[sizeof(int)], numblocks_arr[sizeof(int)];
	unsigned int bcount, bsize, fatstart, fatblocks, rootstart, rootblocks, numblocks;
	// unsigned int bcount_arr;

	if (argc != 2){
		printf("Error - Usage: [file_name] [disk_image]\n");
		return EXIT_FAILURE;
	}

	fp = fopen(argv[1], "rb");
	if (fp == NULL){
		printf("Error - Failed opening the file\n");
		return EXIT_FAILURE;
	} else{
		printf("Disk image [%s] opened...\n\n", argv[1]);
	}

	printf("Super block information:\n");

	//File system identifier
	printf("SYS ID:");
	for (i =0 ; i < SYSTEM_ID_LEN ; i++){
		if (fread(&id_arr[i], sizeof(char), 1, fp) <= 0)
			fprintf(stderr, "Error reading file (fread)\n");
		printf("%c",*(id_arr+i));
	}
	id_arr[i+1]= '\0';
	printf("\n");


	//Block size
	for (i = 0 ; i < BLOCK_SIZE_LEN ; i++){
		if (fread(&bsize_arr[i], sizeof(char), 1, fp) <= 0)
			fprintf(stderr, "Error reading file (fread)\n");
	}
	printf("\n");
	bsize = ((unsigned char)bsize_arr[0] * 256 + (unsigned char)bsize_arr[1]);
	printf("Block size: %d\n", bsize);


	//Block count
	for (i = 0 ; i < sizeof(int) ; i++){
		if (fread(&bcount_arr[i], sizeof(char), 1, fp) <= 0)
			fprintf(stderr, "Error reading file (fread) - block count\n");
		// printf("%02x",*(bcount_arr+i));
	}
	bcount = (unsigned int)bcount_arr[0] | (unsigned int)bcount_arr[1]<<8 | (unsigned int)bcount_arr[2]<<16 | (unsigned int)bcount_arr[3]<<24;
	bcount = htonl(bcount);
	printf("Block count: %u\n", bcount);


	// FAT starts
	for (i = 0 ; i < sizeof(int) ; i++){
		if (fread(&fatstart_arr[i], sizeof(char), 1, fp) <= 0)
			fprintf(stderr, "Error reading file (fread) - FAT starts\n");
		// printf("%02x",*(fatstart_arr+i));
	}
	fatstart = (unsigned int)fatstart_arr[0] | (unsigned int)fatstart_arr[1]<<8 | (unsigned int)fatstart_arr[2]<<16 | (unsigned int)fatstart_arr[3]<<24;
	fatstart = htonl(fatstart);
	printf("FAT starts: %u\n", fatstart);


	//FAT blocks
	for (i = 0 ; i < sizeof(int) ; i++){
		if (fread(&fatblocks_arr[i], sizeof(char), 1, fp) <= 0)
			fprintf(stderr, "Error reading file (fread) - FAT blocks\n");
		// printf("%02x",*(fatblocks_arr+i));
	}
	fatblocks = (unsigned int)fatblocks_arr[0] | (unsigned int)fatblocks_arr[1]<<8 | (unsigned int)fatblocks_arr[2]<<16 | (unsigned int)fatblocks_arr[3]<<24;
	fatblocks = htonl(fatblocks);
	printf("FAT blocks: %u\n", fatblocks);


	//Root dir start
	for (i = 0 ; i < sizeof(int) ; i++){
		if (fread(&rootstart_arr[i], sizeof(char), 1, fp) <= 0)
			fprintf(stderr, "Error reading file (fread) - Root Start\n");
		// printf("%02x",*(rootstart_arr+i));
	}
	rootstart = (unsigned int)rootstart_arr[0] | (unsigned int)rootstart_arr[1]<<8 | (unsigned int)rootstart_arr[2]<<16 | (unsigned int)rootstart_arr[3]<<24;
	rootstart = htonl(rootstart);
	printf("Root directory start: %u\n", rootstart);


	//Root directory blocks
	for (i = 0 ; i < sizeof(int) ; i++){
		if (fread(&rootblocks_arr[i], sizeof(char), 1, fp) <= 0)
			fprintf(stderr, "Error reading file (fread) - Root Start\n");
		// printf("%02x",*(rootblocks_arr+i));
	}
	rootblocks = (unsigned int)rootblocks_arr[0] | (unsigned int)rootblocks_arr[1]<<8 | (unsigned int)rootblocks_arr[2]<<16 | (unsigned int)rootblocks_arr[3]<<24;
	rootblocks = htonl(rootblocks);
	printf("Root directory blocks: %u\n", rootblocks);


	//Num blocks in root dir
	for (i = 0 ; i < sizeof(int) ; i++){
		if (fread(&numblocks_arr[i], sizeof(char), 1, fp) <= 0)
			fprintf(stderr, "Error reading file (fread) - Root Start\n");
		printf("%02x",*(numblocks_arr+i));
	}
	numblocks = (unsigned int)numblocks_arr[0] | (unsigned int)numblocks_arr[1]<<8 | (unsigned int)numblocks_arr[2]<<16 | (unsigned int)numblocks_arr[3]<<24;
	numblocks = htonl(numblocks);
	printf("NUM BLOCKS directory blocks: %u\n", numblocks);




	/* Byte pointer traversal */
	// unsigned char *ptr3 = (unsigned char *) &bcount; 
	// printf("\ntraversing AFTER hton bcount\n");
	// for (i=0; i < sizeof(int); i++)
 	// 		printf("%x ", ptr3[i]);
 	// printf("\n");







	fclose(fp);
	return EXIT_SUCCESS;
}