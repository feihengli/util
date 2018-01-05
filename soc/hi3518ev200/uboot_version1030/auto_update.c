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

#define	REG(addr)	(*(volatile unsigned int *)(addr))
static const unsigned int s_mtdSize[] = {0x40000, 0x140000, 0x200000, 0x410000, 0x140000, 0x200000, 0x410000, 0x10000, 0x100000};
static const char *s_mtdName[] = {"boot", "kernel", "rootfs", "user", "kernel2", "rootfs2", "user2", "factory", "mtd"};
#define MTD_NUM  (sizeof(s_mtdSize) / sizeof(unsigned int))

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

//#undef AU_DEBUG
//#undef debug
#define AU_DEBUG
#ifdef	AU_DEBUG
#define debug(fmt, args...)	printf(fmt, ##args)
#else
#define debug(fmt, args...)
#endif	/* AU_DEBUG */

/* possible names of files on the medium. */
#define AU_FIRMWARE	"mktech_auto_update_uboot"
#define AU_KERNEL	"mktech_auto_update_kernel"
#define AU_ROOTFS	"mktech_auto_update_rootfs"
#define AU_APP	    "mktech_auto_update_app"
#define AU_KERNEL_BAK	"mktech_auto_update_kernel_bak"
#define AU_ROOTFS_BAK	"mktech_auto_update_rootfs_bak"
#define AU_APP_BAK	    "mktech_auto_update_app_bak"

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

#define MAX_UPDATE_INTF	 7
static struct medium_interface s_intf[MAX_UPDATE_INTF] = {
#ifdef CONFIG_AUTO_SD_UPDATE
	{.name = "mmc",	.init = mmc_stor_init,	.exit = mmc_stor_exit,},
#endif
#ifdef CONFIG_AUTO_USB_UPDATE
	{.name = "usb",	.init = usb_stor_init,	.exit = usb_stor_exit,},
#endif
};

/* layout of the FLASH. ST = start address, ND = end address. */
/*
#define AU_FL_FIRMWARE_ST	0x0
#define AU_FL_FIRMWARE_ND	0x7FFFF
#define AU_FL_KERNEL_ST		0x100000
#define AU_FL_KERNEL_ND		0x5FFFFF
#define AU_FL_ROOTFS_ST		0x600000
#define AU_FL_ROOTFS_ND		0xbFFFFF
*/

/*
Creating 9 MTD partitions on "hi_sfc":
0x000000000000-0x000000040000 : "boot"
0x000000040000-0x000000180000 : "kernel"
0x000000180000-0x000000380000 : "rootfs"
0x000000380000-0x000000790000 : "userfs"
0x000000790000-0x0000008d0000 : "bak_kernel"
0x0000008d0000-0x000000ad0000 : "bak_rootfs"
0x000000ad0000-0x000000ee0000 : "bak_userfs"
0x000000ee0000-0x000000ef0000 : "factory"
0x000000ef0000-0x000001000000 : "mtd"
*/

#define AU_FL_FIRMWARE_ST	0x0
#define AU_FL_FIRMWARE_ND	0x3FFFF
#define AU_FL_KERNEL_ST		0x40000
#define AU_FL_KERNEL_ND		0x17FFFF
#define AU_FL_ROOTFS_ST		0x180000
#define AU_FL_ROOTFS_ND		0x37FFFF
#define AU_FL_APP_ST		0x380000
#define AU_FL_APP_ND		0x78FFFF
#define AU_FL_KERNEL_BAK_ST		0x790000
#define AU_FL_KERNEL_BAK_ND		0x8CFFFF
#define AU_FL_ROOTFS_BAK_ST		0x8D0000
#define AU_FL_ROOTFS_BAK_ND		0xACFFFF
#define AU_FL_APP_BAK_ST		0xAD0000
#define AU_FL_APP_BAK_ND		0xEDFFFF



static int au_stor_curr_dev; /* current device */

/* index of each file in the following arrays */
#define IDX_FIRMWARE	0
#define IDX_KERNEL	1
#define IDX_ROOTFS	2
#define IDX_APP	3
#define IDX_KERNEL_BAK	4
#define IDX_ROOTFS_BAK	5
#define IDX_APP_BAK	6


/* max. number of files which could interest us */
#define AU_MAXFILES 7

/* pointers to file names */
char *aufile[AU_MAXFILES] = {
	AU_FIRMWARE,
	AU_KERNEL,
	AU_ROOTFS,
	AU_APP,
	AU_KERNEL_BAK,
	AU_ROOTFS_BAK,
	AU_APP_BAK
};

