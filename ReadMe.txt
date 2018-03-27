PART 1 - diskinfo:
	Usage: [file_name] [disk_image]
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


PART 2 - disklist:

	Usage: [file_name] [disk_image] [path]
	Listing Root folder contents uses char "/" for specifying root directory.
	The program allows for depth search of maximum height of 10 ie: /level1/level2/.../level10
	Command example: 
			./disklist [input_file_name] / 
			./disklist [t.img] /sub1
			./disklist [t.img] /sub1/sub2

PART 3 - diskget:

	Usage: [file_name] [disk_image] [path] [dest_file_name]
	Copies file from specified path, if path not found, error is raised.
	The program allows for depth search of maximum height of 10 ie: /level1/level2/.../level10
	Command example:
			./diskget [input_file_name] /foo.txt /a.txt
			./diskget [t.img] /fff.txt /sub1/out.txt

PART 4:
	

	Usage: [file_name] [disk_image] [source_file] [dest_path]


