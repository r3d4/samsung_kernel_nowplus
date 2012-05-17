/*
 * sysfs.h 
 *
 * Description: This file is an interface to sysfs.c
 *
 * Author: Varun Mahajan <m.varun@samsung.com>
 */

#ifndef _PL_SYSFS_H
#define _PL_SYSFS_H

#include <linux/device.h>
#include <linux/kernel.h>

#ifdef CONFIG_MACH_OSCAR
#include <linux/sensors_core.h>
#endif

extern void P_obj_state_genev(struct input_dev *, u16);

/*extern functions*/
#ifndef CONFIG_MACH_OSCAR
extern int P_sysfs_init(struct device *);
extern void P_sysfs_exit(struct device *);

extern int L_sysfs_init(struct device *);
extern void L_sysfs_exit(struct device *);
#else
extern int P_sysfs_init(struct sensors_dev *);
extern void P_sysfs_exit(struct sensors_dev *);

extern int L_sysfs_init(struct sensors_dev *);
extern void L_sysfs_exit(struct sensors_dev *);
#endif

#endif

