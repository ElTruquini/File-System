.PHONY all:
all:
	gcc -Wall -D PART1 diskutil.c -o diskinfo
	gcc -Wall -D PART2 diskutil.c -o disklist
	gcc -Wall -D PART3 diskutil.c -o diskget
	gcc -Wall -D PART4 diskutil.c -o diskput

.PHONE clean:
clean:
	-rm diskinfo disklist diskget diskput