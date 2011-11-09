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
 * @file	drivers/tbml/tbml_blkdev-24.c
 * @brief	This is BML I/O part which supports linux kernel 2.4.
 *		This provides (un)registering block device, request function
 *
 */

#include <linux/init.h>
#include <linux/bitops.h>

#include <asm/uaccess.h>
#include <asm/errno.h>

#include "tiny_base.h"

#define DEVICE_NAME		"tbml"

#define MAJOR_NR                TBML_BLK_DEVICE_RAW
#define DEVICE_REQUEST          tbml_request
#define DEVICE_NR(device)       MINOR(device)
#define DEVICE_ON(device)
#define DEVICE_OFF(device)
#define DEVICE_NO_RANDOM

#include <linux/blk.h>

#define BML_DEVICES		(XSR_MAX_VOLUME * (MAX_FLASH_PARTITIONS + 1))

/* request busy flag */
static int tbml_request_busy;

/**
 * array to keep track of each created block devices
 */
static int tbml_sizes[BML_DEVICES];
static int tbml_blksizes[BML_DEVICES];

/**
 * transfer data to BML
 * @param write		write or not
 * @param volume	volume nuber
 * @param vsn		virutal sector number (bad free)
 * @param sectors	the number of sectors
 * @param mbuf		the pointer of main buffer 
 * @param sbuf		the pointer of spare buffer
 * @return		0 on success, otherwise see also drivers/xsr/Inc/BML.h
 * @pre			before the write operation, the block should be erased	
 */
static inline int tbml_do_transfer(int write, u32 volume, u32 vsn, u32 sectors,
				  unsigned char *mbuf, unsigned char *sbuf)
{
	int i = 0;
	int ret = 0;

	ret = tbml_mread(volume, vsn, sectors, mbuf, sbuf, BML_FLAG_ECC_ON);
	if ( ret != 0 ) {
		DEBUG(TBML_DEBUG_LEVEL0,"tbml_read error occured\n");
		return 1;
	}

	return 0;
}

/**
 * transfer data from BML to buffer cache
 * @param volume	a volume number
 * @param part_no 	0~15: partition, other: whole device
 * @param req		request description
 * @return		1 on success, 0 on failure 
 *
 * It will erase a block before it do write the data
 */
static int tbml_transfer(u32 volume, u32 part_no, const struct request *req)
{
	char *buf;
	u32 vsn = 0;
	int write = 0, ret;

	tbml_vol_spec *vs = tiny_get_vol_spec(volume);
	part_info *ps = tiny_get_part_spec(volume);

	/*from reqeust */
	buf = req->buffer;

	if (req->cmd == WRITE)
		write = 1;

	/*from device spec */
	if (!tiny_is_whole_dev(part_no))
		vsn = tiny_part_start(ps, part_no) * tiny_vol_spb(vs);
	vsn += req->sector;

	ret = tbml_do_transfer(write, volume, vsn, req->current_nr_sectors, buf, NULL);
	tbml_count_iostat(req->current_nr_sectors, write);

	/* I/O error */
	if (ret) {
		DEBUG(TBML_DEBUG_LEVEL0,"bml: transfer error =%d\n", ret);
		return 0;
	}

	return 1;
}

/**
 * request function which is do read/write sector
 * @param rq		request queue which was created by blk_init_queue()
 * @return		none
 */
static void tbml_request(request_queue_t *rq)
{
	struct request *req;
	u32 minor;
	u32 volume;
	part_info *pi;
	tbml_vol_spec *vs;
	u32 partno, end_sector;
	int ret = 0;		/*fail */
	int *busy = (int *) rq->queuedata;

	/* no race here - io_request_lock held */
	if (*busy)
		return;

	*busy = 1;
	while (!list_empty(&rq->queue_head)) {
		INIT_REQUEST;

		ret = 0;
		req = blkdev_entry_next_request(&rq->queue_head);

		spin_unlock_irq(&io_request_lock);

		minor = MINOR(req->rq_dev);
		/* out-of-range input */
		if (minor > BML_DEVICES) {
			printk(KERN_WARNING "raw: request for unknown device\n");
			goto end_req;
		}

		volume = tiny_vol(minor);
		partno = tiny_part(minor);
		vs = tiny_get_vol_spec(volume);
		pi = tiny_get_part_spec(volume);

		if (tiny_is_whole_dev(partno))
			end_sector = tiny_vol_sectors_nr(vs);
		else
			end_sector = tiny_part_blocks_nr(pi, partno) *
			    tiny_vol_spb(vs);

		if (req->sector + req->current_nr_sectors > end_sector) {
			/* out-of-range input */
			printk(KERN_WARNING
			       "raw: request past end of device\n");
			goto end_req;
		}

		ret = tbml_transfer(volume, partno, req);
end_req:
		spin_lock_irq(&io_request_lock);
		end_request(ret);
	}
	*busy = 0;
}

