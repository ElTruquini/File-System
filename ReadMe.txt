
To compile:

	Execute command:
		./make





PART 1 - diskinfo:
	Usage: diskinfo [disk_image]
	Gets superblock information and prints on screen
		-Block size
		-Block count
		-FAT blocks start
		-Number of FAT blocks
		-Root directory start
		-Number of root directory blocks
		-Number of free blocks
		-Number of reserverd blocks
		-Number of data allocated blocks
	Command example: 
			./diskinfo test.img

PART 2 - disklist:

	Usage: disklist [disk_image] [path]
	Listing Root folder contents uses char "/" for specifying root directory.
	Subdirectory listing functionality implemented.
	The program allows for depth search of maximum height of 10 ie: /level1/level2/.../level10
	Command example: 
			./disklist [input_file_name] [path] 
			./disklist t.img /
			./disklist t.img /sub1
			./disklist t.img /sub1/sub2

PART 3 - diskget:

	Usage: diskget [disk_image] [path] [dest_file_name]
	Copies file from specified path, to current directory. If source path not found, error is raised.
	Subdirectory listing functionality implemented.
	The program allows for depth search of maximum height of 10 ie: /level1/level2/.../level10
	Command example:
			./diskget [input_file_name] [source_path] [destination_path] 
			./diskget test.img /foo.txt a.txt
			./diskget test.img /fff.txt out.txt

PART 4:
	
	
	Usage: diskget [input_file_name] [source_file] [destination_path]


		Command example:
			./diskget test.img a.txt /
