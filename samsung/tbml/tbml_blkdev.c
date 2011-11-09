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
 * @file	drivers/tbml/tbml_blkdev.c
 * @brief	This file is TinyBML I/O part which supports linux kernel 2.6
 *		It provides (un)registering block device, request function
 *
 */

#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
#include <linux/platform_device.h>
#else
#include <linux/device.h>
#endif

#include "tiny_base.h"

#define DEVICE_NAME		"tbml"
#define MAJOR_NR                TBML_BLK_DEVICE_RAW

void tbml_count_iostat(int num_sectors, int rw);

/**
 * list to keep track of each created block devices
 */
static DECLARE_MUTEX(tbml_list_mutex);
static LIST_HEAD(tbml_list); 

#ifdef CONFIG_PM
#include <linux/pm.h>
extern unsigned short xsr_shared;

/**
 * function pointer to call BML module's suspend/resume
 */
#ifdef XSR_FOR_2_6_15
int (*bml_suspend_fp)(struct device *dev, pm_message_t state);
int (*bml_resume_fp)(struct device *dev);
#else
int (*bml_suspend_fp)(struct device *dev, u32 state, u32 level);
int (*bml_resume_fp)(struct device *dev, u32 level);
#endif

EXPORT_SYMBOL(bml_suspend_fp);
EXPORT_SYMBOL(bml_resume_fp);

#ifdef __BML_INTERNAL_PM_TEST__
#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/freezer.h>
/**
 * test for suspend/resume using /proc
 */
struct proc_dir_entry *pm_entry = NULL;
extern struct proc_dir_entry *tiny_proc_dir;
#endif /* __BML_INTERNAL_PM_TEST__ */
#endif /* CONFIG_PM */

/**
 * transger data from BML to buffer cache
 * @param volume	: device number
 * @param partno 	: 0~15: partition, other: whole device
 * @param req		: request description
 * @return		1 on success, 0 on failure 
 *
 * It will erase a block before it do write the data
 */
static int tbml_transfer(u32 volume, u32 partno, const struct request *req, u32 data_len)
//static int tbml_transfer(u32 volume, u32 partno, const struct request *req)
{
	unsigned long sector, nsect;
	char *buf;
	u32 spb_shift, vbn = 0, vsn = 0;
	tbml_vol_spec *vs;
	part_info *ps;
	int ret = 0;

	if (!blk_fs_request(req))
		return 0;

   	sector = blk_rq_pos(req);
	nsect = data_len >> 9;

	//sector = req->sector;
	//nsect = req->current_nr_sectors;
	buf = req->buffer;

	vs = tiny_get_vol_spec(volume);
	ps = tiny_get_part_spec(volume);

	/* from device spec */
	if (!tiny_is_whole_dev(partno))
		vsn = tiny_part_start(ps, partno) * tiny_vol_spb(vs);
	vsn += sector;

	spb_shift = ffs(tiny_vol_spb(vs)) - 1;
	vbn = vsn >> spb_shift;

	switch (rq_data_dir(req)) {
	case READ:
		ret = tbml_mread(volume, vsn, nsect, buf, NULL, BML_FLAG_ECC_ON);
		tbml_count_iostat(nsect, READ);
		break;

	case WRITE:
		/* Not implement write code */
		break;

	default:
		printk(KERN_NOTICE "Unknown request %d\n", rq_data_dir(req));
		return 0;
	}

	/* I/O error */
	if (ret) {
		DEBUG(TBML_DEBUG_LEVEL0,"bml: transfer error =%d\n", ret);
		return 0;
	}

	return 1;
}

