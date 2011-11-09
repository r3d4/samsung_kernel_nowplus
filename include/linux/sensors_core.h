/*
 *  Universal sensors core class
 *  
 *  Author : Ryunkyun Park <ryun.park@samsung.com>
 */

 #include <linux/device.h>

#ifndef _SENSORS_CORE_H_
#define _SENSORS_CORE_H_

#if 0 
struct sensors_data {
	const char *name;
	int num_properties;
	mode_t flag;
	
	ssize_t (*sensor_name_show)(struct device *dev, struct device_attribute *attr, char *buf);
	ssize_t (*sensor_name_store)(struct device *dev, struct device_attribute *attr, 
	                    const char *buf, size_t count);
};
#endif

struct sensors_dev {
	int minor;
	struct device *dev;
	struct file_operations *fops;
	struct list_head list;
};


extern int sensors_register(struct device *parent, struct device *child, struct device_attribute *sd, int array_size, struct sensors_dev *sdev);
extern void sensors_unregister(struct sensors_dev *sdev);

extern struct class *sensors_class;

#endif	// End of file
