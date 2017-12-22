/*
 * (C) Copyright 2003
 * Gary Jennejohn, DENX Software Engineering, gj@denx.de.
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
 */

#include <common.h>
#include <environment.h>
#include <command.h>
#include <malloc.h>
#include <image.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <spi_flash.h>
#include <linux/mtd/mtd.h>
#include <fat.h>

#ifdef CONFIG_AUTO_UPDATE  /* cover the whole file */

#ifdef CONFIG_AUTO_SD_UPDATE
#ifndef CONFIG_MMC
#error "should have defined CONFIG_MMC"
#endif
#include <mmc.h>
#include "mmc_init.c"
#endif

#if defined CONFIG_AUTO_USB_UPDATE
#if !defined CONFIG_USB_OHCI && !defined CONFIG_USB_XHCI
#error "should have defined CONFIG_USB_OHCI or CONFIG_USB_XHCI"
#endif
#ifndef CONFIG_USB_STORAGE
#error "should have defined CONFIG_USB_STORAGE"
#endif
#include <usb.h>
#include "usb_init.c"
#endif

#undef AU_DEBUG
#undef debug
//#ifdef	AU_DEBUG
#if	1
#define debug(fmt, args...)	printf(fmt, ##args)
#else
#define debug(fmt, args...)
#endif	/* AU_DEBUG */

/* possible names of files on the medium. */
//#define AU_FIRMWARE	"u-boot"
#define AU_FIRMWARE	"FIRMWARE_D606_F.BIN"
#define AU_KERNEL	"kernel"
#define AU_ROOTFS	"rootfs"
#define	REG(addr)	(*(volatile unsigned int *)(addr))

struct flash_layout {
	long start;
	long end;
};
static struct spi_flash *flash;

struct medium_interface {
	char name[20];
	int (*init) (void);
	void (*exit) (void);
};

#define MAX_UPDATE_INTF	3
static struct medium_interface s_intf[MAX_UPDATE_INTF] = {
#ifdef CONFIG_AUTO_SD_UPDATE
	{.name = "mmc",	.init = mmc_stor_init,	.exit = mmc_stor_exit,},
#endif
#ifdef CONFIG_AUTO_USB_UPDATE
	{.name = "usb",	.init = usb_stor_init,	.exit = usb_stor_exit,},
#endif
};

/* layout of the FLASH. ST = start address, ND = end address. */
#define AU_FL_FIRMWARE_ST	0x0
//#define AU_FL_FIRMWARE_ND	0x7FFFF
#define AU_FL_FIRMWARE_ND	0xFFFFFF
#define AU_FL_KERNEL_ST		0x100000
#define AU_FL_KERNEL_ND		0x5FFFFF
#define AU_FL_ROOTFS_ST		0x600000
#define AU_FL_ROOTFS_ND		0xbFFFFF

static int au_stor_curr_dev; /* current device */

/* index of each file in the following arrays */
#define IDX_FIRMWARE	0
#define IDX_KERNEL	1
#define IDX_ROOTFS	2

/* max. number of files which could interest us */
#define AU_MAXFILES 3

/* pointers to file names */
char *aufile[AU_MAXFILES] = {
	AU_FIRMWARE,
	AU_KERNEL,
	AU_ROOTFS
};

/* sizes of flash areas for each file */
long ausize[AU_MAXFILES] = {
	(AU_FL_FIRMWARE_ND + 1) - AU_FL_FIRMWARE_ST,
	(AU_FL_KERNEL_ND + 1) - AU_FL_KERNEL_ST,
	(AU_FL_ROOTFS_ND + 1) - AU_FL_ROOTFS_ST,
};

/* array of flash areas start and end addresses */
struct flash_layout aufl_layout[AU_MAXFILES] = {
	{ AU_FL_FIRMWARE_ST,	AU_FL_FIRMWARE_ND, },
	{ AU_FL_KERNEL_ST,	AU_FL_KERNEL_ND,   },
	{ AU_FL_ROOTFS_ST,	AU_FL_ROOTFS_ND,   },
};

/* where to load files into memory */
#if defined(CONFIG_HI3536) || defined(CONFIG_HI3531A)
#define LOAD_ADDR ((unsigned char *)0x42000000)
#else
#define LOAD_ADDR ((unsigned char *)0x82000000)
#endif
/* the app is the largest image */
//#define MAX_LOADSZ ausize[IDX_ROOTFS]
#define MAX_LOADSZ 0x1100000
#define MAX_FILE_CNT										9
#define MAX_NAME_LEN										16

