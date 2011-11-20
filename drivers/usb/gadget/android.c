/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>

#include <linux/usb/android_composite.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>

#include "u_serial.h"
#include "gadget_chips.h"

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static const char longname[] = "Gadget Android";

/* Default vendor and product IDs, overridden by platform data */
#define VENDOR_ID		0x18D1
#define PRODUCT_ID		0x0001

struct android_dev {
	struct usb_composite_dev *cdev;
	struct usb_configuration *config;
	int num_products;
	struct android_usb_product *products;
	int num_functions;
	char **functions;

	int vendor_id;
	int product_id;
	int version;
#ifdef CONFIG_MACH_OMAP_SAMSUNG
	int debugging_usb_mode;
	int ums_switch_mode;
#endif
};

static struct android_dev *_android_dev;

/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

/* String Table */
static struct usb_string strings_dev[] = {
	/* These dummy values should be overridden by platform data */
	[STRING_MANUFACTURER_IDX].s = "Android",
	[STRING_PRODUCT_IDX].s = "Android",
	[STRING_SERIAL_IDX].s = "0123456789ABCDEF",
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
#if defined(CONFIG_USB_IAD)
	.bDeviceClass         = USB_CLASS_MISC,
	.bDeviceSubClass      = 2,
	.bDeviceProtocol      = 1,
#else
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
#endif
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations   = 1,
};

static struct list_head _functions = LIST_HEAD_INIT(_functions);
static int _registered_function_count = 0;
#ifdef CONFIG_MACH_OMAP_SAMSUNG
#if defined(CONFIG_USB_IAD) && defined(CONFIG_USB_ANDROID_MASS_STORAGE)
static int switching_ums_mode = 0;
#endif
void get_usb_serial(char *usb_serial_number);
void get_usb_default_serial(char *usb_serial_number);
#endif

void android_usb_set_connected(int connected)
{
	if (_android_dev && _android_dev->cdev && _android_dev->cdev->gadget) {
		if (connected)
			usb_gadget_connect(_android_dev->cdev->gadget);
		else
			usb_gadget_disconnect(_android_dev->cdev->gadget);
	}
}

static struct android_usb_function *get_function(const char *name)
{
	struct android_usb_function	*f;
	list_for_each_entry(f, &_functions, list) {
		if (!strcmp(name, f->name))
			return f;
	}
	return 0;
}

static void bind_functions(struct android_dev *dev)
{
	struct android_usb_function	*f;
	char **functions = dev->functions;
	int i;

	for (i = 0; i < dev->num_functions; i++) {
		char *name = *functions++;
		f = get_function(name);
		if (f)
			f->bind_config(dev->config);
		else
			printk(KERN_ERR "function %s not found in bind_functions\n", name);
	}
}

static int android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;

	printk(KERN_DEBUG "android_bind_config\n");
	dev->config = c;

	/* bind our functions if they have all registered */
	if (_registered_function_count == dev->num_functions)
		bind_functions(dev);

	return 0;
}

static int android_setup_config(struct usb_configuration *c,
		const struct usb_ctrlrequest *ctrl);

static struct usb_configuration android_config_driver = {
	.label		= "android",
	.bind		= android_bind_config,
	.setup		= android_setup_config,
	.bConfigurationValue = 1,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
#ifdef CONFIG_MACH_OMAP_SAMSUNG
	.bMaxPower	= 0x30, /* x2 = 160ma */
#else
	.bMaxPower	= 0xFA, /* 500ma */
#endif
};

static int android_setup_config(struct usb_configuration *c,
		const struct usb_ctrlrequest *ctrl)
{
	int i;
	int ret = -EOPNOTSUPP;

	for (i = 0; i < android_config_driver.next_interface_id; i++) {
		if (android_config_driver.interface[i]->setup) {
			ret = android_config_driver.interface[i]->setup(
				android_config_driver.interface[i], ctrl);
			if (ret >= 0)
				return ret;
		}
	}
	return ret;
}