/**
 * request function which is do read/write sector
 * @param rq	: request queue which is created by blk_init_queue()
 * @return		none
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 20)
static void tbml_request(struct request_queue *rq)
#else
static void tbml_request(request_queue_t *rq)
#endif
{
        u32 minor, volume, partno, spp_mask;
        struct request *req;
        struct tiny_dev *dev;
        //int ret = 0;

    	int trans_ret;
       	int error = 0;
    	u32 len = 0;
	
	    tbml_vol_spec *vs;
	    
        dev = rq->queuedata;
        if (dev->req)
                return;
        while ((dev->req = req = blk_peek_request(rq)) != NULL) {
        //while ((dev->req = req = elv_next_request(rq)) != NULL) {

                spin_unlock_irq(rq->queue_lock);

                minor = dev->gd->first_minor;
                volume = tiny_vol(minor);
                partno = tiny_part(minor);
                
           		vs = tiny_get_vol_spec(volume);
        		spp_mask = vs->nSctsPerPg - 1;

           		len = blk_rq_cur_bytes(req);
		        if (!( blk_rq_pos(req) & spp_mask) && ( blk_rq_cur_sectors(req) != blk_rq_sectors(req)))
		        {
			        blk_rq_map_sg(rq, req, dev->sg);
			        if (!((dev->sg->length >> SECTOR_BITS) & 0x7))
			        {
				        len = dev->sg->length;
			        }
			        if (len > blk_rq_bytes(req))
			        {
				        len = blk_rq_bytes(req);
			        }

		        }
		        trans_ret = tbml_transfer(volume, partno, req, len);
                //ret = tbml_transfer(volume, partno, req);

                spin_lock_irq(rq->queue_lock);
                
           		if (trans_ret)
		        {
			        error = 0;
		        } else
		        {
			        error = -EIO;
		        }
		        /* don't need to check if request is finished */
		        if (blk_rq_sectors(req) <= (len >> 9))
			        list_del_init(&req->queuelist);
		        __blk_end_request(req, error, len);
                //end_request(req, ret);
        }
}


/**
 * free all disk structure
 * @param *dev	xsr block device structure (ref. inlcude/linux/xsr/xsr_if.h)
 * @return 	none
 *
 */
static void tbml_del_disk(struct tiny_dev *dev)
{
	if (dev->gd) {
		del_gendisk(dev->gd);
		put_disk(dev->gd);
	}
	
	kfree(dev->sg);
	
	if (dev->queue)
		blk_cleanup_queue(dev->queue);
	list_del(&dev->list);
	kfree(dev);
	return;
}

/**
 * add each partitions as disk
 * @param volume	a volume number
 * @param partno	a partition number
 * @return		0 on success, otherwise on error
 *
 */
static int tbml_add_disk(u32 volume, u32 partno)
{
	u32 minor, sectors;
	struct tiny_dev *dev;
	tbml_vol_spec *vs;
	part_info *pi;

	dev = kmalloc(sizeof(struct tiny_dev), GFP_KERNEL);
	/* memory error */
	if (!dev) {
		printk("%s[%d] kmalloc error !!! \n", __func__, __LINE__);
		return -ENOMEM;
	}
	memset(dev, 0, sizeof(struct tiny_dev));

	spin_lock_init(&dev->lock);
	INIT_LIST_HEAD(&dev->list);
	down(&tbml_list_mutex);
	list_add(&dev->list, &tbml_list);
	up(&tbml_list_mutex);

	/* init queue */
	dev->queue = blk_init_queue(tbml_request, &dev->lock);
	dev->queue->queuedata = dev;
	dev->req = NULL;

    dev->sg = kmalloc(sizeof(struct scatterlist) * dev->queue->limits.max_phys_segments, GFP_KERNEL);
	if (!dev->sg)
	{
		kfree(dev);
		return -ENOMEM;
	}
	
	memset(dev->sg, 0, sizeof(struct scatterlist) * dev->queue->limits.max_phys_segments);
	    
	/* Each GBBM2 partition is a physical disk which has one partition */
	dev->gd = alloc_disk(1);
	/* memory error */
	if (!dev->gd) {
		printk("%s[%d] memory error !!! \n", __func__, __LINE__);
		kfree(dev->sg);
		list_del(&dev->list);
		kfree(dev);
		return -ENOMEM;
	}

	minor = tiny_minor(volume, partno);

	dev->gd->major = MAJOR_NR;
	dev->gd->first_minor = minor;
	dev->gd->fops = tbml_get_block_device_operations();
	dev->gd->queue = dev->queue;

	/* get a volume context and partition info */
	vs = tiny_get_vol_spec(volume);
	pi = tiny_get_part_spec(volume);

	if (minor == 0) {
		snprintf(dev->gd->disk_name, 32, "%s%c", DEVICE_NAME, 'c');
		sectors = tiny_vol_sectors_nr(vs);
	} else {
		snprintf(dev->gd->disk_name, 32, "%s%d", DEVICE_NAME, minor);
		sectors = (tiny_part_blocks_nr(pi, partno) * 
				tiny_vol_blksize(vs)) >> 9;
	}

	/* setup block device parameter array */
	set_capacity(dev->gd, sectors);

	add_disk(dev->gd);
		
	return 0;
}

