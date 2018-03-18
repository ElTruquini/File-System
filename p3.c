#include <stdio.h> 
#include <stdint.h> 
#include <stdlib.h> 
#include <sys/stat.h> 
#include <string.h> 
#include <limits.h> 
#include <assert.h> 
#include <time.h> 

int main(int argc, char* argv[]) {

	#if defined(PART1) 
		diskinfo(argc, argv); 
	#elif defined(PART2) 
		disklist(argc, argv); 
	#elif defined(PART3) 
		diskget(argc, argv); 
	#elif defined(PART4) 
		diskput(argc,argv); 
	#else 
		#error "PART[1234] must be defined" 
	#endif
	
	return 0; 
}