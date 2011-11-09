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
 * @file	drivers/tbml/tiny_base.c
 * @brief	This file is a basement for TinyBML adoption. It povides 
 *		partition management, proc inteface, contexts management
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>

#include "tiny_base.h"

/* This is the contexts to keep the specification of volume */
static tbml_vol_spec	vol_spec[XSR_MAX_VOLUME];
static part_info	part_spec[XSR_MAX_VOLUME];

struct proc_dir_entry *tiny_proc_dir = NULL;

EXPORT_SYMBOL(tiny_proc_dir);

/* To protect TinyBML operations, this semaphore lock TinyBML codes */
DECLARE_MUTEX(tiny_mutex);

int (*sec_stl_delete)(dev_t dev, u32 start, u32 nums, u32 b_size) = NULL;

EXPORT_SYMBOL(sec_stl_delete);

/* OAM shared memory */
struct semaphore xsr_sem[XSR_MAX_VOLUME];
int semaphore_use[XSR_MAX_VOLUME] = {0,};
EXPORT_SYMBOL(xsr_sem);
EXPORT_SYMBOL(semaphore_use);

/* LLD shared memory */
unsigned short result_ctrl_status[XSR_MAX_VOLUME];
unsigned short tiny_shared;
unsigned short xsr_shared;
unsigned short tiny_block_shared;
unsigned short xsr_block_shared;
EXPORT_SYMBOL(result_ctrl_status);
EXPORT_SYMBOL(tiny_shared);
EXPORT_SYMBOL(xsr_shared);
EXPORT_SYMBOL(tiny_block_shared);
EXPORT_SYMBOL(xsr_block_shared);

#define NR_BLOCKS(pi, i) (pi->stPEntry[i].n1stVbn + pi->stPEntry[i].nNumOfBlks)
/**
 * PARTITION - make a description of one partition
 * @param pi	: pointer to store the partition table
 * @param i	: a partition number
 * @param id	: partition ID (ref. drivers/tbml/include/tbml_interface.h)
 * @param blocks: the numbers of block
 * @param attr	: the partition attribute
 */
static inline void PARTITION(part_info *pi, u32 i, u32 id, u32 blocks, u32 attr)
{
	pi->stPEntry[i].nID = id;
	pi->stPEntry[i].nAttr = attr;

	if (i == 0)
		pi->stPEntry[i].n1stVbn = 0;
	else
		pi->stPEntry[i].n1stVbn = NR_BLOCKS(pi, (i - 1));

	pi->stPEntry[i].nNumOfBlks = blocks;
}

/**
 * tiny_get_vol_spec - get a volume instance
 * @param volume	: a volume number
 */
tbml_vol_spec *tiny_get_vol_spec(u32 volume)
{
	return &vol_spec[volume];
}

/**
 * tiny_get_part_spec - get a partition instance 
 * @param volume	: a volume number
 */

part_info *tiny_get_part_spec(u32 volume)
{
	return &part_spec[volume];
}


/**
 * tiny_update_vol_spec - update volume & partition instance from the device
 * @param volume	: a volume number
 */
int tiny_update_vol_spec(u32 volume)
{
	int error, len;
        tbml_vol_spec *vs;
        part_info *pi;

	vs = tiny_get_vol_spec(volume);
	pi = tiny_get_part_spec(volume);

	memset(vs, 0x00, sizeof(tbml_vol_spec));
	memset(pi, 0x00, sizeof(part_info));
		
	error = tbml_get_vol_spec(volume, vs);
	/* I/O error */
	if (error)
		return -1;

	error = tbml_ioctl(volume, BML_IOCTL_GET_FULL_PI, 
				NULL, 0, (u8 *) pi, sizeof(part_info), &len);
	/* I/O error */
	if (error)
		return -1;

	return 0;
}

/**
 * tiny_read_partitions - read partition table into the device
 * @param volume	: a volume number
 * @param parttab	: buffer to store the partition table
 */
int tiny_read_partitions(u32 volume, BML_PARTTAB_T *parttab)
{
	int error;
	u32 partno;
        part_info *pi;

	pi = tiny_get_part_spec(volume);

	/*could't find vaild part table*/
	if (!pi->nNumOfPartEntry) {
		DEBUG(TBML_DEBUG_LEVEL0,"update partition spec\n");
		error = tiny_update_vol_spec(volume);
		/* I/O error */
		if (error) /*never action, because of low-formatting*/
			DEBUG(TBML_DEBUG_LEVEL0,"error(%x)", error);
	}

	DEBUG(TBML_DEBUG_LEVEL0,"pi->nNumOfPartEntry: %d", pi->nNumOfPartEntry);
	parttab->num_parts =  pi->nNumOfPartEntry;
	for (partno = 0; partno < pi->nNumOfPartEntry; partno++) {
		parttab->part_size[partno]	= pi->stPEntry[partno].nNumOfBlks;
		parttab->part_id[partno]	= pi->stPEntry[partno].nID;
		parttab->part_attr[partno]	= pi->stPEntry[partno].nAttr;
	}

	return 0;
}

