.phony all:
all: diskinfo 


diskinfo: diskinfo.c
	gcc -Wall diskinfo.c -o diskinfo
	# gcc -Wall -D PART2 parts.c -o disklist
	# gcc -Wall -D PART3 parts.c -o diskget
	# gcc -Wall -D PART4 parts.c -o diskput

clean: 
	# -rm diskinfo disklist diskget diskput
	-rm *.o