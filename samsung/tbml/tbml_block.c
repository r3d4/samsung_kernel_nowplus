/*
 *---------------------------------------------------------------------------*
 *                                                                           *
 *          COPYRIGHT 2003-2009 SAMSUNG ELECTRONICS CO., LTD.                *
 *                          ALL RIGHTS RESERVED                              *
 *                                                                           *
 *   Permission is hereby granted to licensees of Samsung Electronics        *
 *   Co., Ltd. products to use or abstract this computer program only in     *
 *   accordance with the terms of the NAND FLASH MEMORY SOFTWARE LICENSE     *
 *   AGREEMENT for the sole purpose of implementing a product based on       *
 *   Samsung Electronics Co., Ltd. products. No other rights to reproduce,   *
 *   use, or disseminate this computer program, whether in part or in        *
 *   whole, are granted.                                                     *
 *                                                                           *
 *   Samsung Electronics Co., Ltd. makes no representation or warranties     *
 *   with respect to the performance of this computer program, and           *
 *   specifically disclaims any responsibility for any damages,              *
 *   special or consequential, connected with the use of this program.       *
 *                                                                           *
 *---------------------------------------------------------------------------*
*/   
/**
 * @version     RFS_1.3.1_b055-tinyBML_1.1.1_b002-XSR_1.5.2p4_b122_RTM
 * @file	drivers/tbml/tbml_block.c
 * @brief	This file is TinyBML common part which supports the raw interface
 *		It provides block device operations and utilities
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>

#include <asm/errno.h>
#include <asm/uaccess.h>

#include "tiny_base.h"


/**
 * tbml block open operation 
 * @param inode		block device inode
 * @param file		block device file
 * @return 		0 on success, otherwise on failure
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int tbml_block_open(struct block_device *bdev, fmode_t mode)
{
       u32 volume, minor;
       int ret;

       minor = MINOR(bdev->bd_dev);
#else
static int tbml_block_open(struct inode *inode, struct file *file)
{
	u32 volume, minor;
	int ret;

	minor = MINOR(inode->i_rdev);
#endif
	volume = tiny_vol(minor);

	ret = tbml_open(volume);

	if (ret) {
		printk(KERN_ERR "BML: open error = 0x%x\n", ret);
		return -ENODEV;
	}

	return 0;
}

/**
 * tbml_block_release - BML block release operation 
 * @param inode		block device inode
 * @param file		block device file
 * @return 		0 on success
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int tbml_block_release(struct gendisk *disk, fmode_t mode)
{
       u32 volume, minor;

       minor = disk->first_minor;
#else
static int tbml_block_release(struct inode *inode, struct file *file)
{
	u32 volume, minor;

	minor = MINOR(inode->i_rdev);
#endif
	volume = tiny_vol(minor);

	tbml_close(volume);

	return 0;
}

/**
 * BML raw block I/O control
 * @param inode		block device inode
 * @param file		block device file
 * @param cmd		IOCTL command - ref. include/linux/xsr_if.h
 * @param arg		arguemnt from user
 * @return 		0 on success, otherwise on failure
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int tbml_block_ioctl(struct block_device *bdev, fmode_t mode,
			unsigned cmd, unsigned long arg)
#else
static int tbml_block_ioctl(struct inode *inode, struct file *file,
			unsigned cmd, unsigned long arg)
#endif
{
	/* Not implement code */
	return 0;
}

/**
 * TinyBML common block device operations
 */
static struct block_device_operations tbml_block_fops = {
	.owner		= THIS_MODULE,
	.open		= tbml_block_open,
	.release	= tbml_block_release,
	.ioctl		= tbml_block_ioctl,
};

/**
 * tbml_get_block_device_ops
 * @return		XSR common block device operations
 */
struct block_device_operations *tbml_get_block_device_operations(void)
{
	return &tbml_block_fops;
}

#ifdef CONFIG_RFS_TINY_DEBUG
static unsigned int tbml_iostat[IO_DIRECTION][MAX_NUMBER_SECTORS + 1];

void tbml_count_iostat(int num_sectors, int rw)
{
	if (num_sectors > MAX_NUMBER_SECTORS)
		num_sectors = MAX_NUMBER_SECTORS;
	if (rw) /* write? */
		rw = 1;
	tbml_iostat[rw][num_sectors] ++;
}

static int tbml_show_iostat(char *page, char **start, off_t off, 
			int count, int *eof, void *data)
{
	int i, j;
	char *buf = page;

	for (i = 0; i < IO_DIRECTION; i++) {
		buf += sprintf(buf, "* %s [number of sectors: counts]\n ", 
				i ? "WRITE": "READ");
		for (j = 0; j < MAX_NUMBER_SECTORS; j++) {
			if (tbml_iostat[i][j] == 0)
				continue;
			buf += sprintf(buf, "[%d: %d] ", j, tbml_iostat[i][j]);
		}
		buf += sprintf(buf, "\n");
	}

	buf += sprintf(buf, "\n");
	/* NOTE: This page is limited PAGE_SIZE - 80 */
	*eof = 1;
	return (buf - page);
}

static int tbml_reset_iostat(struct file *file, const char *buffer,
		unsigned long count, void *data) {
	memset(tbml_iostat, 0, sizeof(tbml_iostat));
	printk("BML iostat is reseted!!!\n");
	return count;
}
#endif

/**
 * tbml block module init
 * @return	0 on success
 */
static int __init tbml_block_init(void)
{
	int ret=0;
#ifdef CONFIG_RFS_TINY_DEBUG
	struct proc_dir_entry *tbmlinfo_entry = NULL;
	tbmlinfo_entry = create_proc_entry(TBML_IOSTAT_PROC_NAME, 
			S_IFREG | S_IWUSR | S_IRUGO, tiny_proc_dir);
	tbmlinfo_entry->read_proc = tbml_show_iostat;
	tbmlinfo_entry->write_proc = tbml_reset_iostat;
#endif

	ret = tbml_blkdev_init();
	if (ret != 0)
		DEBUG(TBML_DEBUG_LEVEL0,"tbml_blkdev_init error\n");

	return 0;
}
/**
 * BML block module exit
 */
static void __exit tbml_block_exit(void)
{

#ifdef CONFIG_RFS_TINY_DEBUG
	remove_proc_entry(TBML_IOSTAT_PROC_NAME, NULL);
#endif
	tbml_blkdev_exit();
}

module_init(tbml_block_init);
module_exit(tbml_block_exit);

MODULE_LICENSE("Samsung Proprietary");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("TinyBML common block device layer");
