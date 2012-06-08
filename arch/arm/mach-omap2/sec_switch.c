/*
 * UART/USB path switching driver for Samsung Electronics devices.
 *
 *  Copyright (C) 2010 Samsung Electronics
 *  Ikkeun Kim <iks.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <mach/param.h>
#if defined(CONFIG_USB_SWITCH_FSA9480)
#include <linux/fsa9480.h>
#endif
#include <asm/mach/arch.h>
#include <linux/regulator/consumer.h>
#include <mach/gpio.h>
#include <mach/sec_switch.h>
#include <linux/moduleparam.h>
#include <plat/mux.h>


#if defined(CONFIG_USB_SWITCH_FSA9480)
extern void twl4030_phy_enable(void);
extern void twl4030_phy_disable(void);
#endif

#ifdef CONFIG_ARCH_OMAP
#ifndef GPIO_UART_SEL
#define GPIO_UART_SEL	OMAP_GPIO_UART_SEL
#endif
#ifndef GPIO_USB_SEL
#define GPIO_USB_SEL	OMAP_GPIO_USB_SEL
#endif
#endif

struct sec_switch_struct {
	struct sec_switch_platform_data *pdata;
	int switch_sel;
	int uart_owner;
};

struct sec_switch_wq {
	struct delayed_work work_q;
	struct sec_switch_struct *sdata;
	struct list_head entry;
};


static int sec_switch_started;

extern struct device *switch_dev;
static int switchsel = USB_SEL_MASK | UART_SEL_MASK;
// Get SWITCH_SEL param value from kernel CMDLINE parameter.
__module_param_call("", switchsel, param_set_int, param_get_int, &switchsel, 0, 0444);
MODULE_PARM_DESC(switchsel, "Switch select parameter value.");


static void usb_switch_mode(struct sec_switch_struct *secsw, int mode)
{
	if(mode == SWITCH_PDA)
	{
		gpio_set_value(GPIO_USB_SEL, 0);

		if(secsw->pdata && secsw->pdata->set_regulator)
			secsw->pdata->set_regulator(AP_VBUS_ON);
		mdelay(10);

#if defined(CONFIG_USB_SWITCH_FSA9480)
		fsa9480_manual_switching(AUTO_SWITCH);
#endif
	}
	else  // SWITCH_MODEM
	{
		gpio_set_value(GPIO_USB_SEL, 1);

		if(secsw->pdata && secsw->pdata->set_regulator)
			secsw->pdata->set_regulator(CP_VBUS_ON);
		mdelay(10);

#if defined(CONFIG_USB_SWITCH_FSA9480)
		fsa9480_manual_switching(SWITCH_V_Audio_Port);
#endif
	}
}

/* for sysfs control (/sys/class/sec/switch/usb_sel) */
static ssize_t usb_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_switch_struct *secsw = dev_get_drvdata(dev);
	int usb_path = secsw->switch_sel & (int)(USB_SEL_MASK);

	return sprintf(buf, "USB Switch : %s\n", usb_path==SWITCH_PDA?"PDA":"MODEM");
}

static ssize_t usb_sel_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct sec_switch_struct *secsw = dev_get_drvdata(dev);

	printk("\n");

	// if (sec_get_param_value)
		// sec_get_param_value(__SWITCH_SEL, &(secsw->switch_sel));

	if(strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0) {
		usb_switch_mode(secsw, SWITCH_PDA);
		secsw->switch_sel |= USB_SEL_MASK;
	}

	if(strncmp(buf, "MODEM", 5) == 0 || strncmp(buf, "modem", 5) == 0) {
		usb_switch_mode(secsw, SWITCH_MODEM);
		secsw->switch_sel &= ~USB_SEL_MASK;
	}

	// if (sec_set_param_value)
		// sec_set_param_value(__SWITCH_SEL, &(secsw->switch_sel));

	// update shared variable.
	if(secsw->pdata && secsw->pdata->set_switch_status)
		secsw->pdata->set_switch_status(secsw->switch_sel);

	return size;
}

static DEVICE_ATTR(usb_sel, 0664, usb_sel_show, usb_sel_store);


