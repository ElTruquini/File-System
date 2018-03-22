.phony all:
all: diskinfo disklist


diskinfo: diskinfo.c diskinfo.h
	# gcc -Wall diskinfo.c -o diskinfo

disklist: diskinfo.c diskinfo.h disklist.c
	gcc -Wall disklist.c diskinfo.c -o disklist 


clean: 
	# -rm diskinfo disklist diskget diskput
	-rm *.o