/* sizes of flash areas for each file */
long ausize[AU_MAXFILES] = {
	(AU_FL_FIRMWARE_ND + 1) - AU_FL_FIRMWARE_ST,
	(AU_FL_KERNEL_ND + 1) - AU_FL_KERNEL_ST,
	(AU_FL_ROOTFS_ND + 1) - AU_FL_ROOTFS_ST,
	(AU_FL_APP_ND + 1) - AU_FL_APP_ST,
	(AU_FL_KERNEL_BAK_ND + 1) - AU_FL_KERNEL_BAK_ST,
	(AU_FL_ROOTFS_BAK_ND + 1) - AU_FL_ROOTFS_BAK_ST,
	(AU_FL_APP_BAK_ND + 1) - AU_FL_APP_BAK_ST,
};

/* array of flash areas start and end addresses */
struct flash_layout aufl_layout[AU_MAXFILES] = {
	{ AU_FL_FIRMWARE_ST,	AU_FL_FIRMWARE_ND, },
	{ AU_FL_KERNEL_ST,	AU_FL_KERNEL_ND,   },
	{ AU_FL_ROOTFS_ST,	AU_FL_ROOTFS_ND,   },
	{ AU_FL_APP_ST,	AU_FL_APP_ND,   },
	{ AU_FL_KERNEL_BAK_ST,	AU_FL_KERNEL_BAK_ND,   },
	{ AU_FL_ROOTFS_BAK_ST,	AU_FL_ROOTFS_BAK_ND,   },
	{ AU_FL_APP_BAK_ST,	AU_FL_APP_BAK_ND,   },
};

/* where to load files into memory */
#if defined(CONFIG_HI3536) || defined(CONFIG_HI3531A)
#define LOAD_ADDR ((unsigned char *)0x42000000)
#else
#define LOAD_ADDR ((unsigned char *)0x82000000)
#endif
/* the app is the largest image */
#define MAX_LOADSZ ausize[IDX_APP]

