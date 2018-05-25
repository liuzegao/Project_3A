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
#include "ext2_fs.h"

char *img_file;
int fs_fd=-1;
int mydata_fd;

struct ext2_super_block my_superblock;


void output_superblock()
{
	//double or unsigned int???
	char id[11] = "SUPERBLOCK";

	//The superblock is always located at byte offset 1024 from the beginning of the file in Ext2
	int ret = pread(fs_fd, &my_superblock, sizeof(struct ext2_super_block), 1024);
	if(ret == -1)
	{
		fprintf(stderr, "Error. pread failed.\n");
		exit(2);
	}
	int total_number_of_blocks = my_superblock.s_blocks_count;
	int total_number_of_inodes = my_superblock.s_inodes_count;
	int block_size = EXT2_MIN_BLOCK_SIZE << my_superblock.s_log_block_size;
	int inode_size = my_superblock.s_inode_size;
	int blocks_per_group = my_superblock.s_blocks_per_group;
	int inodes_per_group = my_superblock.s_inodes_per_group;
	int first_non_reserved_inode = my_superblock.s_first_ino;

	//printf("%s,%d,%d,%d,%d,%d,%d,%d\n", id, total_number_of_blocks, total_number_of_inodes, block_size, inode_size, blocks_per_group, inodes_per_group, first_non_reserved_inode);
	dprintf(mydata_fd, "%s,%d,%d,%d,%d,%d,%d,%d\n", id, total_number_of_blocks, total_number_of_inodes, block_size, inode_size, blocks_per_group, inodes_per_group, first_non_reserved_inode);
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
	mydata_fd = open("MYDATA", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
	if(mydata_fd < 0)
	{
		fprintf(stderr, "Error opening my data file.\n");
		exit(2);
	}

	output_superblock();

	exit(0);
}