/**
 * free resource 
 * @return 		none
 * @remark bml_blkdev is the build-in code, so it will be never reached
 */
static void tbml_blkdev_free(void)
{
	int i, j;

	for (i = 0; i < XSR_MAX_VOLUME; i++) {
		for (j = 0; j < (MAX_FLASH_PARTITIONS + 1); j++) {
			/* unregister partition instance */
		}
		/* unregister device instance */
	}

}

/**
 * create device node, it will scan every chips and partitions
 * @return 		always 0
 */
static int tbml_blkdev_create(void)
{
	u32 blksizes, sizes;
	u32 minor;
	u32 volume, partno, i;
	u32 nparts;
	int ret;
	tbml_vol_spec *vs;
	part_info *pi;
	DECLARE_TIMER;

	for (volume = 0; volume < XSR_MAX_VOLUME; volume++) {
		START_TIMER();
		ret = tbml_open(volume);
		STOP_TIMER("tbml_open");

		if (ret) {
			DEBUG(TBML_DEBUG_LEVEL0,"No such device or address, %d(0x%x)", volume, ret);
			continue;
		}

		START_TIMER();
		ret = tiny_update_vol_spec(volume);
		STOP_TIMER("BML_IOCtl");
		/* I/O error */
		if (ret)
			continue;
		vs = tiny_get_vol_spec(volume);
		pi = tiny_get_part_spec(volume);
		nparts = tiny_parts_nr(pi);
		blksizes = tiny_vol_spp(vs) << SECTOR_BITS;

		for (i = 0; i < MAX_FLASH_PARTITIONS + 1; i++) {
			partno = i - 1;
			minor = tiny_minor(volume, partno);
			/*No more partition */
			if (i > nparts)
				break;

			if (minor == 0)
				sizes = tiny_vol_sectors_nr(vs) >> 1UL;
			else
				sizes = (tiny_part_blocks_nr(pi, partno) *
				    tiny_vol_blksize(vs)) >> 10UL;

			/*setting the block device param */
			tbml_blksizes[minor] = blksizes;
			tbml_sizes[minor] = sizes;
		}
		tbml_close(volume);
	}

	return 0;
}

/**
 * update the block device parameter - ref. drivers/block/ll_rw_blk.c
 * @param minor			minor number to update device
 * @param blkdev_size		it contains the size of all block-devices by 1KB
 * @param blkdev_blksize	it contains the size of logical block
 * @return			0 on success
 */
int tbml_update_blkdev_param(u32 minor, u32 blkdev_size, u32 blkdev_blksize)
{
	tbml_sizes[minor] = blkdev_size;
	tbml_blksizes[minor] = blkdev_blksize;
	
	return 0;
}

/**
 * initialize the bml devices
 * @return		0 on success, otherwise on failure
 */
int __init tbml_blkdev_init(void)
{
	request_queue_t *q;
	int i;

	for (i = 0; i < MAX_FLASH_PARTITIONS * XSR_MAX_VOLUME; i++) {
		tbml_blksizes[i] = 0;
		tbml_sizes[i] = 0;
	}

	tbml_blkdev_create();

	if (register_blkdev
	    (MAJOR_NR, DEVICE_NAME, tbml_get_block_device_operations())) {
		printk(KERN_WARNING "raw: unable to get major %d\n", MAJOR_NR);
		return -EAGAIN;
	}

	q = BLK_DEFAULT_QUEUE(MAJOR_NR);
	q->queuedata = (void *) &tbml_request_busy;
	blk_init_queue(q, DEVICE_REQUEST);

	read_ahead[MAJOR_NR] = 8;
	blk_size[MAJOR_NR] = tbml_sizes;	/*partition size */
	blksize_size[MAJOR_NR] = tbml_blksizes;	/*size of logical block */

	printk("TBML block device init\n");

	return 0;
}

/**
 * exit the bml devices
 * @return		none
 * @remark bml_blkdev is the build-in code, so it will be never reached
 */
void __exit tbml_blkdev_exit(void)
{
	tbml_blkdev_free();

	unregister_blkdev(MAJOR_NR, DEVICE_NAME);

	/* Fix up the request queue(s) */
	blk_cleanup_queue(BLK_DEFAULT_QUEUE(MAJOR_NR));

	/* Clean up the global arrays */
	read_ahead[MAJOR_NR] = 0;
	blk_size[MAJOR_NR] = NULL;
	blksize_size[MAJOR_NR] = NULL;
}
