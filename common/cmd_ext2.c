/*
 * (C) Copyright 2010
 * Samsung Electronics Co. Ltd
 * added ext2/ext3 format
 *
 * (C) Copyright 2004
 * esd gmbh <www.esd-electronics.com>
 * Reinhard Arlt <reinhard.arlt@esd-electronics.com>
 *
 * made from cmd_reiserfs by
 *
 * (C) Copyright 2003 - 2004
 * Sysgo Real-Time Solutions, AG <www.elinos.com>
 * Pavel Bartusek <pba@sysgo.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

/*
 * Ext2fs support
 */
#include <common.h>
#include <part.h>
#include <config.h>
#include <command.h>
#include <image.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <ext2fs.h>
#include <make_ext4fs.h>
#if defined(CONFIG_CMD_USB) && defined(CONFIG_USB_STORAGE)
#include <usb.h>
#endif

#ifndef CONFIG_DOS_PARTITION
#error DOS partition support must be selected
#endif

enum _FS_TYPE{
	FS_TYPE_EXT2,
	FS_TYPE_EXT3,
	FS_TYPE_EXT4
};

/* #define	EXT2_DEBUG */

#ifdef	EXT2_DEBUG
#define	PRINTF(fmt,args...)	printf (fmt ,##args)
#else
#define PRINTF(fmt,args...)
#endif

int do_ext2ls (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *filename = "/";
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;
	int part_length;

	if (argc < 3) {
		printf ("Usage:\n%s\n", cmdtp->usage);
		return(1);
	}
	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);

	if (dev_desc == NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}

	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return(1);
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	if (argc == 4) {
	    filename = argv[3];
	}

	PRINTF("Using device %s %d:%d, directory: %s\n", argv[1], dev, part, filename);

	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return(1);
	}

	if (!ext2fs_mount(part_length)) {
		printf ("** Bad ext2 partition or disk - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return(1);
	}

	if (ext2fs_ls (filename)) {
		printf ("** Error ext2fs_ls() **\n");
		ext2fs_close();
		return(1);
	};

	ext2fs_close();

	return(0);
}

U_BOOT_CMD(
	ext2ls,	4,	1,	do_ext2ls,
	"ext2ls  - list files in a directory (default /)\n",
	"<interface> <dev[:part]> [directory]\n"
	"    - list files from 'dev' on 'interface' in a 'directory'\n"
);

/******************************************************************************
 * Ext2fs boot command intepreter. Derived from diskboot
 */
int do_ext2load (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	char *filename = NULL;
	char *ep;
	int dev, part = 1;
	ulong addr = 0, part_length, filelen;
	disk_partition_t info;
	block_dev_desc_t *dev_desc = NULL;
	char buf [12];
	unsigned long count;
	char *addr_str;

	switch (argc) {
	case 3:
		addr_str = getenv("loadaddr");
		if (addr_str != NULL) {
			addr = simple_strtoul (addr_str, NULL, 16);
		} else {
			addr = CFG_LOAD_ADDR;
		}
		filename = getenv ("bootfile");
		count = 0;
		break;
	case 4:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = getenv ("bootfile");
		count = 0;
		break;
	case 5:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = argv[4];
		count = 0;
		break;
	case 6:
		addr = simple_strtoul (argv[3], NULL, 16);
		filename = argv[4];
		count = simple_strtoul (argv[5], NULL, 16);
		break;

	default:
		printf ("Usage:\n%s\n", cmdtp->usage);
		return(1);
	}

	if (!filename) {
		puts ("\n** No boot file defined **\n");
		return(1);
	}

	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc = get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		printf ("\n** Block device %s %d not supported\n", argv[1], dev);
		return(1);
	}
	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return(1);
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
	}

	PRINTF("Using device %s%d, partition %d\n", argv[1], dev, part);

	if (part != 0) {
		if (get_partition_info (dev_desc, part, &info)) {
			printf ("** Bad partition %d **\n", part);
			return(1);
		}

		if (strncmp((char *)info.type, BOOT_PART_TYPE, sizeof(info.type)) != 0) {
			printf ("\n** Invalid partition type \"%.32s\""
				" (expect \"" BOOT_PART_TYPE "\")\n",
				info.type);
			return(1);
		}
		PRINTF ("\nLoading from block device %s device %d, partition %d: "
			"Name: %.32s  Type: %.32s  File:%s\n",
			argv[1], dev, part, info.name, info.type, filename);
	} else {
		PRINTF ("\nLoading from block device %s device %d, File:%s\n",
			argv[1], dev, filename);
	}


	if ((part_length = ext2fs_set_blk_dev(dev_desc, part)) == 0) {
		printf ("** Bad partition - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return(1);
	}

	if (!ext2fs_mount(part_length)) {
		printf ("** Bad ext2 partition or disk - %s %d:%d **\n",  argv[1], dev, part);
		ext2fs_close();
		return(1);
	}

	filelen = ext2fs_open(filename);
	if (filelen < 0) {
		printf("** File not found %s\n", filename);
		ext2fs_close();
		return(1);
	}
	if ((count < filelen) && (count != 0)) {
	    filelen = count;
	}

	if (ext2fs_read((char *)addr, filelen) != filelen) {
		printf("\n** Unable to read \"%s\" from %s %d:%d **\n", filename, argv[1], dev, part);
		ext2fs_close();
		return(1);
	}

	ext2fs_close();

	/* Loading ok, update default load address */
	load_addr = addr;

	printf ("\n%ld bytes read\n", filelen);
	sprintf(buf, "%lX", filelen);
	setenv("filesize", buf);

	return(filelen);
}