static int au_check_cksum_valid(int idx, long nbytes)
{
	image_header_t *hdr;
	unsigned long checksum;

	hdr = (image_header_t *)LOAD_ADDR;

	if (nbytes != (sizeof(*hdr) + ntohl(hdr->ih_size))) {
		printf("Image %s bad total SIZE\n", aufile[idx]);
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

static int au_check_header_valid(int idx, long nbytes)
{
	image_header_t *hdr;
	unsigned long checksum;

	char env[20];
	char auversion[20];

	hdr = (image_header_t *)LOAD_ADDR;
	/* check the easy ones first */
#if 1
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

	if ((idx == IDX_APP) && (hdr->ih_type != IH_TYPE_FILESYSTEM)) {
		printf("Image %s wrong type\n", aufile[idx]);
		ausize[idx] = 0;
		return -1;
	}

	if ((idx == IDX_KERNEL_BAK) && (hdr->ih_type != IH_TYPE_KERNEL)) {
		printf("Image %s wrong type\n", aufile[idx]);
		return -1;
	}
	if ((idx == IDX_ROOTFS_BAK) &&
			(hdr->ih_type != IH_TYPE_RAMDISK) &&
			(hdr->ih_type != IH_TYPE_FILESYSTEM)) {
		printf("Image %s wrong type\n", aufile[idx]);
		ausize[idx] = 0;
		return -1;
	}

	if ((idx == IDX_APP_BAK) && (hdr->ih_type != IH_TYPE_FILESYSTEM)) {
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

static void schedule_notify(unsigned long offset, unsigned long len,
		unsigned long off_start)
{
	int percent_complete = -1;

	do {
		unsigned long long n = (unsigned long long)
			(offset - off_start) * 100;
		int percent;

		do_div(n, len);
		percent = (int)n;

		/* output progress message only at whole percent
		 * steps to reduce the number of messages
		 * printed on (slow) serial consoles
		 */
		if (percent != percent_complete) {
			percent_complete = percent;

			printf("\rOperation at 0x%lx -- %3d%% complete.",
					offset, percent);
		}
	} while (0);
}

static int spi_flash_erase_op(struct spi_flash *flash, unsigned long offset,
		unsigned long len)
{
	int ret;
	struct mtd_info_ex *spiflash_info = get_spiflash_info();
	unsigned long erase_start, erase_len, erase_step;

	erase_start = offset;
	erase_len   = len;
	erase_step  = spiflash_info->erasesize;

	while (len > 0) {
		if (len < erase_step)
			erase_step = len;

		ret = flash->erase(flash, (u32)offset, erase_step);
		if (ret)
			return 1;

		len -= erase_step;
		offset += erase_step;
		/* notify real time schedule */
		schedule_notify(offset, erase_len, erase_start);
	}
	return ret;
}

static int spi_flash_write_op(struct spi_flash *flash, unsigned long offset,
		unsigned long len, void *buf)
{
	int ret = 0;
	unsigned long write_start, write_len, write_step;
	char *pbuf = buf;
	struct mtd_info_ex *spiflash_info = get_spiflash_info();

	write_start = offset;
	write_len   = len;
	write_step  = spiflash_info->erasesize;

	while (len > 0) {
		if (len < write_step)
			write_step = len;

		ret = flash->write(flash, offset, write_step, pbuf);
		if (ret)
			break;

		offset += write_step;
		pbuf   += write_step;
		len    -= write_step;
		/* notify real time schedule */
		schedule_notify(offset, write_len, write_start);
	}

	return ret;
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
	image_header_t *hdr;
	unsigned long start, len;
	unsigned long write_len;
	int rc;
	void *buf;
	char *pbuf;

	hdr = (image_header_t *)LOAD_ADDR;

	start = aufl_layout[idx].start;
	len = aufl_layout[idx].end - aufl_layout[idx].start + 1;

	/*
	 * erase the address range.
	 */
	printf("flash erase...start: %#x len: %#x\n", start, len);
	rc = spi_flash_erase_op(flash, start, len);
	if (rc) {
		printf("SPI flash sector erase failed\n");
		return 1;
	}

	buf = map_physmem((unsigned long)LOAD_ADDR, len, MAP_WRBACK);
	if (!buf) {
		puts("Failed to map physical memory\n");
		return 1;
	}

	/* strip the header - except for the kernel and ramdisk */
	if (hdr->ih_type == IH_TYPE_KERNEL || hdr->ih_type == IH_TYPE_RAMDISK) {
		pbuf = buf;
		write_len = sizeof(*hdr) + ntohl(hdr->ih_size);
	} else {
		pbuf = (buf + sizeof(*hdr));
		write_len = ntohl(hdr->ih_size);
	}

	/* copy the data from RAM to FLASH */
	printf("\nflash write...start: %#x len: %ld\n", start, write_len);
	rc = spi_flash_write_op(flash, start, write_len, pbuf);
	if (rc) {
		printf("SPI flash write failed, return %d\n", rc);
		return 1;
	}

	if (!strcmp(aufile[idx], AU_KERNEL)
	    || !strcmp(aufile[idx], AU_ROOTFS)
		|| !strcmp(aufile[idx], AU_APP))
	{
		printf("write WORK flag, item name %s\n", aufile[idx]);
		start = GetAddr(idx) + GetMtdLen(idx) - 4;
		#if 0
		rc = flash->erase(flash, start, 4);
		if (rc) {
			printf("SPI flash sector erase failed\n");
			return 1;
		}
		#endif
		printf("write addr: %#x, buf_work: %s\n", start, "WORK");
		rc = flash->write(flash, start, 4, "WORK");
		if (rc) {
			printf("SPI flash write failed, return %d\n", rc);
			return 1;
		}

	}

	/* check the dcrc of the copy */
	if (crc32(0, (unsigned char const *)(buf + sizeof(*hdr)),
		ntohl(hdr->ih_size)) != ntohl(hdr->ih_dcrc)) {
		printf("Image %s Bad Data Checksum After COPY\n", aufile[idx]);
		return -1;
	}

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
	for (i = 0; i < AU_MAXFILES; i++) {
		/* just read the header */
		sz = file_fat_read(aufile[i], LOAD_ADDR,
			sizeof(image_header_t));
		debug("read %s sz %ld hdr %d\n",
			aufile[i], sz, sizeof(image_header_t));
		if (sz <= 0 || sz < sizeof(image_header_t)) {
			debug("%s not found\n", aufile[i]);
			continue;
		}

		image_found = 1;

		if (au_check_header_valid(i, sz) < 0) {
			debug("%s header not valid\n", aufile[i]);
			continue;
		}

		sz = file_fat_read(aufile[i], LOAD_ADDR, MAX_LOADSZ);
		debug("read %s sz %ld hdr %d\n",
			aufile[i], sz, sizeof(image_header_t));
		if (sz <= 0 || sz <= sizeof(image_header_t)) {
			debug("%s not found\n", aufile[i]);
			continue;
		}

		if (au_check_cksum_valid(i, sz) < 0) {
			debug("%s checksum not valid\n", aufile[i]);
			continue;
		}

		/* If u-boot had been updated, we need to
		 * save current env to flash */
		if (0 == strcmp((char *)AU_FIRMWARE, aufile[i]))
			uboot_updated = 1;

        REG(0x200f007c) = 0x00;
		REG(0x20150400) = 0x01;
		REG(0x201503fc) = 0x01;
		printf("red light!\n");
		printf("au_do_update start!\n");

		/* this is really not a good idea, but it's what the */
		/* customer wants. */
		cnt = 0;
		do {
			res = au_do_update(i, sz);
			/* let the user break out of the loop */
			if (ctrlc() || had_ctrlc()) {
				clear_ctrlc();

				break;
			}
			cnt++;
#ifdef AU_TEST_ONLY
		} while (res < 0 && cnt < 3);
		if (cnt < 3)
#else
		} while (res < 0);
#endif
	}

	if (1 == uboot_updated)
		return 1;
	if (1 == image_found)
		return 0;

	return -1;
}



static int work_flag_check(void)
{
	int rc, j;
	unsigned long start, len;
	unsigned char *buf_work = NULL;

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
		printf("check addr: %#x, buf_work: %s\n", start, buf_work);
		if(strcmp(buf_work, "WORK"))
		{
			printf("%s WORK check failed\n", s_mtdName[j]);
			break;
		}
	}
	if(j != 4)
	{
		setenv("bootargs", "mem=42M console=ttyAMA0,115200 root=/dev/mtdblock5 rw noinitrd init=/linuxrc rootfstype=squashfs  mtdparts=hi_sfc:256K(boot),1280K(kernel),2M(rootfs),4160K(userfs),1280K(bak_kernel),2M(bak_rootfs),4160K(bak_userfs),64K(factory),1088K(mtd)");
		setenv("bootcmd", "sf probe 0;sf read 0x82000000 0x790000 0x140000;bootm 0x82000000");
		setenv("mdio_intf", "rmii");
		setenv("phyaddru", "0");
		saveenv();
		printf("*****start from bak partition*****\n");
	}
	else
	{
		setenv("bootargs", "mem=42M console=ttyAMA0,115200 root=/dev/mtdblock2 rw noinitrd init=/linuxrc rootfstype=squashfs  mtdparts=hi_sfc:256K(boot),1280K(kernel),2M(rootfs),4160K(userfs),1280K(bak_kernel),2M(bak_rootfs),4160K(bak_userfs),64K(factory),1088K(mtd)");
		setenv("bootcmd", "sf probe 0;sf read 0x82000000 0x40000 0x140000;bootm 0x82000000");
		setenv("mdio_intf", "rmii");
		setenv("phyaddru", "0");
		saveenv();
		printf("*****start from major partition*****\n");
	}

	free(buf_work);
	return 0;
}

/*
 * This is called by board_init() after the hardware has been set up
 * and is usable. Only if SPI flash initialization failed will this function
 * return -1, otherwise it will return 0;
 */
int do_auto_update(void)
{
	block_dev_desc_t *stor_dev;
	int old_ctrlc;
	int j;
	int state = -1;
	int dev;
	unsigned int w = 0;

    //physical button
	REG(0x200f0094) = (unsigned int)0;
	REG(0x20150400) = (unsigned int)0;

	REG(0x200f008c) = 0x0;
	REG(0x20150400) = 0x10;
	REG(0x201503fc) = 0x10;
	printf("green light!\n");

    work_flag_check();

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

			dev = 0;
#ifdef CONFIG_ARCH_HI3519
			if (strncmp("mmc", s_intf[j].name, sizeof("mmc")) == 0)
				dev = 2;
#endif
			debug("device name %s!\n", s_intf[j].name);
			stor_dev = get_dev(s_intf[j].name, dev);
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
			get_update_env("app_st", "app_nd");

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

    if (1 == state || 0 == state)
    {
        run_command("reset", 0);
		printf("****** if you see this, that's means upgrade failed*******\n");
    }

	/*
	 * If u-boot has been updated, it's better to save environment to flash
	 */
	if (1 == state) {
		env_crc_update();
		saveenv();
	}

	return 0;
}
#endif /* CONFIG_AUTO_UPDATE */