/**
 * update the block device parameter 
 * @param minor			minor number to update device
 * @param blkdev_size		it contains the size of all block-devices by 1KB
 * @param blkdev_blksize	it contains the size of logical block
 * @return			0 on success, -1 on failure
 */
int tbml_update_blkdev_param(u32 minor, u32 blkdev_size, u32 blkdev_blksize)
{
	struct tiny_dev *dev;
	struct gendisk *gd = NULL;
	struct list_head *this, *next;
	unsigned int nparts = 0;
	part_info *pi;
	u32 volume, partno;

	down(&tbml_list_mutex);
	list_for_each_safe(this, next, &tbml_list) {
		dev = list_entry(this, struct tiny_dev, list);
		if (dev && dev->gd && dev->gd->first_minor) {
			volume = tiny_vol(dev->gd->first_minor);
			pi = tiny_get_part_spec(volume);
			nparts = tiny_parts_nr(pi);

			if (dev->gd->first_minor == minor) {
				gd = dev->gd;
				break;
			} else if (dev->gd->first_minor > nparts)
				/* update for removed disk */
				tbml_del_disk(dev);

		}
	}
	up(&tbml_list_mutex);

	if (!gd) {
		volume = tiny_vol(minor);
		partno = tiny_part(minor);
		if (tbml_add_disk(volume, partno)) {
			/* memory error */
			DEBUG(TBML_DEBUG_LEVEL0,"gd updated failed, minor = %d\n", minor);
			return -1;
		}
	} else {
		/* blkdev_size use KB, set_capacity need numbers of sector */
		set_capacity(gd, blkdev_size << 1U);

	}

	return 0;
}


/**
 * free resource 
 * @return 		none
 * @remark bml_blkdev is the build-in code, so it will be never reached
 */
static void tbml_blkdev_free(void)
{
	struct tiny_dev *dev;
	struct list_head *this, *next;

	down(&tbml_list_mutex);
	list_for_each_safe(this, next, &tbml_list) {
		dev = list_entry(this, struct tiny_dev, list);
		tbml_del_disk(dev);
	}
	up(&tbml_list_mutex);
}

/**
 * tbml_blkdev_create - create device node
 * @return		none
 */
static int tbml_blkdev_create(void)
{
	u32 volume, partno, i;
	part_info *pi;
	unsigned int nparts;
	int ret;

	for (volume = 0; volume < XSR_MAX_VOLUME; volume++) {
		ret = tbml_open(volume);

		if (ret) {
			DEBUG(TBML_DEBUG_LEVEL0,"No such device or address, %d(0x%x)", volume, ret);
			continue;
		}

		ret = tiny_update_vol_spec(volume);
		if (ret) {
			tbml_close(volume);
			continue;
		}

		pi = tiny_get_part_spec(volume);
		nparts = tiny_parts_nr(pi);

		/*
		 * which is better auto or static?
		 */
		for (i = 0; i < MAX_FLASH_PARTITIONS + 1; i++) {
			/* when partno has -1, it means whole device */
			partno = i - 1;
			/* there is no partition */
			if (i > nparts)
				break;

			ret = tbml_add_disk(volume, partno);
			if (ret)
				continue;
		}
		tbml_close(volume);
	}

	return 0;
}


