/*
 * sysfs.h 
 *
 * Description: This file is an interface to sysfs.c
 *
 * Author: Varun Mahajan <m.varun@samsung.com>
 */

#ifndef _KXSD9_SYSFS_H
#define _KXSD9_SYSFS_H

#include <linux/device.h>
#include <linux/kernel.h>

#ifndef CONFIG_MACH_OSCAR
/*extern functions*/
extern int KXSD9_sysfs_init(struct device *);
extern void KXSD9_sysfs_exit(struct device *);
#else
#include <linux/sensors_core.h>
extern int KXSD9_sysfs_init(struct sensors_dev *);
extern void KXSD9_sysfs_exit(struct sensors_dev *);
#endif

#endif