#define D606_IH_MAGIC	0x30363355	/* Image Magic Number		*/
static const unsigned int s_mtdSize[] = {0x40000, 0x140000, 0x200000, 0x410000, 0x140000, 0x200000, 0x410000, 0x10000, 0x100000};
static const char *s_mtdName[] = {"boot", "kernel", "rootfs", "user", "kernel2", "rootfs2", "user2", "factory", "mtd"};
#define MTD_NUM  (sizeof(s_mtdSize) / sizeof(unsigned int))


typedef struct __PACK_ITEM_HEADER
{
	unsigned int startAddr;
	unsigned int size;
	unsigned int sum;
	unsigned int imageAddr;
	char name[MAX_NAME_LEN];
}QH360_PACK_ITEM_HEADER;

typedef struct __PACK_HEADER
{
	unsigned int magic;
	unsigned int checksum;
	unsigned int fileCnt;
	QH360_PACK_ITEM_HEADER item[MAX_FILE_CNT];
	unsigned char resver[512 - 12 - MAX_FILE_CNT * sizeof(QH360_PACK_ITEM_HEADER)];
}QH360_PACK_HEADER;

static const unsigned int d606_crc32_table[256] = {
	0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
	0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
	0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
	0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
	0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
	0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
	0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
	0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
	0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
	0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
	0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
	0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
	0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
	0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
	0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
	0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
	0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
	0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
	0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
	0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
	0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
	0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
	0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
	0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
	0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
	0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
	0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
	0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
	0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
	0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
	0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
	0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
	0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
	0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
	0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
	0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
	0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
	0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
	0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
	0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
	0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
	0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
	0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
	0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
	0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
	0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
	0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
	0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
	0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
	0x2d02ef8dL
};
static unsigned int d606_crc32(unsigned char *buf, int size)
{
	unsigned int sum = 0;
	const unsigned char *s = buf;
	
	while (--size >= 0)
	{
		sum = d606_crc32_table[(sum ^ *(s++)) & 0xff] ^ (sum >> 8);
	}
	
	return sum;
}

static int qh360_au_check_cksum_valid(int idx, long nbytes)
{

	QH360_PACK_HEADER *hdr;
	unsigned int checksum;
	long test, max_addr;
	int i, j, uboot_exist; 

	hdr = (QH360_PACK_HEADER *)LOAD_ADDR;
	checksum = hdr->checksum;
	
	for(i = 0, test = 0, max_addr = 0; i < hdr->fileCnt; i++)
	{
		test += hdr->item[i].size;
		printf("%s size is %d\n", hdr->item[i].name, hdr->item[i].size);
		if(hdr->item[i].startAddr + hdr->item[i].size > max_addr)
		{
			max_addr = hdr->item[i].startAddr + hdr->item[i].size;
		}
	}
	
	printf("header sum is %d\n", checksum);	
	printf("Image %s total size is %ld, header size is %ld, items total size is %ld, partition max size = %ld\n", 
		aufile[idx], nbytes, sizeof(QH360_PACK_HEADER), test, ausize[idx]);

	if(test > ausize[idx] || max_addr > ausize[idx])
	{
		printf("Image %s size or partion write is bigger than the flash size\n", aufile[idx]);
		return -1;
	}
#if 0
	if (nbytes != (test + sizeof(QH360_PACK_HEADER))|| nbytes > ausize[idx]) {
		printf("Image %s bad total SIZE, nbytes = %ld, test = %d, partition size = %d\n", 
				aufile[idx], nbytes, test, ausize[idx]);
		return -1;
	}
#endif

#if 0
	for(i = 0; i <= hdr->filecnt; i++)
	{
		for(j = i + 1; j <= hdr->filecnt; j++)
		{
			if(!strcmp(hdr->item[i].name, hdr->item[j].name))
			{
				printf("same item name!\n");
				return -1;
			}
		}
	}
#endif
	/* check the header CRC */

	if (checksum != d606_crc32((unsigned char *)(LOAD_ADDR + 8), nbytes - 8))
	{
		printf("Image %s bad header checksum\n", aufile[idx]);
		return -1;
	}
	
	/* check the data CRC */
	
	for(i = 0, uboot_exist = 0; i < hdr->fileCnt && i < MTD_NUM; i++)
	{
		for(j = 0; j < MTD_NUM; j++)
		{
			if(strcmp(hdr->item[i].name, s_mtdName[j]))
			{continue;}
			
			printf("%s size is %u, sum is %u, partition max size is %u\n", 
				hdr->item[i].name, hdr->item[i].size, hdr->item[i].sum, s_mtdSize[j]);
			if(!strcmp(hdr->item[i].name, s_mtdName[0]))
			{
				uboot_exist = 1;
			}
			if(hdr->item[i].size > s_mtdSize[j] && !uboot_exist)
			{
				printf("%s size too big, that means partition table must change, but %s is not in the firmware\n",
						hdr->item[i].name, s_mtdName[0]);
				return -1;
			}			
			if (hdr->item[i].sum != d606_crc32((unsigned char *)(LOAD_ADDR + hdr->item[i].startAddr), hdr->item[i].size)) 
			{
				printf("Image %s bad data checksum\n", aufile[idx]);
				return -1;
			}

		}
	}
	return 0;

}