/**
 * flush all volume of bml
 * @return              0 on success
 */
#ifdef CONFIG_PM
static int tbml_flush_all_volume(void)
{
	u32 volume;
	int ret = 0;

	for (volume = 0; volume < XSR_MAX_VOLUME; volume++) {
		ret = tbml_open(volume);
		if (ret) {
			DEBUG(TBML_DEBUG_LEVEL0,"No such device or address, %d(0x%x)", volume, ret);
			return ret;
		}

		ret = tbml_flush(volume, BML_NORMAL_MODE);
		if (ret) {
			DEBUG(TBML_DEBUG_LEVEL0,"Can't Flush Operation, %d(0x%x)", volume, ret);
			tbml_close(volume);
			return ret;
		}
		tbml_close(volume);
	}
	return 0;
}

/**
 * suspend the bml devices
 * @param dev           device structure
 * @param state         device power management state
 * @return              0 on success
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
static int tbml_suspend(struct device *dev, pm_message_t state)
#else
static int tbml_suspend(struct device *dev, u32 state, u32 level)
#endif
{
	int ret = 0;

	DEBUG(TBML_DEBUG_LEVEL1,"TinyBML: tbml_suspend() called");

	/* to finish cache read command */
	if (!xsr_shared) {
		ret = tbml_flush_all_volume();	
		if (ret) {
			DEBUG(TBML_DEBUG_LEVEL0, "TinyBML: tbml_flush_all_volume fail");
			return ret;
		}
	}

	/* if BML module loaded */
	if (NULL != bml_suspend_fp) {
		DEBUG(TBML_DEBUG_LEVEL1,"TinyBML: bml_suspend start");

		/* call suspend function of BML module */
#ifdef XSR_FOR_2_6_15
		ret = bml_suspend_fp(NULL, state);
#else
		ret = bml_suspend_fp(NULL, 0, 0);
#endif
		if (ret)
			DEBUG(TBML_DEBUG_LEVEL0,"TinyBML: bml_suspend_fp fail");
	}

	return ret;
}

/**
 * resume the bml devices
 * @param dev           device structure
 * @return              0 on success
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
static int tbml_resume(struct device *dev)
#else
static int tbml_resume(struct device *dev, u32 level)
#endif
{
	int ret = 0;

	DEBUG(TBML_DEBUG_LEVEL1,"TinyBML: tbml_resume called");

	/* if BML module loaded */
	if (NULL != bml_resume_fp) {
		DEBUG(TBML_DEBUG_LEVEL1,"TinyBML: bml_resume start");

		/* call resume function of BML module */
#ifdef XSR_FOR_2_6_15
		ret = bml_resume_fp(NULL);
#else
		ret = bml_resume_fp(NULL, 0);
#endif
		if (ret)
			DEBUG(TBML_DEBUG_LEVEL0,"TinyBML: bml_resume_fp fail : ret=%d\n",ret);
	}	
	
	return ret;
}

#ifdef __BML_INTERNAL_PM_TEST__
/**
 * test for suspend/resume using /proc
 * @param file		file to write
 * @param buffer	buffer to write
 * @param count		buffer size
 * @param data		data address of proc_dir_entry
 * @return		0 on success, otherwise on error
 *
 * usage) echo 1 > /proc/tinyrfs/PM
 *
 */