static int product_has_function(struct android_usb_product *p,
		struct usb_function *f)
{
	char **functions = p->functions;
	int count = p->num_functions;
	const char *name = f->name;
	int i;

	for (i = 0; i < count; i++) {
		if (!strcmp(name, *functions++))
			return 1;
	}
	return 0;
}

static int product_matches_functions(struct android_usb_product *p)
{
	struct usb_function		*f;
	list_for_each_entry(f, &android_config_driver.functions, list) {
		if (product_has_function(p, f) == !!f->disabled)
			return 0;
	}
	return 1;
}

static int get_vendor_id(struct android_dev *dev)
{
	struct android_usb_product *p = dev->products;
	int count = dev->num_products;
	int i;

	if (p) {
		for (i = 0; i < count; i++, p++) {
			if (p->vendor_id && product_matches_functions(p))
				return p->vendor_id;
		}
	}
	/* use default vendor ID */
	return dev->vendor_id;
}

static int get_product_id(struct android_dev *dev)
{
	struct android_usb_product *p = dev->products;
	int count = dev->num_products;
	int i;

	if (p) {
		for (i = 0; i < count; i++, p++) {
			if (product_matches_functions(p))
				return p->product_id;
		}
	}
	/* use default product ID */
	return dev->product_id;
}

static int android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget	*gadget = cdev->gadget;
	int			gcnum, id, ret;

	printk(KERN_INFO "android_bind\n");

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	device_desc.iSerialNumber = id;

#ifndef CONFIG_MACH_OMAP_SAMSUNG
	if (gadget->ops->wakeup)
		android_config_driver.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
#endif

	/* register our configuration */
	ret = usb_add_config(cdev, &android_config_driver);
	if (ret) {
		printk(KERN_ERR "usb_add_config failed\n");
		return ret;
	}

#if defined(CONFIG_MACH_OMAP_SAMSUNG)
	device_desc.bcdDevice = __constant_cpu_to_le16(0x0400); // support kies
#else
	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so just warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("%s: controller '%s' not recognized\n",
			longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}
#endif

	usb_gadget_set_selfpowered(gadget);
	dev->cdev = cdev;
	device_desc.idVendor = __constant_cpu_to_le16(get_vendor_id(dev));
	device_desc.idProduct = __constant_cpu_to_le16(get_product_id(dev));
	cdev->desc.idVendor = device_desc.idVendor;
	cdev->desc.idProduct = device_desc.idProduct;

	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.bind		= android_bind,
	.enable_function = android_enable_function,
};

void android_register_function(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;

	printk(KERN_INFO "android_register_function %s\n", f->name);
	list_add_tail(&f->list, &_functions);
	_registered_function_count++;

	/* bind our functions if they have all registered
	 * and the main driver has bound.
	 */
	if (dev && dev->config && _registered_function_count == dev->num_functions)
		bind_functions(dev);
}