static int au_check_cksum_valid(int idx, long nbytes)
{
	image_header_t *hdr;
	unsigned long checksum;
	long test;

	hdr = (image_header_t *)LOAD_ADDR;

	test = sizeof(*hdr) + ntohl(hdr->ih_size);
	if (nbytes != (sizeof(*hdr) + ntohl(hdr->ih_size))) {
		printf("Image %s bad total SIZE, nbytes = %ld, test = %d\n", aufile[idx], nbytes, test);
		return -1;
	}
	/* check the data CRC */
	checksum = ntohl(hdr->ih_dcrc);

	if (crc32(0, (unsigned char const *)(LOAD_ADDR + sizeof(*hdr)),
			ntohl(hdr->ih_size)) != checksum) {
		printf("Image %s bad data checksum\n", aufile[idx]);
		return -1;
	}

	return 0;
}

static int qh360_au_check_header_valid(int idx, long nbytes)
{

		QH360_PACK_HEADER *hdr;
		unsigned int checksum;	
		hdr = (QH360_PACK_HEADER *)LOAD_ADDR;
		/* check the easy ones first */
#if 1
	#define D606_CHECK_VALID_DEBUG
#else
	#undef CHECK_VALID_DEBUG
#endif
	
#ifdef D606_CHECK_VALID_DEBUG
		printf("\nmagic check %#x %#x\n", hdr->magic, D606_IH_MAGIC);
		printf("\nfileCnt is %d, header length is %d\n", hdr->fileCnt, nbytes);
#endif
		if (nbytes < sizeof(*hdr)) {
			printf("Image %s bad header SIZE\n", aufile[idx]);
			return -1;
		}
		if (hdr->magic != D606_IH_MAGIC) {
			printf("Image %s bad MAGIC\n", aufile[idx]);
			return -1;
		}
		if(hdr->fileCnt <= 0 || hdr->fileCnt > MTD_NUM)
		{
			printf("Image %s bad filecnt\n", aufile[idx]);
			return -1;
		}
	
		return 0;

}

