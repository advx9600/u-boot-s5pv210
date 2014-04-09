/************************************************************
Copyright (C), 2010-2015, SSCR
FileName: sdformat_utils.c
Author: wangkaiyu     
Date:2011-6-3
Description: sdcard update utils
************************************************************/
#include <common.h>
#include <part.h>

#include <ext2fs.h>
#include <make_ext4fs.h>


enum _FS_TYPE{
	FS_TYPE_EXT2,
	FS_TYPE_EXT3,
	FS_TYPE_EXT4
};


int sd_format_mmcs(void)
{
	block_dev_desc_t *dev_desc=NULL;

	dev_desc = get_dev("mmc", 0);
	if (dev_desc==NULL) {
		printf("\n** Invalid boot device **\n");
		return -1;
	}
	/*for t34, 1:fat, 2,3,4:ext4*/
	if (ext2fs_format(dev_desc, 3, FS_TYPE_EXT3) != 0)
	{
		printf("ext4format failed for data\n");
		return -1;
	}
	
	if(ext4fs_format(dev_desc, 4))
	{
		printf("ext4format failed for cache\n");
		return -1;
	}

	return 0;
}