void android_enable_function(struct usb_function *f, int enable)
{
	struct android_dev *dev = _android_dev;
	int disable = !enable;
	int product_id;

	if (!!f->disabled != disable) {
		usb_function_set_enabled(f, !disable);

#ifdef CONFIG_USB_ANDROID_RNDIS
		if (!strcmp(f->name, "rndis")) {
			struct usb_function		*func;

#if !defined(CONFIG_USB_IAD)
			/* We need to specify the COMM class in the device descriptor
			 * if we are using RNDIS.
			 */
			if (enable)
#ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
				dev->cdev->desc.bDeviceClass = USB_CLASS_WIRELESS_CONTROLLER;
#else
				dev->cdev->desc.bDeviceClass = USB_CLASS_COMM;
#endif
			else
				dev->cdev->desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
#endif

			/* Windows does not support other interfaces when RNDIS is enabled,
			 * so we disable UMS and MTP when RNDIS is on.
			 */
			list_for_each_entry(func, &android_config_driver.functions, list) {
#ifdef CONFIG_MACH_OMAP_SAMSUNG
#ifdef CONFIG_USB_ANDROID_ACM
				if (!strcmp(func->name, "acm")) {
					func->disabled = enable;
				}
#endif
#ifdef CONFIG_USB_ANDROID_EEM
				if (!strcmp(func->name, "cdc_eem")) {
					func->disabled = enable;
				}
#endif
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
				if (!strcmp(func->name, "usb_mass_storage")) {
					func->disabled = enable;
					usb_function_set_enabled(func, !enable);
				}
#endif
#ifdef CONFIG_USB_ANDROID_ADB
				if (!strcmp(func->name, "adb")) {
					func->disabled = (dev->debugging_usb_mode ? enable : 1);
				}
#endif
#else // original code
				if (!strcmp(func->name, "usb_mass_storage")) {
					func->disabled = enable;
					usb_function_set_enabled(func, !enable);
					break;
				}
#endif
			}

			if(enable) dev->cdev->mute_switch = 1;
		}
#endif

#ifdef CONFIG_USB_ANDROID_ACCESSORY
		if (!strcmp(f->name, "accessory")) {
			struct usb_function		*func;

			list_for_each_entry(func, &android_config_driver.functions, list) {
#ifdef CONFIG_USB_ANDROID_ACM
				if (!strcmp(func->name, "acm")) {
					func->disabled = enable;
				}
#endif
#ifdef CONFIG_USB_ANDROID_EEM
				if (!strcmp(func->name, "cdc_eem")) {
					func->disabled = enable;
				}
#endif
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
				if (!strcmp(func->name, "usb_mass_storage")) {
					func->disabled = enable;
					usb_function_set_enabled(func, !enable);
				}
#endif
#ifdef CONFIG_USB_ANDROID_ADB
				if (!strcmp(func->name, "adb")) {
					func->disabled = (dev->debugging_usb_mode ? enable : 1);
				}
#endif
			}

			if(enable) dev->cdev->mute_switch = 1;
		}
#endif

#ifdef CONFIG_MACH_OMAP_SAMSUNG
		if (!strcmp(f->name, "adb")) {
#if !defined(CONFIG_USB_IAD)
			struct usb_function		*func;
			char usb_serial_number[13] = {0,};

			if (enable)
				get_usb_default_serial(usb_serial_number);
			else
				get_usb_serial(usb_serial_number);

			strcpy((char *)(strings_dev[STRING_SERIAL_IDX].s), usb_serial_number);

			list_for_each_entry(func, &android_config_driver.functions, list) {
#ifdef CONFIG_USB_ANDROID_ACM
				if (!strcmp(func->name, "acm")) {
					func->disabled = !enable;
				}
#endif
#ifdef CONFIG_USB_ANDROID_EEM
				if (!strcmp(func->name, "cdc_eem")) {
					func->disabled = !enable;
				}
#endif
			}
#endif

			if(enable)
				dev->debugging_usb_mode = 1;
			else
				dev->debugging_usb_mode = 0;
		}

#if defined(CONFIG_USB_IAD) && defined(CONFIG_USB_ANDROID_MASS_STORAGE)
		if (!strcmp(f->name, "acm") && switching_ums_mode) { // for only UMS mode
			struct usb_function		*func;

			if (enable) {
				dev->cdev->desc.bDeviceClass = USB_CLASS_MISC;
				dev->cdev->desc.bDeviceSubClass = 2;
				dev->cdev->desc.bDeviceProtocol = 1;
			} else {
				dev->cdev->desc.bDeviceClass = USB_CLASS_MASS_STORAGE;
				dev->cdev->desc.bDeviceSubClass = 0x06;//US_SC_SCSI;
				dev->cdev->desc.bDeviceProtocol = 0x50;//US_PR_BULK;
			}

			list_for_each_entry(func, &android_config_driver.functions, list) {
#ifdef CONFIG_USB_ANDROID_ACM
				if (!strcmp(func->name, "acm")) {
					func->disabled = !enable;
				}
#endif
#ifdef CONFIG_USB_ANDROID_EEM
				if (!strcmp(func->name, "cdc_eem")) {
					func->disabled = !enable;
				}
#endif
#ifdef CONFIG_USB_ANDROID_ADB
				if (!strcmp(func->name, "adb")) {
					func->disabled = (dev->debugging_usb_mode ? !enable : 1);
				}
#endif
			}

			if(enable)
				dev->ums_switch_mode = 0;
			else
				dev->ums_switch_mode = 1;
		}
#endif /* defined(CONFIG_USB_IAD) && defined(CONFIG_USB_ANDROID_MASS_STORAGE) */
#endif /* CONFIG_MACH_OMAP_SAMSUNG */

		device_desc.idVendor = __constant_cpu_to_le16(get_vendor_id(dev));
		device_desc.idProduct = __constant_cpu_to_le16(get_product_id(dev));
		if (dev->cdev) {
			dev->cdev->desc.idVendor = device_desc.idVendor;
			dev->cdev->desc.idProduct = device_desc.idProduct;
		}

		/* force reenumeration */
		if (dev->cdev && dev->cdev->gadget &&
				dev->cdev->gadget->speed != USB_SPEED_UNKNOWN) {
			usb_gadget_disconnect(dev->cdev->gadget);
			msleep(10);
			usb_gadget_connect(dev->cdev->gadget);
		}
	}
}