static int au_check_header_valid(int idx, long nbytes)
{
	image_header_t *hdr;
	unsigned long checksum;

	char env[20];
	char auversion[20];

	hdr = (image_header_t *)LOAD_ADDR;
	/* check the easy ones first */
#if 0
	#define CHECK_VALID_DEBUG
#else
	#undef CHECK_VALID_DEBUG
#endif

#ifdef CHECK_VALID_DEBUG
	printf("\nmagic %#x %#x\n", ntohl(hdr->ih_magic), IH_MAGIC);
	printf("arch %#x %#x\n", hdr->ih_arch, IH_ARCH_ARM);
	printf("size %#x %#lx\n", ntohl(hdr->ih_size), nbytes);
	printf("type %#x %#x\n", hdr->ih_type, IH_TYPE_KERNEL);
#endif
	if (nbytes < sizeof(*hdr)) {
		printf("Image %s bad header SIZE\n", aufile[idx]);
		return -1;
	}
	if (ntohl(hdr->ih_magic) != IH_MAGIC || hdr->ih_arch != IH_ARCH_ARM) {
		printf("Image %s bad MAGIC or ARCH\n", aufile[idx]);
		return -1;
	}
	/* check the hdr CRC */
	checksum = ntohl(hdr->ih_hcrc);
	hdr->ih_hcrc = 0;

	if (crc32(0, (unsigned char const *)hdr, sizeof(*hdr)) != checksum) {
		printf("Image %s bad header checksum\n", aufile[idx]);
		return -1;
	}
	hdr->ih_hcrc = htonl(checksum);
	/* check the type - could do this all in one gigantic if() */
	if ((idx == IDX_FIRMWARE) && (hdr->ih_type != IH_TYPE_FIRMWARE)) {
		printf("Image %s wrong type\n", aufile[idx]);
		return -1;
	}
	if ((idx == IDX_KERNEL) && (hdr->ih_type != IH_TYPE_KERNEL)) {
		printf("Image %s wrong type\n", aufile[idx]);
		return -1;
	}
	if ((idx == IDX_ROOTFS) &&
			(hdr->ih_type != IH_TYPE_RAMDISK) &&
			(hdr->ih_type != IH_TYPE_FILESYSTEM)) {
		printf("Image %s wrong type\n", aufile[idx]);
		ausize[idx] = 0;
		return -1;
	}

	/* recycle checksum */
	checksum = ntohl(hdr->ih_size);
	/* for kernel and app the image header must also fit into flash */
	if ((idx == IDX_KERNEL) && (idx == IH_TYPE_RAMDISK))
		checksum += sizeof(*hdr);

	/* check the size does not exceed space in flash. HUSH scripts */
	/* all have ausize[] set to 0 */
	if ((ausize[idx] != 0) && (ausize[idx] < checksum)) {
		printf("Image %s is bigger than FLASH\n", aufile[idx]);
		return -1;
	}

	sprintf(env, "%lx", (unsigned long)ntohl(hdr->ih_time));
	setenv(auversion, env);

	return 0;
}
static unsigned int GetMtdLen(int partId)
{
        int max_id;
        if(partId < 0 || partId >= MTD_NUM)
               return 0;

        return s_mtdSize[partId];
}

static unsigned int GetAddr(int partId)
{
	int i, max_id;
	unsigned int addr;

	if(partId >= MTD_NUM)
	{
		return 0;
	}
	addr = 0;
	for(i = 0; i < partId; i++)
	{
		addr += s_mtdSize[i];
	}
	
	//printf("606---------max_id:%d, partId:%d, getaddr:0x%x----------\n", max_id, partId, addr);
	return addr;
}