static ssize_t uart_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_switch_struct *secsw = dev_get_drvdata(dev);

	if (secsw->uart_owner)
		return sprintf(buf, "[UART Switch] Current UART owner = PDA \n");
	else			
		return sprintf(buf, "[UART Switch] Current UART owner = MODEM \n");
}

static ssize_t uart_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{	
	struct sec_switch_struct *secsw = dev_get_drvdata(dev);
	int console_mode;
	
	// if (sec_get_param_value) {
		// sec_get_param_value(__SWITCH_SEL, &(secsw->switch_sel));
		// sec_get_param_value(__CONSOLE_MODE, &console_mode);
	// }

	if (strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0) {
		gpio_set_value(GPIO_UART_SEL, 0);
		secsw->uart_owner = 1;
		secsw->switch_sel |= UART_SEL_MASK;
		console_mode = 1;
		printk("[UART Switch] Path : PDA\n");	
	}	

	if (strncmp(buf, "MODEM", 5) == 0 || strncmp(buf, "modem", 5) == 0) {
		gpio_set_value(GPIO_UART_SEL, 1);
		secsw->uart_owner = 0;
		secsw->switch_sel &= ~UART_SEL_MASK;
		console_mode = 0;
		printk("[UART Switch] Path : MODEM\n");	
	}

	// if (sec_set_param_value) {
		// sec_set_param_value(__SWITCH_SEL, &(secsw->switch_sel));
		// sec_set_param_value(__CONSOLE_MODE, &console_mode);
	// }

	// update shared variable.
	if(secsw->pdata && secsw->pdata->set_switch_status)
		secsw->pdata->set_switch_status(secsw->switch_sel);

	return size;
}

static DEVICE_ATTR(uart_sel, 0664, uart_switch_show, uart_switch_store);


// for sysfs control (/sys/class/sec/switch/usb_state)
static ssize_t usb_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_switch_struct *secsw = dev_get_drvdata(dev);
	int cable_state = CABLE_TYPE_NONE;

	if(secsw->pdata && secsw->pdata->get_cable_status)
		cable_state = secsw->pdata->get_cable_status();

	return sprintf(buf, "%s\n", (cable_state==CABLE_TYPE_USB)?"USB_STATE_CONFIGURED":"USB_STATE_NOTCONFIGURED");
} 

static ssize_t usb_state_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	printk("\n");
	return size;
}

static DEVICE_ATTR(usb_state, 0664, usb_state_show, usb_state_store);


// for sysfs control (/sys/class/sec/switch/disable_vbus)
static ssize_t disable_vbus_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("\n");
	return 0;
} 

static ssize_t disable_vbus_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct sec_switch_struct *secsw = dev_get_drvdata(dev);
	printk("%s\n", __func__);
	if(secsw->pdata && secsw->pdata->set_regulator)
		secsw->pdata->set_regulator(AP_VBUS_OFF);

	return size;
}

static DEVICE_ATTR(disable_vbus, 0664, disable_vbus_show, disable_vbus_store);