#ifdef CONFIG_PROC_FS
#include "include/tbml_type.h"
static u8 bad_block_map[1024];

/**
 * tiny_proc_output - Display the partition information
 * @param buf		buffer to write data
 * @return		none
 *
 * If you use the CONFIG_RFS_XSR_DEBUG, it will print the chip id
 */

static int tiny_proc_output(char *buf)
{
	int i, nchips;
	int start, size, block_nr;
	int ret;
	unsigned int j, num_bad=0, index;
	bmf *bmap;
	char *p;

	part_info *pi;
	tbml_vol_spec *vs;

	nchips = XSR_MAX_VOLUME;

	p = buf;

	p += sprintf(p, "minor       position           size    blocks       id\n");

	for (i = 0; i < nchips; i++) {
		vs = tiny_get_vol_spec(i);
		/*no more device*/
		if ( vs->nPgsPerBlk == 0)
			continue;
		pi = tiny_get_part_spec(i);

		for (j = 0; j < tiny_parts_nr(pi); j++) {
			start = tiny_part_start(pi, j) * tiny_vol_blksize(vs);
			size = tiny_part_blocks_nr(pi, j) * tiny_vol_blksize(vs);
			block_nr = tiny_part_blocks_nr(pi, j);

			p += sprintf(p, "%4u: 0x%08x-0x%08x 0x%08x %6d %8d\n",
				tiny_minor(i, j),
				start, start + size, size, block_nr,
				tiny_part_id(pi, j));
		}
	}

	for (i = 0; i < nchips; i++) {
		ret = tbml_open(i); 
		if (ret)
			continue;
		index = 0;
		ret = tbml_ioctl(i, BML_IOCTL_GET_BMI, (u8 *) &index, sizeof(u32), 
			(u8 *) bad_block_map, (SECTOR_SIZE<<1), &num_bad);
		if (ret)
			continue;

		num_bad /= sizeof(bmf);
		bmap = (bmf *) bad_block_map;

		p += sprintf(p, "\n(%d) bad mapping information\n", i);
		p += sprintf(p, "  No  BadBlock   RsvBlock\n");
		for (j  = 0; j < num_bad; j++) {
			p += sprintf(p, "%4d: %8d  %8d \n", j, bmap->nSbn, bmap->nRbn);
			bmap ++;
		}

		tbml_close(i); 
	}

	return p - buf;
}

/**
 * tbml_read_proc - XSR kernel proc interface
 */
static int tbml_read_proc(char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	int len = tiny_proc_output(page);
	if (len <= off + count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len > count) len = count;
	if (len < 0) len = 0;
	return len;
}

#endif	/* CONFIG_PROC_FS */

/**
 * tiny_init - [Init] initalize the tiny
 */
static int __init tiny_init(void)
{
	int error;
	DECLARE_TIMER;

	/*initialize global array*/
	START_TIMER();
	error = tbml_init();
	STOP_TIMER("TinyBML_init");

	/*It will call PAM_RegLFT()- Register LLD function by Platform*/
	if (error) {
		printk(KERN_ERR "TinyBML_init: error (%x)\n", error);
		printk(KERN_ERR "Check the onenand_interface module\n");
		return -ENXIO;
	}

	/* make proc directory */
	tiny_proc_dir = proc_mkdir(XSR_PROC_DIR, NULL);
	if (!tiny_proc_dir)
		return -EINVAL;

	/* make proc entry and link the read function*/
	create_proc_read_entry(XSR_PROC_TBMLINFO, 0, tiny_proc_dir, tbml_read_proc, NULL);

	printk("TinyBML init registered\n");

	return 0;
}

/**
 * tiny_exit - exit all and clear proc entry
 */
static void __exit tiny_exit(void)
{
	remove_proc_entry(XSR_PROC_TBMLINFO, NULL);

	if (tiny_proc_dir)
		remove_proc_entry(XSR_PROC_DIR, NULL);
}

module_init(tiny_init);
module_exit(tiny_exit);

MODULE_LICENSE("Samsung Proprietary");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("XSR common device layer");
