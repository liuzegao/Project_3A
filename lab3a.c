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
int groupNumber;

struct ext2_super_block my_superblock;
struct ext2_group_desc* groupSum;

const int SUPEROFF = 1024;

int block_size;//!!!

int block_num_to_offset(int block_number)
{
	return (SUPEROFF+(block_number-1)*block_size);
}

void output_superblock()
{
	//double or unsigned int???
	char id[11] = "SUPERBLOCK";

	//The superblock is always located at byte offset 1024 from the beginning of the file in Ext2
	int ret = pread(fs_fd, &my_superblock, sizeof(struct ext2_super_block), SUPEROFF);
	if(ret == -1)
	{
		fprintf(stderr, "Error. pread failed.\n");
		exit(2);
	}
	int total_number_of_blocks = my_superblock.s_blocks_count;
	int total_number_of_inodes = my_superblock.s_inodes_count;
	block_size = EXT2_MIN_BLOCK_SIZE << my_superblock.s_log_block_size;
	int inode_size = my_superblock.s_inode_size;
	int blocks_per_group = my_superblock.s_blocks_per_group;
	int inodes_per_group = my_superblock.s_inodes_per_group;
	int first_non_reserved_inode = my_superblock.s_first_ino;

	//printf("%s,%d,%d,%d,%d,%d,%d,%d\n", id, total_number_of_blocks, total_number_of_inodes, block_size, inode_size, blocks_per_group, inodes_per_group, first_non_reserved_inode);
	dprintf(mydata_fd, "%s,%d,%d,%d,%d,%d,%d,%d\n", id, total_number_of_blocks, total_number_of_inodes, block_size, inode_size, blocks_per_group, inodes_per_group, first_non_reserved_inode);
}

void output_group()
{
    unsigned int remainedInodes = my_superblock.s_inodes_count;
    unsigned int remainedBlocks = my_superblock.s_blocks_count;
    groupNumber = my_superblock.s_blocks_count/ my_superblock.s_blocks_per_group+1;
    groupSum = malloc(groupNumber*sizeof(struct ext2_group_desc));
    int STARTOFFSET = SUPEROFF + block_size;
    
    for (int i = 0; i < groupNumber; i++){
        
        dprintf(mydata_fd, "GROUP,%d,", i);
        //printf("GROUP,%d,", i);
        
        // Number of blocks in group
        if(remainedBlocks > my_superblock.s_blocks_per_group) {
            dprintf(mydata_fd, "%d,", my_superblock.s_blocks_per_group);
            //printf("%d,", my_superblock.s_blocks_per_group);
            remainedBlocks-= my_superblock.s_blocks_per_group;
        }
        else {
            dprintf(mydata_fd, "%d,", remainedBlocks);
            //printf("%d,", remainedBlocks);
        }
        
        // Number of inodes in group
        if(remainedInodes > my_superblock.s_inodes_per_group) {
            dprintf(mydata_fd, "%d,", my_superblock.s_inodes_per_group);
            //printf("%d,", my_superblock.s_inodes_per_group);
            remainedBlocks-= my_superblock.s_inodes_per_group;
        }
        else {
            dprintf(mydata_fd, "%d,", remainedInodes);
            //printf("%d,", remainedInodes);
        }
        
        pread(fs_fd, &groupSum[i], sizeof(struct ext2_group_desc), STARTOFFSET + i*sizeof(struct ext2_group_desc));
        //int bytes_read = pread(fs_fd, &group_data[i].free_block_count, 2, SUPERBLOCK_OFFSET + SUPERBLOCK_SIZE + (i * GROUP_DESC_SIZE) + 12);
        // Number of free blocks
        dprintf(mydata_fd, "%d,", groupSum[i].bg_free_blocks_count);
        //printf("%d,", groupSum[i].bg_free_blocks_count);
        
        // Number of free inodes
        dprintf(mydata_fd, "%d,", groupSum[i].bg_free_inodes_count);
        //printf("%d,", groupSum[i].bg_free_inodes_count);
        
        // Block number of free block bitmap for group
        dprintf(mydata_fd, "%d,", groupSum[i].bg_block_bitmap);
        //printf("%d,", groupSum[i].bg_block_bitmap);
        
        // Block number of free inode bitmap for group
        dprintf(mydata_fd, "%d,", groupSum[i].bg_inode_bitmap);
        //printf("%d,", groupSum[i].bg_inode_bitmap);
        
        // Block number of first block of inodes in group
        dprintf(mydata_fd, "%d\n", groupSum[i].bg_inode_table);
        //printf("%d\n", groupSum[i].bg_inode_table);

    } 
}