U_BOOT_CMD(
	ext2load,	6,	0,	do_ext2load,
	"ext2load- load binary file from a Ext2 filesystem\n",
	"<interface> <dev[:part]> [addr] [filename] [bytes]\n"
	"    - load binary file 'filename' from 'dev' on 'interface'\n"
	"      to address 'addr' from ext2 filesystem\n"
);

int ext_format (int argc, char *argv[], char filesystem_type)
{
	int dev=0;
	int part=1;
	char *ep;
	block_dev_desc_t *dev_desc=NULL;

	if (argc < 2) {
		printf ("usage: ext2formt <interface> <dev[:part]>\n");
		return (0);
	}
	dev = (int)simple_strtoul (argv[2], &ep, 16);
	dev_desc=get_dev(argv[1],dev);
	if (dev_desc==NULL) {
		puts ("\n** Invalid boot device **\n");
		return 1;
	}
	if (*ep) {
		if (*ep != ':') {
			puts ("\n** Invalid boot device, use `dev[:part]' **\n");
			return 1;
		}
		part = (int)simple_strtoul(++ep, NULL, 16);
		if (part > 4 || part < 1) {
			puts ("** Partition Number shuld be 1 ~ 4 **\n");
			return 1;
		}
	}
	printf("Start format MMC%d partition%d ....\n", dev, part);
	switch(filesystem_type){
	case FS_TYPE_EXT3:
	case FS_TYPE_EXT2:
		if (ext2fs_format(dev_desc, part, filesystem_type) != 0) {
			printf("Format failure!!!\n");
		}
		break;

	case FS_TYPE_EXT4:
		if (ext4fs_format(dev_desc, part) != 0) {
			printf("Format failure!!!\n");
		}
		break;

	default:
		printf("FileSystem Type Value is not invalidate=%d \n", filesystem_type);
		break;
	}

	return 0;
}

int do_ext2_format (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return ext_format(argc, argv, FS_TYPE_EXT2);
}

U_BOOT_CMD(
	ext2format,	3,	0,	do_ext2_format,
	"ext2format - disk format by ext2\n",
	"<interface(only support mmc)> <dev:partition num>\n"
	"    - format by ext2 on 'interface'\n"
);

int do_ext3_format (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return ext_format(argc, argv, FS_TYPE_EXT3);
}

U_BOOT_CMD(
	ext3format,	3,	0,	do_ext3_format,
	"ext3format - disk format by ext3\n",
	"<interface(only support mmc)> <dev:partition num>\n"
	"    - format by ext3 on 'interface'\n"
);

int do_ext4_format (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	printf("do_ext4_format\n");
	return ext_format(argc, argv, FS_TYPE_EXT4);
}

U_BOOT_CMD(
	ext4format,	3,	0,	do_ext4_format,
	"ext4format - disk format by ext4\n",
	"<interface(only support mmc)> <dev:partition num>\n"
	"    - format by ext4 on 'interface'\n"
);