int pm_write_proc(struct file *file, const char *buffer,
		unsigned long count, void *data)
{
	char *str;
	int ret = 0;
	unsigned int test_number = 0;

	struct device *dev = NULL;
	pm_message_t state;

	str = kmalloc((size_t)count, GFP_KERNEL);
	if (!str) {
		DEBUG(TBML_DEBUG_LEVEL0,"kmalloc error !!! \n");
		return -ENOMEM;
	}
	str[count] = '\0';

	ret = copy_from_user(str, buffer, count);
	if (ret) {
		DEBUG(TBML_DEBUG_LEVEL0,"copy_from_user error !!!\n");
		return -EFAULT;
	}

	test_number = simple_strtoul(str, NULL, 0);
	DEBUG(TBML_DEBUG_LEVEL1,"test number :%d\n",test_number);

	switch (test_number) {
		case 1: /* auto suspend/resume test */
		{
			DEBUG(TBML_DEBUG_LEVEL0,"Start suspend !!!\n");
			
			/* sleep tasks */
			ret = freeze_processes();
			if (ret) {
				DEBUG(TBML_DEBUG_LEVEL0,"freeze process fail. return : %d\n",ret);
				kfree(str);
				return ret;
			}
			/* do suspend */
			ret = tbml_suspend(dev,state);
			if (ret) {
				DEBUG(TBML_DEBUG_LEVEL0,"Suspend fail. return : %d\n",ret);
				kfree(str);
				return ret;
			}

			/* wait 3 second. just simulation run time */
			mdelay(3000);

			DEBUG(TBML_DEBUG_LEVEL0,"Starting resume !!!\n");

			/* do resume */
			ret = tbml_resume(dev);
			if (ret) {
				DEBUG(TBML_DEBUG_LEVEL0,"Resume fail. return : %d\n",ret);
				kfree(str);
				return ret;
			}

			/* wake up tasks */
			thaw_processes();

			break;
		}
		default:
			DEBUG(TBML_DEBUG_LEVEL0,"Usage) echo 1 > /proc/tinyrfs/PM");
			break;
	}
	kfree(str);
	return count;
}
#endif /* __BML_INTERNAL_PM_TEST__ */
#endif /* CONFIG_PM */

/**
 * initialize bml driver structure
 */
static struct device_driver tbml_driver = {
	.name		= DEVICE_NAME,
	.bus		= &platform_bus_type,
#ifdef CONFIG_PM
	.suspend	= tbml_suspend,
	.resume		= tbml_resume,
#endif
};

/**
 * initialize bml device structure
 */
static struct platform_device tbml_device = {
	.name	= DEVICE_NAME,
};

/**
 * create device node, it will scan every chips and partitions
 * @return 	0 on success, otherwise on error
 */
int __init tbml_blkdev_init(void)
{
#ifdef __BML_INTERNAL_PM_TEST__
#ifdef CONFIG_PM
	/* test for suspend/resume using /proc */
	pm_entry = create_proc_entry("PM", S_IFREG | S_IWUSR | S_IRUGO, tiny_proc_dir);
	if (pm_entry != NULL) {
		pm_entry->write_proc = pm_write_proc;
	}
#endif
#endif /* __BML_INTERNAL_PM_TEST__ */

	if (register_blkdev(MAJOR_NR, DEVICE_NAME)) {
		printk(KERN_WARNING "raw: unable to get major %d\n", MAJOR_NR);
		return -EAGAIN;
	}

	if (tbml_blkdev_create()) {
		unregister_blkdev(MAJOR_NR, DEVICE_NAME);
		return -ENOMEM;
	}

	if (driver_register(&tbml_driver)) {
		tbml_blkdev_free();
		unregister_blkdev(MAJOR_NR, DEVICE_NAME);
		return -ENODEV;
	}

	if (platform_device_register(&tbml_device)) {
		driver_unregister(&tbml_driver);
		tbml_blkdev_free();
		unregister_blkdev(MAJOR_NR, DEVICE_NAME);
		return -ENODEV;
	}

	return 0;
}

/**
 * initialize the bml devices
 * @return		0 on success, otherwise on failure
 */
void __exit tbml_blkdev_exit(void)
{
	platform_device_unregister(&tbml_device);
	driver_unregister(&tbml_driver);

	tbml_blkdev_free();

	unregister_blkdev(MAJOR_NR, DEVICE_NAME);
}

MODULE_LICENSE("Samsung Proprietary");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("The kernel 2.6 block device interface for TinyBML");
