//NAME: Jiayu Zhao
//EMAIL: jyzhao1230@g.ucla.edu
//ID: 904818173
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <errno.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

char *img_file;
int fs_fd=-1;
int mydata_fd;

struct ext2_super_block my_superblock;


void output_superblock()
{
	char id[11] = "SUPERBLOCK";
	double total_number_of_blocks;
	double total_number_of_inodes;
	double block_size;
	double inode_size;
	double blocks_per_group;
	double inodes_per_group;
	double first_non_reserved_inode;

	//The superblock is always located at byte offset 1024 from the beginning of the file in Ext2


}

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Error. Wrong number of arguments. Usage: %s ext2_image.\n", argv[0]);
		exit(1);
	}

	img_file = argv[1];
	if(img_file != NULL)
	{
		fs_fd = open(img_file, O_RDONLY);
		int errsv_open = errno;
		if(fs_fd < 0)
		{
			fprintf(stderr, "Error opening the img_file %s. Error message: %s\n", img_file, strerror(errsv_open));
			exit(2);
		}
	}


	//just for testing!!!
	mydata_fd = open(MYDATA, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if(mydata_fd < 0)
	{
		fprintf(stderr, "Error opeing my data file.\n");
		exit(2)
	}

	exit(0);
}