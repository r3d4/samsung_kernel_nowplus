/*
 *  Universal sensors core class
 *  
 *  Author : Ryunkyun Park <ryun.park@samsung.com>
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/sensors_core.h>

struct class *sensors_class;
static atomic_t sensor_count;

static int sensors_major = 0;

static LIST_HEAD(sensors_list);
static DEFINE_MUTEX(sensors_mutex);

static int sensors_open(struct inode * inode, struct file * file)
{
	int minor = iminor(inode);
	struct sensors_dev *sdev;
	int err = -ENODEV;
	const struct file_operations *old_fops, *new_fops = NULL;

	printk("SENSOR : sensors_open is called!\n");
	mutex_lock(&sensors_mutex);
	
	list_for_each_entry(sdev, &sensors_list, list)
	{
		if(sdev->minor == minor)
		{
			new_fops = fops_get(sdev->fops);
			break;
		}
	}

	if(!new_fops)
	{
		mutex_unlock(&sensors_mutex);
		request_module("char-major-%d-%d", sensors_major, minor);
		mutex_lock(&sensors_mutex);

		list_for_each_entry(sdev, &sensors_list, list)
		{
			if(sdev->minor == minor)
			{
				new_fops = fops_get(sdev->fops);
				break;
			}
		}
		if(!new_fops)
			goto fail;
	}

	err = 0;
	old_fops = file->f_op;
	file->f_op = new_fops;
	if(file->f_op->open)
	{
		err = file->f_op->open(inode, file);
		if(err)
		{
			fops_put(file->f_op);
			file->f_op = fops_get(old_fops);
		}
	}
	fops_put(old_fops);

fail:
	mutex_unlock(&sensors_mutex);
	return err;
	return 0;
}

static const struct file_operations sensors_fops = {
	.owner = THIS_MODULE,
	.open = sensors_open,
};

void set_sensor_attr(struct device *child, struct device_attribute *sd, int array_size)
{
	int i;
	printk("[SENSOR CORE] set_sensor_attr() :  dev_addr=0x%x, array_size=%d \n", child, array_size);
		
	for (i=0 ; i< array_size ; i++)
	{
		if( ( device_create_file( child, &sd[i] )) < 0 )
		{
			printk("[SENSOR CORE] fail!!! device_create_file( child, &sd[%d] )\n", i);
		}
	}
}

int sensors_register(struct device *parent, struct device *child, struct device_attribute *sd, int array_size, struct sensors_dev *sdev)
{
	int ret = 0;
	dev_t dev;

	if(!sdev->fops)
	{
		printk(KERN_WARNING "Sensors register: No file operations!\n");
		return -EINVAL;
	}

	if(!sensors_class)
	{
		sensors_class = class_create(THIS_MODULE, "sensors");
		if(IS_ERR(sensors_class))
			return PTR_ERR(sensors_class);
	}

	INIT_LIST_HEAD(&sdev->list);
	mutex_lock(&sensors_mutex);

	sdev->minor = atomic_read(&sensor_count);
	dev = MKDEV(sensors_major, sdev->minor);	
	printk("[SENSORS CORE] sensors_register() sensor[%d], array_size=%d, sensors_major[%d] \n", sdev->minor, array_size, sensors_major);

	child = device_create(sensors_class, parent, dev, NULL, "sensor%d", sdev->minor);
	if (IS_ERR(child)) {
		ret = PTR_ERR(child);
        printk(KERN_ERR "[SENSORS CORE] device_create failed! [%d]\n", ret);
		return ret;
	}
	
	set_sensor_attr(child, sd, array_size);
	atomic_inc(&sensor_count);

	list_add(&sdev->list, &sensors_list);
	mutex_unlock(&sensors_mutex);

	return 0;
}

void sensors_unregister(struct sensors_dev *sdev)
{
	if(list_empty(&sdev->list))
		return;

	mutex_lock(&sensors_mutex);
	list_del(&sdev->list);
	mutex_unlock(&sensors_mutex);
}

static int __init sensors_class_init(void)
{
	if(!sensors_major)
	{
		sensors_major = register_chrdev(0, "sensors", &sensors_fops);
		if(sensors_major < 0)
		{
			printk(KERN_WARNING "Sensors: Could not get major number\n");
			return sensors_major;
		}
	}

	sensors_class = class_create(THIS_MODULE, "sensors");

	if (IS_ERR(sensors_class))
		return PTR_ERR(sensors_class);
	atomic_set(&sensor_count, 0);
	sensors_class->dev_uevent = NULL;

	return 0;
}

static void __exit sensors_class_exit(void)
{
	class_destroy(sensors_class);
}

EXPORT_SYMBOL_GPL(sensors_register);
EXPORT_SYMBOL_GPL(sensors_unregister);

/* exported for the APM Power driver, APM emulation */
EXPORT_SYMBOL_GPL(sensors_class);

subsys_initcall(sensors_class_init);
module_exit(sensors_class_exit);

MODULE_DESCRIPTION("Universal sensors core class");
MODULE_AUTHOR("Ryunkyun Park <ryun.park@samsung.com>");
MODULE_LICENSE("GPL");
