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
 * @file	drivers/tbml/tiny_base.h
 * @brief	The most commom part and some inline functions to mainipulate
 *		the XSR instance (volume specification, partition table)
 *
 */

#ifndef _XSR_BASE_H_
#define _XSR_BASE_H_

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/xsr_if.h>

#include <tbml_common.h>
#include <onenand_interface.h>
#include <onenand_lld.h>
#include <tbml_interface.h>
#include <tbml_type.h>
#include <gbbm.h>

#include <os_adapt.h>
#include <tbml_type.h>

/*
 * kernel version macro
 */
#undef XSR_FOR_2_6
#undef XSR_FOR_2_6_14
#undef XSR_FOR_2_6_15
#undef XSR_FOR_2_6_19

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#define XSR_FOR_2_6             1
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 19)
#define XSR_FOR_2_6_19          1
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
#define XSR_FOR_2_6_15          1
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 14)
#define XSR_FOR_2_6_14          1
#endif

#define TBML_BLK_DEVICE_RAW		139
#define PARTN_BITS			5
#define PARTN_MASK      MASK(PARTN_BITS)
#define SECTOR_SIZE             512
#define SECTOR_BITS             9
#define OOB_SIZE		16
#define OOB_BITS		4
#define SECTOR_MASK             MASK(SECTOR_BITS)
#define MAX_LEN_PARTITIONS	(sizeof(part_info))
#define ONENAND_PAGE_SHIFT		0x800
#define SCTS_PER_PAGE	4

#define MAX_NUMBER_SECTORS  16
#define IO_DIRECTION        2
#define STL_IOSTAT_PROC_NAME    "stl-iostat"
#define TBML_IOSTAT_PROC_NAME    "tbml-iostat"

#ifdef XSR_TIMER
#define DECLARE_TIMER   struct timeval start, stop
#define START_TIMER()   do { do_gettimeofday(&start); } while (0)
#define STOP_TIMER(name)                    \
do {                                \
    do_gettimeofday(&stop);                 \
    if (stop.tv_usec < start.tv_usec) {         \
        stop.tv_sec -= (start.tv_sec + 1);      \
        stop.tv_usec += (1000000U - start.tv_usec); \
    } else {                        \
        stop.tv_sec -= start.tv_sec;            \
        stop.tv_usec -= start.tv_usec;          \
    }                           \
    printk("STOP: %s: %d.%06d",             \
     name, (int) (stop.tv_sec), (int) (stop.tv_usec));  \
} while (0)

#else
#define DECLARE_TIMER       do { } while (0)
#define START_TIMER()       do { } while (0)
#define STOP_TIMER(name)    do { } while (0)
#endif

#ifdef CONFIG_PROC_FS
extern struct proc_dir_entry *tiny_proc_dir;
#endif

#define XSR_PROC_DIR		"tinyrfs"
#define XSR_PROC_TBMLINFO	"tinyinfo"

extern struct semaphore tiny_mutex;

tbml_vol_spec *tiny_get_vol_spec(u32 volume);
part_info   *tiny_get_part_spec(u32 volume);

static inline unsigned int tiny_minor(unsigned int vol, unsigned int part)
{
	return ((vol << PARTN_BITS) + (part + 1));
}

static inline unsigned int tiny_vol(unsigned int minor)
{
	return (minor >> PARTN_BITS);
}

/*Get partition*/
static inline unsigned int tiny_part(unsigned int minor)
{
	return (minor & PARTN_MASK) - 1;
}

static inline unsigned int tiny_is_whole_dev(unsigned int part_no)
{
	return (part_no >> PARTN_BITS);
}

static inline unsigned int tiny_parts_nr(part_info *pt)
{
	return (pt->nNumOfPartEntry);
}

static inline XSRPartEntry *tiny_part_entry(part_info *pt, unsigned int no)
{
	return &(pt->stPEntry[no]);
}

static inline unsigned int tiny_part_blocks_nr(part_info *pt, unsigned int no)
{
	return (pt->stPEntry[no].nNumOfBlks);
}

static inline unsigned int tiny_part_id(part_info *pt, unsigned int no)
{
	return (pt->stPEntry[no].nID);
}

static inline unsigned int tiny_part_attr(part_info *pt, unsigned int no)
{
	return (pt->stPEntry[no].nAttr);
}

static inline unsigned int tiny_part_start(part_info *pt, unsigned int no)
{
	return (pt->stPEntry[no].n1stVbn);
}

static inline unsigned int tiny_part_size(part_info *pt, unsigned int no)
{
	return (pt->stPEntry[no].nNumOfBlks << SECTOR_BITS);
}

/*Get volume*/
static inline unsigned int tiny_vol_spb(tbml_vol_spec *volume)
{
	return (volume->nPgsPerBlk * volume->nSctsPerPg);
}

static inline unsigned int tiny_vol_spp(tbml_vol_spec *volume)
{
	return (volume->nSctsPerPg);
}

static inline unsigned int tiny_vol_sectors_nr(tbml_vol_spec *volume)
{
	return (volume->nSctsPerPg * volume->nPgsPerBlk * volume->nNumOfUsBlks);
}

static inline unsigned int tiny_vol_blksize(tbml_vol_spec *volume)
{
	return ((volume->nPgsPerBlk * volume->nSctsPerPg) << SECTOR_BITS);
}

static inline unsigned int tiny_vol_block_nr(tbml_vol_spec *volume)
{
	 return (volume->nNumOfUsBlks);
}

int tiny_update_vol_spec(u32 volume);
int tiny_read_partitions(u32 volume, BML_PARTTAB_T *parttab);
struct block_device_operations *tbml_get_block_device_operations(void);
int tbml_blkdev_init(void);
void tbml_blkdev_exit(void);
int tbml_update_blkdev_param(u32 minor, u32 blkdev_size, u32 blkdev_blksize);

static inline unsigned int xsr_stl_sectors_nr(stl_info_t *ssp)
{
	return (ssp->total_sectors);
}

static inline unsigned int xsr_stl_page_size(stl_info_t *ssp)
{
	return (ssp->page_size);
}

/* kernel 2.6 */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/blkdev.h>

struct tiny_dev {        
	struct request			*req;
	struct list_head 		list;
	int                     size;
	spinlock_t              lock;
	struct request_queue    *queue;
	struct gendisk          *gd;
	int                     dev_id;
	struct scatterlist	*sg;	
};
#else
/* Kernel 2.4 */
#ifndef __user
#define __user
#endif
#endif

#endif	/* _TINY_BASE_H_ */