#ifdef CONFIG_MACH_OMAP_SAMSUNG
#if defined(CONFIG_USB_IAD) && defined(CONFIG_USB_ANDROID_MASS_STORAGE)
/* sysfs for to show status of ums switch
 *                Path (/sys/devices/platform/android_usb/ums)
 */
static ssize_t ums_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int value = -1;

	if(_android_dev) {
		if (_android_dev->ums_switch_mode)
			value = 1;
		else
			value = 0;
	} else {
		printk(KERN_ERR "Fail to show ums switch.\n");
	}

	return sprintf(buf, "%d\n", value);
}

/* sysfs for to change status of ums switch
 *                Path (/sys/devices/platform/android_usb/ums)
 */
static ssize_t ums_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct usb_function *func;
	int value;

	sscanf(buf, "%d", &value);

	list_for_each_entry(func, &android_config_driver.functions, list) {
		if (!strcmp(func->name, "acm"))
			break;
	}

	if (value) {
		printk(KERN_DEBUG "Enable UMS\n");
		switching_ums_mode = 1;
		android_enable_function(func, 0);
		switching_ums_mode = 0;
	} else {
		printk(KERN_DEBUG "Disable UMS\n");
		switching_ums_mode = 1;
		android_enable_function(func, 1);
		switching_ums_mode = 0;
	}

	return size;
}

/* attribute of sysfs for ums mode switch */
static DEVICE_ATTR(ums, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,
		ums_switch_show, ums_switch_store);
#endif

/* soonyong.cho : sysfs for to show status of tethering switch
 *                Path (/sys/devices/platform/android_usb/tethering)
 */
static ssize_t tethering_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_function *func;
	int value = -1;

	list_for_each_entry(func, &android_config_driver.functions, list) {
		if (!strcmp(func->name, "rndis"))
			break;
	}

	if(dev) {
		if (func->disabled == 0)
			value = 1;
		else
			value = 0;
	} else {
		printk(KERN_ERR "Fail to show tethering switch.\n");
	}

	return sprintf(buf, "%d\n", value);
}

static DEVICE_ATTR(tethering, S_IRUGO | S_IRUSR, tethering_switch_show, NULL);

static ssize_t show_serial_num
(
	struct device *dev,
	struct device_attribute *attr,
	char *buf
)
{
	char usb_serial_number[13] = {0,};

	get_usb_serial(usb_serial_number);

	return snprintf(buf, PAGE_SIZE, "%s\n", usb_serial_number);
}

static DEVICE_ATTR(serial_num, S_IRUGO | S_IRUSR, show_serial_num, NULL);
#endif

