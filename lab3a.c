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
#include <sys/time.h>
#include <time.h>

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
    int STARTOFFSET = SUPEROFF + BLOCKSIZE;
    
    for (int i = 0; i < groupNumber; i++){
        dprintf(mydata_fd, "GROUP,%d,", i);
        //printf("GROUP,%d,", i);
        if(remainedBlocks > my_superblock.s_blocks_per_group) {
            dprintf(mydata_fd, "%d,", my_superblock.s_blocks_per_group);
            //printf("%d,", my_superblock.s_blocks_per_group);
            remainedBlocks-= my_superblock.s_blocks_per_group;
        }
        else {
            dprintf(mydata_fd, "%d,", remainedBlocks);
            //printf("%d,", remainedBlocks);
        }
        if(remainedInodes > my_superblock.s_inodes_per_group) {
            dprintf(mydata_fd, "%d,", my_superblock.s_inodes_per_group);
            //printf("%d,", my_superblock.s_inodes_per_group);
            remainedBlocks-= my_superblock.s_inodes_per_group;
        }
        else {
            dprintf(mydata_fd, "%d,", remainedInodes);
            //printf("%d,", remainedInodes);
        }
        if(pread(fs_fd, &groupSum[i], sizeof(struct ext2_group_desc), STARTOFFSET + i*sizeof(struct ext2_group_desc))==-1){
            fprintf(stderr, "Failed to pread group desc.\n");
            exit(2);
        }
        /*
         GROUP
         group number (decimal, starting from zero)
         total number of blocks in this group (decimal)
         total number of i-nodes in this group (decimal)
         number of free blocks (decimal)
         number of free i-nodes (decimal)
         block number of free block bitmap for this group (decimal)
         block number of free i-node bitmap for this group (decimal)
         block number of first block of i-nodes in this group (decimal)
         */
        dprintf(mydata_fd, "%d,", groupSum[i].bg_free_blocks_count);
        //printf("%d,", groupSum[i].bg_free_blocks_count);
        dprintf(mydata_fd, "%d,", groupSum[i].bg_free_inodes_count);
        //printf("%d,", groupSum[i].bg_free_inodes_count);
        dprintf(mydata_fd, "%d,", groupSum[i].bg_block_bitmap);
        //printf("%d,", groupSum[i].bg_block_bitmap);
        dprintf(mydata_fd, "%d,", groupSum[i].bg_inode_bitmap);
        //printf("%d,", groupSum[i].bg_inode_bitmap);
        dprintf(mydata_fd, "%d\n", groupSum[i].bg_inode_table);
        //printf("%d\n", groupSum[i].bg_inode_table);
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

void ConverTime(int timestamp, char temp[]) {
    time_t timetemp= timestamp;
    struct tm ts = *gmtime(&timetemp);
    strftime(temp, 80, "%m/%d/%y %H:%M:%S", &ts);
}

void indirect(int level, int maxlevel, int logicalOffset, int inodeNumber,int parentNode){
    if(level > maxlevel) return;
    
    int nRef = block_size / sizeof(__u32);
    int address[nRef];
    
    if (pread(fs_fd, address, block_size, inodeNumber * block_size) < 0) {
        fprintf(stderr, "Failed to pread indirect blocks\n");
        exit(2);
    }
    /*
     INDIRECT
     I-node number of the owning file (decimal)
     (decimal) level of indirection for the block being scanned ... 1 for single indirect, 2 for double indirect, 3 for triple
     logical block offset (decimal) represented by the referenced block. If the referenced block is a data block, this is the logical block offset of that block within the file. If the referenced block is a single- or double-indirect block, this is the same as the logical offset of the first data block to which it refers.
     block number of the (1, 2, 3) indirect block being scanned (decimal) . . . not the highest level block (in the recursive scan), but the lower level block that contains the block reference reported by this entry.
     block number of the referenced block (decimal)
     */
    for (int i = 0; i < nRef; i++){
        if (address[i] == 0)
            continue;
        dprintf(mydata_fd, "INDIRECT,%u,%u,%u,%u,%u\n",
                parentNode,
                level,
                logicalOffset+i,
                inodeNumber,
                address[i]);
        indirect(level+1, maxlevel, logicalOffset, address[i],parentNode);
    }
}

void output_inode(){
    for (int i = 0; i < groupNumber; i++) {
        int inodeNumber = my_superblock.s_inodes_count;
        int inodeTable = groupSum[i].bg_inode_table;
        //block_size
        for (int j = 0; j < inodeNumber; j++){
            struct ext2_inode* inodeIterator = malloc(sizeof(struct ext2_inode));
            //block_num_to_offset(inodeTable)
            if (pread(fs_fd, inodeIterator, sizeof(struct ext2_inode),block_num_to_offset(inodeTable)+j*sizeof(struct ext2_inode)) == -1) {
                fprintf(stderr, "Failed to pread inode iterator\n");
                exit(2);
            }
            if (inodeIterator ->i_mode == 0 || inodeIterator ->i_links_count == 0)
                continue;
            
            char Type = '?';
            if (S_ISLNK(inodeIterator->i_mode)) Type = 's';
            else if (S_ISREG(inodeIterator->i_mode)) Type = 'f';
            else if (S_ISDIR(inodeIterator->i_mode)) Type = 'd';
            
            /*
             INODE
             inode number (decimal)
             file type ('f' for file, 'd' for directory, 's' for symbolic link, '?" for anything else)
             mode (low order 12-bits, octal ... suggested format "%o")
             owner (decimal)
             group (decimal)
             link count (decimal)
             time of last I-node change (mm/dd/yy hh:mm:ss, GMT)
             modification time (mm/dd/yy hh:mm:ss, GMT)
             time of last access (mm/dd/yy hh:mm:ss, GMT)
             file size (decimal)
             number of (512 byte) blocks of disk space (decimal) taken up by this file
             */
            dprintf(mydata_fd, "INODE,%d,", j + 1);
            dprintf(mydata_fd, "%c,", Type);
            dprintf(mydata_fd, "%o,",inodeIterator->i_mode & 0xFFF);
            dprintf(mydata_fd, "%d,",inodeIterator->i_uid);
            dprintf(mydata_fd, "%d,",inodeIterator->i_gid);
            dprintf(mydata_fd, "%d,",inodeIterator->i_links_count);
            char changeTime[25], modifyTime[25], accessTime[25];
            ConverTime(inodeIterator->i_ctime, changeTime);
            ConverTime(inodeIterator->i_mtime, modifyTime);
            ConverTime(inodeIterator->i_atime, accessTime);
            dprintf(mydata_fd, "%s,", changeTime);
            dprintf(mydata_fd, "%s,", modifyTime);
            dprintf(mydata_fd, "%s,", accessTime);
            dprintf(mydata_fd, "%d,", inodeIterator->i_size);
            dprintf(mydata_fd, "%d", inodeIterator->i_blocks);
            if(Type == 'f' || Type == 'd'){
                for (int y = 0; y < EXT2_N_BLOCKS; y++)
                    dprintf(mydata_fd, ",%d", inodeIterator->i_block[y]);
                dprintf(mydata_fd, "\n");
            }else if(Type == 's'){
                for (int y = 0; y < 1; y++){
                    dprintf(mydata_fd, ",%d", inodeIterator->i_block[y]);
                }
                dprintf(mydata_fd, "\n");
            }
            
            if(Type == 'd'){
                for (int k = 0; k < 12; k++) {
                    if (inodeIterator->i_block[k] == 0) break; // NULL - no more
                    int offset = 0;
                    while(offset < 1024){
                        struct ext2_dir_entry directory;
                        if(pread(fs_fd, &directory, sizeof(struct ext2_dir_entry), block_num_to_offset(inodeIterator->i_block[k]) + offset)<0){
                            fprintf(stderr, "failed pread directory\n");
                            exit(2);
                        }
                        if (directory.inode == 0 || directory.rec_len == 0){
                            offset += directory.rec_len;
                            continue;
                        }
                        dprintf(mydata_fd, "DIRECT,%d,%d,%d,%d,%d,'%s'\n",
                                j+1,
                                offset,
                                directory.inode,
                                directory.rec_len,
                                directory.name_len,
                                directory.name);
                        offset += directory.rec_len;
                    }
                }
            }
            
            if (inodeIterator->i_block[12] > 0)
                indirect(1, 1, 12,inodeIterator->i_block[12], j+1);
            if (inodeIterator->i_block[13] > 0)
                indirect(1, 2, 268,inodeIterator->i_block[13], j+1);
            if (inodeIterator->i_block[14] > 0)
                indirect(1, 3, 65804,inodeIterator->i_block[14], j+1);
            free(inodeIterator);
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
    output_inode();
	exit(0);
}