static int au_do_update(int idx, long sz)
{
	QH360_PACK_HEADER *hdr;
	unsigned long start, len;
	unsigned long write_len;
	int rc;
	void *buf;
	char *pbuf;
	int i, j, q;

	hdr = (QH360_PACK_HEADER *)LOAD_ADDR;

	start = aufl_layout[idx].start;
	len = aufl_layout[idx].end - aufl_layout[idx].start + 1;

	/*
	 * erase the address range.
	 */
	 
	buf = map_physmem((unsigned long)(LOAD_ADDR), len, MAP_WRBACK);
	if (!buf) {
		puts("Failed to map physical memory\n");
		return 1;
	}
	

#if 0
	/* strip the header - except for the kernel and ramdisk */
	if (hdr->ih_type == IH_TYPE_KERNEL || hdr->ih_type == IH_TYPE_RAMDISK) {
		printf("-------test_01------\n");
		pbuf = buf;
		write_len = sizeof(*hdr) + ntohl(hdr->ih_size);
	} else {
		printf("-------test_02------\n");
		pbuf = (buf + sizeof(*hdr));
		write_len = ntohl(hdr->ih_size);
	}
#endif
	/* copy the data from RAM to FLASH */

for(i = 0; i < hdr->fileCnt && i < MTD_NUM; i++)
{
	for(j = 0; j < MTD_NUM; j++)
	{
		//printf("****test, i = %d, j = %d ***\n", i, j);
		if(strcmp(hdr->item[i].name, s_mtdName[j]))
		{continue;}

		start = GetAddr(j);
		len = s_mtdSize[j];
		printf("flash erase......start:0x%x, len:0x%x\n", start, len);
		rc = flash->erase(flash, start, len);
		if (rc) {
			printf("SPI flash sector erase failed\n");
			return 1;
		}

		

		start = GetAddr(j);
		pbuf = buf + hdr->item[i].startAddr;
		write_len = hdr->item[i].size;

		printf("flash write......start:0x%x, len:0x%x(%ldbytes), pbuf:%p, name:%s, \n",
				start, write_len, write_len, pbuf, hdr->item[i].name);
/////////////////////////////
		printf("start=======");
		for(q = 0; q < 16; q++)
		{
			printf(" 0x%x", (unsigned char)(pbuf[q]));
		}
		printf("=====\n");
		printf("end=======");
		for(q = 15; q >=0; q--)
		{
			printf(" 0x%x", (unsigned char)(pbuf[write_len - q - 1]));
		}
		printf("=====\n");
/////////////////////////////
		rc = flash->write(flash, start, write_len, pbuf);
		if (rc) {
			printf("SPI flash write failed, return %d\n", rc);
			return 1;
		}

		if(!strcmp(hdr->item[i].name, "kernel") ||
			!strcmp(hdr->item[i].name, "rootfs") ||
			!strcmp(hdr->item[i].name, "user"))
		{
			printf("write WORK flag, item name %s\n", hdr->item[i].name);
			start = GetAddr(j) + GetMtdLen(j) - 4;
			#if 0
			rc = flash->erase(flash, start, 4);
			if (rc) {
				printf("SPI flash sector erase failed\n");
				return 1;
			}
			#endif
			rc = flash->write(flash, start, 4, "WORK");
			if (rc) {
				printf("SPI flash write failed, return %d\n", rc);
				return 1;
			}

		}
#if 0
		/* check the dcrc of the copy */
		if (crc32(0, (unsigned char const *)(buf + sizeof(*hdr)),
			ntohl(hdr->ih_size)) != ntohl(hdr->ih_dcrc)) {
			printf("Image %s Bad Data Checksum After COPY\n", aufile[idx]);
			return -1;
		}
#endif
		break;
	}
}
	len = aufl_layout[idx].end - aufl_layout[idx].start + 1;
	unmap_physmem(buf, len);

	return 0;
}

static void get_update_env(char *img_start, char *img_end)
{
	long start = -1, end = 0;
	char *env;

	/*
	 * check whether start and end are defined in environment
	 * variables.
	 */
	env = getenv(img_start);
	if (env != NULL)
		start = simple_strtoul(env, NULL, 16);

	env = getenv(img_end);
	if (env != NULL)
		end = simple_strtoul(env, NULL, 16);

	if (start >= 0 && end && end > start) {
		ausize[IDX_FIRMWARE] = (end + 1) - start;
		aufl_layout[0].start = start;
		aufl_layout[0].end = end;
	}
}

/*
 * If none of the update file(u-boot, kernel or rootfs) was found
 * in the medium, return -1;
 * If u-boot has been updated, return 1;
 * Others, return 0;
 */