static void sec_switch_init_work(struct work_struct *work)
{
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	struct sec_switch_wq *wq = container_of(dw, struct sec_switch_wq, work_q);
	struct sec_switch_struct *secsw = wq->sdata;
	int usb_sel = 0;
	int uart_sel = 0;
	int ret = 0;
#if 0
    if (sec_get_param_value) {
        sec_get_param_value(__SWITCH_SEL, &switchsel);
        secsw->switch_sel = switchsel;
        cancel_delayed_work(&wq->work_q);
    } else {
        if(!sec_switch_started) {
            sec_switch_started = 1;
            schedule_delayed_work(&wq->work_q, msecs_to_jiffies(3000));
        } else {
            schedule_delayed_work(&wq->work_q, msecs_to_jiffies(100));
        }
        return;
    }
#else
	 secsw->switch_sel = switchsel;
	 cancel_delayed_work(&wq->work_q);
#endif
	if(secsw->pdata && secsw->pdata->get_regulator) {
		ret = secsw->pdata->get_regulator();
		if(ret != 0) {
			pr_err("%s : failed to get regulators\n", __func__);
			return ;
		}
	}

	// init shared variable.
	if(secsw->pdata && secsw->pdata->set_switch_status)
		secsw->pdata->set_switch_status(secsw->switch_sel);

	usb_sel = secsw->switch_sel & (int)(USB_SEL_MASK);
	uart_sel = (secsw->switch_sel & (int)(UART_SEL_MASK)) >> 1;

	printk("%s : initial usb_sel(%d), uart_sel(%d)\n", __func__, usb_sel, uart_sel);

	// init UART/USB path.
	if(usb_sel) {
		usb_switch_mode(secsw, SWITCH_PDA);
	} else {
		usb_switch_mode(secsw, SWITCH_MODEM);
	}

	if(uart_sel) {
		gpio_set_value(GPIO_UART_SEL, 0);
		secsw->uart_owner = 1;
	} else {
		gpio_set_value(GPIO_UART_SEL, 1);
		secsw->uart_owner = 0;
	}
}

static int sec_switch_probe(struct platform_device *pdev)
{
	struct sec_switch_struct *secsw;
	struct sec_switch_platform_data *pdata = pdev->dev.platform_data;
	struct sec_switch_wq *wq;

	if (!pdata) {
		pr_err("%s : pdata is NULL.\n", __func__);
		return -ENODEV;
	}

	secsw = kzalloc(sizeof(struct sec_switch_struct), GFP_KERNEL);
	if (!secsw) {
		pr_err("%s : failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	printk("%s : *** switch_sel (0x%x)\n", __func__, switchsel);

	secsw->pdata = pdata;
	secsw->switch_sel = switchsel;

	dev_set_drvdata(switch_dev, secsw);

	if (gpio_request(GPIO_UART_SEL, "UART_SEL"))
	{
		printk(KERN_ERR "Filed to request OMAP_GPIO_UART_SEL!\n");
	}
	gpio_direction_output(GPIO_UART_SEL, 0);

	if (gpio_request(GPIO_USB_SEL, "USB_SEL"))
	{
		printk(KERN_ERR "Filed to request OMAP_GPIO_USB_SEL!\n");
	}
	gpio_direction_output(GPIO_USB_SEL, 0);

	// create sysfs files.
	if (device_create_file(switch_dev, &dev_attr_uart_sel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_uart_sel.attr.name);

	if (device_create_file(switch_dev, &dev_attr_usb_sel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_usb_sel.attr.name);

	if (device_create_file(switch_dev, &dev_attr_usb_state) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_usb_state.attr.name);

	if (device_create_file(switch_dev, &dev_attr_disable_vbus) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_disable_vbus.attr.name);

	// run work queue
	wq = kmalloc(sizeof(struct sec_switch_wq), GFP_ATOMIC);
	if (wq) {
		wq->sdata = secsw;
		sec_switch_started = 0;
		INIT_DELAYED_WORK(&wq->work_q, sec_switch_init_work);
		schedule_delayed_work(&wq->work_q, msecs_to_jiffies(3000));
	}
	else
		return -ENOMEM;

	return 0;
}

static int sec_switch_remove(struct platform_device *pdev)
{
	struct sec_switch_struct *secsw = dev_get_drvdata(&pdev->dev);

	kfree(secsw);

	return 0;
}

static struct platform_driver sec_switch_driver = {
	.probe = sec_switch_probe,
	.remove = sec_switch_remove,
	.driver = {
			.name = "sec_switch",
			.owner = THIS_MODULE,
	},
};

static int __init sec_switch_init(void)
{
	return platform_driver_register(&sec_switch_driver);
}

static void __exit sec_switch_exit(void)
{
	platform_driver_unregister(&sec_switch_driver);
}

module_init(sec_switch_init);
module_exit(sec_switch_exit);

MODULE_AUTHOR("Ikkeun Kim <iks.kim@samsung.com>");
MODULE_DESCRIPTION("Samsung Electronics Corp Switch driver");
MODULE_LICENSE("GPL");
