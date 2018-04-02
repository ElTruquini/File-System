.PHONY all:
all:
	gcc -Wall -ggdb3 -D PART1 diskutil.c -o diskinfo
	gcc -Wall -ggdb3 -D PART2 diskutil.c -o disklist
	gcc -Wall -ggdb3 -D PART3 diskutil.c -o diskget
	gcc -Wall -ggdb3 -D PART4 diskutil.c -o diskput

# .PHONY clean:
# clean:
# 	rm diskinfo disklist diskget diskput
# 	