static int update_to_flash(void)
{
	int i = 0;
	long sz;
	int res, cnt;
	int uboot_updated = 0;
	int image_found = 0;


	/* just loop thru all the possible files */
	for (i = 0; i < 1 /*AU_MAXFILES*/; i++) {
		/* just read the header */
		sz = file_fat_read(aufile[i], LOAD_ADDR,
			sizeof(QH360_PACK_HEADER));
		debug("read %s sz %ld hdr %d\n",
			aufile[i], sz, sizeof(QH360_PACK_HEADER));
		if (sz <= 0 || sz < sizeof(QH360_PACK_HEADER)) {
			debug("%s not found\n", aufile[i]);
			continue;
		}

		image_found = 1;

#if 0
		if (au_check_header_valid(i, sz) < 0) {
			debug("%s header not valid\n", aufile[i]);
			continue;
		}
#endif
		if (qh360_au_check_header_valid(i, sz) < 0) {
			debug("%s header not valid\n", aufile[i]);
			continue;
		}

		sz = file_fat_read(aufile[i], LOAD_ADDR, MAX_LOADSZ);
		debug("read %s sz %ld hdr %d\n",
			aufile[i], sz, sizeof(QH360_PACK_HEADER));
		if (sz <= 0 || sz <= sizeof(QH360_PACK_HEADER)) {
			debug("%s not found\n", aufile[i]);
			continue;
		}
		if (qh360_au_check_cksum_valid(i, sz) < 0) {
			debug("%s checksum not valid\n", aufile[i]);
			continue;
		}
#if 0
		if (au_check_cksum_valid(i, sz) < 0) {
			debug("%s checksum not valid\n", aufile[i]);
			continue;
		}
#endif
		REG(0x200f007c) = 0x00;
		REG(0x20150400) = 0x01;
		REG(0x201503fc) = 0x01;
		printf("red light!\n");
		printf("au_do_update start!\n");

		/* If u-boot had been updated, we need to
		 * save current env to flash */
		if (0 == strcmp((char *)AU_FIRMWARE, aufile[i]))
			uboot_updated = 1;

		/* this is really not a good idea, but it's what the */
		/* customer wants. */
		cnt = 0;
		do {
			res = au_do_update(i, sz);
			/* let the user break out of the loop */
			//printf("**************res = %d*****ha\n",res);
			#if 1
			if (ctrlc() || had_ctrlc()) {
				//printf("*****just test_007*******\n");
				clear_ctrlc();

				break;
			}
			#endif
			cnt++;
#ifdef AU_TEST_ONLY
		} while (res < 0 && cnt < 3);
		if (cnt < 3)
		//printf("****AU_TEST_ONLY defined****\n");
#else
		//printf("20151224****AU_TEST_ONLY not defined****\n");
		} while (res < 0);
#endif
	}

	if (1 == uboot_updated)
		return 1;
	if (1 == image_found)
		return 0;

	return -1;
}

/*
 * This is called by board_init() after the hardware has been set up
 * and is usable. Only if SPI flash initialization failed will this function
 * return -1, otherwise it will return 0;
 */
