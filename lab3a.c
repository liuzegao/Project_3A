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

char *img_file;
int fs_fd=-1;

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

	exit(0);
}