static int android_probe(struct platform_device *pdev)
{
	struct android_usb_platform_data *pdata = pdev->dev.platform_data;
	struct android_dev *dev = _android_dev;

	printk(KERN_INFO "android_probe pdata: %p\n", pdata);

	if (pdata) {
		dev->products = pdata->products;
		dev->num_products = pdata->num_products;
		dev->functions = pdata->functions;
		dev->num_functions = pdata->num_functions;
		if (pdata->vendor_id) {
			dev->vendor_id = pdata->vendor_id;
			device_desc.idVendor =
				__constant_cpu_to_le16(pdata->vendor_id);
		}
		if (pdata->product_id) {
			dev->product_id = pdata->product_id;
			device_desc.idProduct =
				__constant_cpu_to_le16(pdata->product_id);
		}
		if (pdata->version)
			dev->version = pdata->version;

		if (pdata->product_name)
			strings_dev[STRING_PRODUCT_IDX].s = pdata->product_name;
		if (pdata->manufacturer_name)
			strings_dev[STRING_MANUFACTURER_IDX].s =
					pdata->manufacturer_name;
		if (pdata->serial_number)
			strings_dev[STRING_SERIAL_IDX].s = pdata->serial_number;
	}

#ifdef CONFIG_MACH_OMAP_SAMSUNG
	/* /sys/devices/platform/android_usb/serial_num */
	if (device_create_file(&pdev->dev, &dev_attr_serial_num) < 0) {
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_serial_num.attr.name);
	}

	if (device_create_file(&pdev->dev, &dev_attr_tethering) < 0) {
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_tethering.attr.name);
	}

#if defined(CONFIG_USB_IAD) && defined(CONFIG_USB_ANDROID_MASS_STORAGE)
	/* for UMS only mode /sys/devices/platform/android_usb/ums */
	if (device_create_file(&pdev->dev, &dev_attr_ums) < 0) {
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_ums.attr.name);
	}
#endif
#endif

	return usb_composite_register(&android_usb_driver);
}

#if defined(CONFIG_MACH_OMAP_SAMSUNG) && defined(CONFIG_SAMSUNG_MODEL_NAME)
void get_usb_serial(char *usb_serial_number)
{
	char temp_serial_number[13] = {0};
	u32 serial_number =0;
	u8 model_name[16];

	strncpy(model_name, CONFIG_SAMSUNG_MODEL_NAME, sizeof(model_name));
	model_name[8] = '\0';

	serial_number = omap_readl(0x4830A218) ^ omap_readl(0x4830A224); // use TI OMAP34XX CONTROL_DIE_ID

	sprintf(temp_serial_number,"%s%08x", (char*)&model_name[4], serial_number);

	strcpy(usb_serial_number,temp_serial_number);
}
EXPORT_SYMBOL(get_usb_serial);

void get_usb_default_serial(char *usb_serial_number)
{
	char temp_serial_number[13] = {0};
	u32 serial_number=0;
	u8 model_name[16];

	strncpy(model_name, CONFIG_SAMSUNG_MODEL_NAME, sizeof(model_name));
	model_name[8] = '\0';

	serial_number = 0x14909308; // modify serial number according to model

	sprintf(temp_serial_number,"%s%08x", (char*)&model_name[4], serial_number);

	strcpy(usb_serial_number,temp_serial_number);
}
EXPORT_SYMBOL(get_usb_default_serial);
#endif

static struct platform_driver android_platform_driver = {
	.driver = { .name = "android_usb", },
	.probe = android_probe,
};

static int __init init(void)
{
	struct android_dev *dev;

	printk(KERN_INFO "android init\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* set default values, which should be overridden by platform data */
	dev->product_id = PRODUCT_ID;
	_android_dev = dev;

	return platform_driver_register(&android_platform_driver);
}
module_init(init);

static void __exit cleanup(void)
{
	usb_composite_unregister(&android_usb_driver);
	platform_driver_unregister(&android_platform_driver);
	kfree(_android_dev);
	_android_dev = NULL;
}
module_exit(cleanup);