static int work_flag_check(void)
{
	int rc, j;
	unsigned long start, len;
	unsigned char *buf_work = NULL;

	//printf("jump in work_flag_check\n");
	
	flash = spi_flash_probe(0, 0, 1000000, 0x3);
	buf_work = (unsigned char *)malloc(8);
	for(j = 1; j < 4; j++)
	{
	
		memset(buf_work, 0, 8);
		start = GetAddr(j) + GetMtdLen(j) - 4;
		rc = flash->read(flash, start, 4, buf_work);
		if (rc) {
			printf("SPI flash sector read failed\n");
			break;
		}
		if(strcmp(buf_work, "WORK"))
		{
			printf("%s WORK check failed\n", s_mtdName[j]);
			break;
		}
	}
	if(j != 4)
	{
		setenv("bootargs", "mem=32M console=ttyAMA0,115200 root=/dev/mtdblock5 rw noinitrd init=/linuxrc rootfstype=squashfs  mtdparts=hi_sfc:256K(boot),1280K(kernel),2M(rootfs),4160K(userfs),1280K(bak_kernel),2M(bak_rootfs),4160K(bak_userfs),64K(factory),1M(mtd)");
		setenv("bootcmd", "sf probe 0;sf read 0x82000000 0x790000 0x150000;bootm 0x82000000");
		setenv("mdio_intf", "rmii");
		setenv("phyaddru", "0");
		saveenv();
		printf("*****start from bak partition*****\n");
	}
	else
	{
		setenv("bootargs", "mem=32M console=ttyAMA0,115200 root=/dev/mtdblock2 rw noinitrd init=/linuxrc rootfstype=squashfs  mtdparts=hi_sfc:256K(boot),1280K(kernel),2M(rootfs),4160K(userfs),1280K(bak_kernel),2M(bak_rootfs),4160K(bak_userfs),64K(factory),1M(mtd)");
		setenv("bootcmd", "sf probe 0;sf read 0x82000000 0x40000 0x150000;bootm 0x82000000");
		setenv("mdio_intf", "rmii");
		setenv("phyaddru", "0");
		saveenv();
		printf("*****start from major partition*****\n");
	}
	
	free(buf_work);
	return 0;
}
int do_auto_update(void)
{
	block_dev_desc_t *stor_dev;
	int old_ctrlc;
	int j = 0;
	unsigned int w = 0;
	int state = -1;
	
	REG(0x200f0094) = (unsigned int)0;
	REG(0x20150400) = (unsigned int)0;

	REG(0x200f008c) = 0x0;
	REG(0x20150400) = 0x10;
	REG(0x201503fc) = 0x10;
	printf("green light!\n");

	work_flag_check();
#if 1
	w = (unsigned int)REG(0x20150100);

	if(w)
	{	
		printf("no hold the upgrade button down, button value is %d\n", w);
		return 0;
	}
	else
	{
		udelay(10 * 1000);
		w = (unsigned int)REG(0x20150100);
		if(w)
		{
			printf("no hold the upgrade button down, button value is %d\n", w);
			return 0;
		}
	}	
#endif
	printf("you hold the upgrade button down, button value is %d\n", w);
	au_stor_curr_dev = -1;
	for (j = 0; j < MAX_UPDATE_INTF; j++) {
		if (0 != (unsigned int)s_intf[j].name[0]) {
			au_stor_curr_dev = s_intf[j].init();
			if (-1 == au_stor_curr_dev) {
				debug("No %s storage device found!\n",
						s_intf[j].name);
				continue;
			}

			printf("device name %s!\n", s_intf[j].name);

			if(!strcmp("mmc",s_intf[j].name))
			{
			}

			stor_dev = get_dev(s_intf[j].name, 0);
			if (NULL == stor_dev) {
				debug("Unknow device type!\n");
				continue;
			}

			if (fat_register_device(stor_dev, 1) != 0) {
				debug("Unable to use %s %d:%d for fatls\n",
						s_intf[j].name,
						au_stor_curr_dev,
						1);
				continue;
			}

			if (file_fat_detectfs() != 0) {
				debug("file_fat_detectfs failed\n");
				continue;
			}

			/*
			 * Get image layout from environment.
			 * If the start address and the end address
			 * were not definedin environment virables,
			 * use the default value
			 */
			get_update_env("firmware_st", "firmware_nd");
			get_update_env("kernel_st", "kernel_nd");
			get_update_env("rootfs_st", "rootfs_nd");

			/*
			 * make sure that we see CTRL-C
			 * and save the old state
			 */
			old_ctrlc = disable_ctrlc(0);

			/*
			 * CONFIG_SF_DEFAULT_SPEED=1000000,
			 * CONFIG_SF_DEFAULT_MODE=0x3
			 */
			flash = spi_flash_probe(0, 0, 1000000, 0x3);
			if (!flash) {
				printf("Failed to initialize SPI flash\n");
				return -1;
			}



			state = update_to_flash();
			if(1 == state)
			{
				printf("update_to_flash success!\n");
				#if 0
				setenv("bootargs", "mem=32M console=ttyAMA0,115200 root=/dev/mtdblock2 rw noinitrd init=/linuxrc rootfstype=squashfs  mtdparts=hi_sfc:256K(boot),1344K(kernel),2368K(rootfs),3M(userfs),1344K(bak_kernel),2368K(bak_rootfs),3M(bak_userfs),64K(factory),1M(mtd)");
				setenv("bootcmd", "sf probe 0;sf read 0x82000000 0x40000 0x150000;bootm 0x82000000");
				//setenv("serverip", "192.168.64.23");
				//setenv("ipaddr", "192.168.64.41");
				setenv("mdio_intf", "rmii");
				setenv("phyaddru", "0");
				saveenv();
				printf("setenv & reset, by qiuz.201512251432 file:%s, line:%d\n", __FILE__, __LINE__);
				//udelay(10 * 1000 * 1000);
				#endif
				run_command("reset", 0);
				printf("****** if you see this, that's means upgrade failed*******\n");
			}
			REG(0x200f008c) = 0x0;
			REG(0x20150400) = 0x10;
			REG(0x201503fc) = 0x10;
			printf("green light & upgrade failed\n");

			/* restore the old state */
			disable_ctrlc(old_ctrlc);

			s_intf[j].exit();

			/*
			 * no update file found
			 */
			if (-1 == state)
				continue;
			/*
			 * update files have been found on current medium,
			 * so just break here
			 */
			break;
		}
	}

	/*
	 * If u-boot has been updated, it's better to save environment to flash
	 */
	if (1 == state) {
		env_crc_update();
		saveenv();
	}
	
	//do_reset(NULL, 0, 0, NULL);
	//printf("****** if you see this, that's means upgrade failed*******\n");
	return 0;
}
#endif /* CONFIG_AUTO_UPDATE */