void output_free_block_entries()
{
	int number_of_blocks_in_group;
	char my_bitmap[block_size];
	for (int i = 0; i < groupNumber; i++)
	{
		int block_num_of_block_bitmap = groupSum[i].bg_block_bitmap;
		pread(fs_fd, my_bitmap, block_size, block_num_to_offset(block_num_of_block_bitmap));
		if(i == groupNumber-1)
		{
			number_of_blocks_in_group = my_superblock.s_blocks_count % my_superblock.s_blocks_per_group;
			if(number_of_blocks_in_group == 0)
			{
				number_of_blocks_in_group= my_superblock.s_blocks_per_group;
			}
		} 
		else
		{
			number_of_blocks_in_group = my_superblock.s_blocks_per_group;
		}

		int number_of_bytes_in_bitmap = number_of_blocks_in_group/8;
		int number_of_remaining_bits = number_of_blocks_in_group % 8;

		//interpret the bytes that can be extracted by the whole char
		int j;
		for(j = 0; j < number_of_bytes_in_bitmap; j++)
		{
			char my_byte = my_bitmap[j];
			for(int k = 0; k < 8 ; k++)
			{
				if((my_byte & (1 << k)) == 0) //0 means free, get the least significant bit for the least number of block
				{
					int number_of_the_free_block = 1 + i*my_superblock.s_blocks_per_group + j*8 + k;
					dprintf(mydata_fd, "BFREE,%d\n", number_of_the_free_block);
					//printf("BFREE,%d\n", number_of_the_free_block);
				}
			}
		}

		//interpret the remaining bits
		char my_remaining_byte = my_bitmap[j];
		for(int p = 0; p < number_of_remaining_bits; p++)
		{
			if((my_remaining_byte & (1 << p)) == 0)
			{
				int number_of_the_free_block = 1 + i*my_superblock.s_blocks_per_group + j*8 + p;
				dprintf(mydata_fd, "BFREE,%d\n", number_of_the_free_block);
				//printf("BFREE,%d\n", number_of_the_free_block);
			}
		}
	}
}

void output_free_inode_entries()
{
	int number_of_free_inodes;
	char my_inode_bitmap[block_size];
	for(int i = 0; i < groupNumber; i++)
	{
		int block_num_of_inode_bitmap = groupSum[i].bg_inode_bitmap;
		pread(fs_fd, my_inode_bitmap, block_size, block_num_to_offset(block_num_of_inode_bitmap));
		if(i == groupNumber-1)
		{
			number_of_free_inodes = my_superblock.s_inodes_count % my_superblock.s_inodes_per_group;
			if(number_of_free_inodes == 0)
			{
				number_of_free_inodes = my_superblock.s_inodes_per_group;
			}
		} 
		else
		{
			number_of_free_inodes = my_superblock.s_inodes_per_group;
		}

		int number_of_bytes_in_inode_bitmap = number_of_free_inodes/8;
		int number_of_remaining_bits_inode = number_of_free_inodes % 8;

		int j;
		for(j = 0; j < number_of_bytes_in_inode_bitmap; j++)
		{
			char my_inode_byte = my_inode_bitmap[j];
			for(int k = 0; k < 8; k++)
			{
				if((my_inode_byte & (1 << k)) == 0) //free inode 
				{
					int block_number_of_the_free_inode = 1 + i*my_superblock.s_inodes_per_group + j*8 + k;
					dprintf(mydata_fd, "IFREE,%d\n", block_number_of_the_free_inode);
					//printf("IFREE,%d", block_number_of_the_free_inode);
				}
			}
		}

		char my_remaining_inode_byte = my_inode_bitmap[j];
		for(int p = 0; p < number_of_remaining_bits_inode; p++)
		{
			if((my_remaining_inode_byte & (1 << p)) == 0)
			{
				int block_number_of_the_free_inode = 1 + i*my_superblock.s_inodes_per_group + j*8 + p;
				dprintf(mydata_fd, "IFREE,%d\n", block_number_of_the_free_inode);
				//printf("IFREE,%d", block_number_of_the_free_inode);
			}
		}
	}
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
    output_group();
    output_free_block_entries();
    output_free_inode_entries();
    
	exit(0);
}
