/*
 * LED Class Core
 *
 * Copyright (C) 2005 John Lenz <lenz@cs.wisc.edu>
 * Copyright (C) 2005-2007 Richard Purdie <rpurdie@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/sysdev.h>
#include <linux/timer.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/leds.h>
#include "leds.h"

static struct class *leds_class;

#if 1 // Archer custom feature
static void led_update_flashlight(struct led_classdev *led_cdev) 
{
    if (led_cdev->flashlight_get)
    {
    	led_cdev->hand_flash = led_cdev->flashlight_get(led_cdev);
    }
}

static ssize_t  lcd_flashlight_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	/* no lock needed for this */
	led_update_flashlight(led_cdev);
	return sprintf(buf, "%u\n", led_cdev->hand_flash); 
}

static ssize_t  lcd_flashlight_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char    *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size) 
    {
		ret = count;
		led_set_flashlight(led_cdev, state);
	}
	return ret;
}
#endif 

#if 1 
static void led_update_lcd_gamma(struct led_classdev *led_cdev) // Archer_LSJ DA26
{
    if (led_cdev->lcd_gamma_get)
    {
    	led_cdev->lcd_gamma = led_cdev->lcd_gamma_get(led_cdev);
    }
}

static ssize_t  lcd_gamma_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	/* no lock needed for this */
	led_update_lcd_gamma(led_cdev);  // Archer_LSJ DA26

	return sprintf(buf, "%u\n", led_cdev->lcd_gamma); 
}

static ssize_t  lcd_gamma_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char    *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size) 
    {
		ret = count;

		if (state == LED_OFF)
        {
			led_trigger_remove(led_cdev);
        }
		led_set_lcd_gamma(led_cdev, state);
	}
	return ret;
}
#endif 

#ifdef SUPPORT_LCD_ACL_CTL
static void lcd_update_ACL_state(struct led_classdev *led_cdev)
{
	if (led_cdev->lcd_ACL_get)
	{
		led_cdev->acl_state = led_cdev->lcd_ACL_get(led_cdev);
	}
}

static ssize_t acl_state_show(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	/* no lock needed for this */
	lcd_update_ACL_state(led_cdev);

	return sprintf(buf, "%u\n", led_cdev->acl_state);
}

static ssize_t acl_state_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char    *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size) 
	{
		ret = count;
		led_set_lcd_ACL(led_cdev, state);
	}

	return ret;
}
#endif

static void led_update_brightness(struct led_classdev *led_cdev)
{
	if (led_cdev->brightness_get)
		led_cdev->brightness = led_cdev->brightness_get(led_cdev);
}

static ssize_t led_brightness_show(struct device *dev, 
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	/* no lock needed for this */
	led_update_brightness(led_cdev);

	return sprintf(buf, "%u\n", led_cdev->brightness);
}

static ssize_t led_brightness_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	ssize_t ret = -EINVAL;
	char *after;
	unsigned long state = simple_strtoul(buf, &after, 10);
	size_t count = after - buf;

	if (*after && isspace(*after))
		count++;

	if (count == size) {
		ret = count;

		if (state == LED_OFF)
			led_trigger_remove(led_cdev);
		led_set_brightness(led_cdev, state);
	}

	return ret;
}

static ssize_t led_max_brightness_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", led_cdev->max_brightness);
}

static DEVICE_ATTR(hand_flash, 0666, lcd_flashlight_show, lcd_flashlight_store);  // Archer_LSJ DB10
static DEVICE_ATTR(lcd_gamma,  0666, lcd_gamma_show,      lcd_gamma_store);  // Archer_LSJ DA25
#ifdef SUPPORT_LCD_ACL_CTL
static DEVICE_ATTR(acl_state,  0666, acl_state_show,      acl_state_store);	// ACL On/Off sysfs
#endif
static DEVICE_ATTR(brightness, 0644, led_brightness_show, led_brightness_store);
static DEVICE_ATTR(max_brightness, 0444, led_max_brightness_show, NULL);
#ifdef CONFIG_LEDS_TRIGGERS
static DEVICE_ATTR(trigger, 0644, led_trigger_show, led_trigger_store);
#endif

/**
 * led_classdev_suspend - suspend an led_classdev.
 * @led_cdev: the led_classdev to suspend.
 */
void led_classdev_suspend(struct led_classdev *led_cdev)
{
	led_cdev->flags |= LED_SUSPENDED;
	led_cdev->brightness_set(led_cdev, 0);
}
EXPORT_SYMBOL_GPL(led_classdev_suspend);

/**
 * led_classdev_resume - resume an led_classdev.
 * @led_cdev: the led_classdev to resume.
 */
void led_classdev_resume(struct led_classdev *led_cdev)
{
	led_cdev->brightness_set(led_cdev, led_cdev->brightness);
	led_cdev->flags &= ~LED_SUSPENDED;
}
EXPORT_SYMBOL_GPL(led_classdev_resume);

static int led_suspend(struct device *dev, pm_message_t state)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (led_cdev->flags & LED_CORE_SUSPENDRESUME)
		led_classdev_suspend(led_cdev);

	return 0;
}

static int led_resume(struct device *dev)
{
	struct led_classdev *led_cdev = dev_get_drvdata(dev);

	if (led_cdev->flags & LED_CORE_SUSPENDRESUME)
		led_classdev_resume(led_cdev);

	return 0;
}

/**
 * led_classdev_register - register a new object of led_classdev class.
 * @parent: The device to register.
 * @led_cdev: the led_classdev structure for this device.
 */
int led_classdev_register(struct device *parent, struct led_classdev *led_cdev)
{
	int rc;

	led_cdev->dev = device_create(leds_class, parent, 0, led_cdev,
				      "%s", led_cdev->name);
	if (IS_ERR(led_cdev->dev))
		return PTR_ERR(led_cdev->dev);

	/* register the attributes */
	rc = device_create_file(led_cdev->dev, &dev_attr_brightness);
	if (rc)
		goto err_out;

#ifdef CONFIG_LEDS_TRIGGERS
	init_rwsem(&led_cdev->trigger_lock);
#endif
	/* add to the list of leds */
	down_write(&leds_list_lock);
	list_add_tail(&led_cdev->node, &leds_list);
	up_write(&leds_list_lock);

	if (!led_cdev->max_brightness)
		led_cdev->max_brightness = LED_FULL;

	rc = device_create_file(led_cdev->dev, &dev_attr_max_brightness);
	if (rc)
		goto err_out_attr_max;

	led_update_brightness(led_cdev);

#ifdef CONFIG_LEDS_TRIGGERS
	rc = device_create_file(led_cdev->dev, &dev_attr_trigger);
	if (rc)
		goto err_out_led_list;

	led_trigger_set_default(led_cdev);
#endif

    #if 1  // Archer custom feature
	/* register the attributes */
	rc = device_create_file(led_cdev->dev, &dev_attr_lcd_gamma);
	if (rc)
		goto err_out;
    
	led_update_lcd_gamma(led_cdev);


#ifdef SUPPORT_LCD_ACL_CTL
	/* register the attributes */
	rc = device_create_file(led_cdev->dev, &dev_attr_acl_state);
	if (rc)
		goto err_out;
    
	lcd_update_ACL_state(led_cdev);
#endif

	rc = device_create_file(led_cdev->dev, &dev_attr_hand_flash);
	if (rc)
		goto err_out;
    
	led_update_flashlight(led_cdev);
    #endif 
	printk(KERN_INFO "Registered led device: %s\n",	led_cdev->name);

	return 0;

#ifdef CONFIG_LEDS_TRIGGERS
err_out_led_list:
	device_remove_file(led_cdev->dev, &dev_attr_max_brightness);

#endif
err_out_attr_max:
	device_remove_file(led_cdev->dev, &dev_attr_brightness);
	device_remove_file(led_cdev->dev, &dev_attr_lcd_gamma);
#ifdef SUPPORT_LCD_ACL_CTL	
	device_remove_file(led_cdev->dev, &dev_attr_acl_state);
#endif	
	list_del(&led_cdev->node);
err_out:
	device_unregister(led_cdev->dev);
	return rc;
}
EXPORT_SYMBOL_GPL(led_classdev_register);

/**
 * led_classdev_unregister - unregisters a object of led_properties class.
 * @led_cdev: the led device to unregister
 *
 * Unregisters a previously registered via led_classdev_register object.
 */
void led_classdev_unregister(struct led_classdev *led_cdev)
{
	device_remove_file(led_cdev->dev, &dev_attr_max_brightness);
	device_remove_file(led_cdev->dev, &dev_attr_brightness);
	device_remove_file(led_cdev->dev, &dev_attr_lcd_gamma);
#ifdef SUPPORT_LCD_ACL_CTL	
	device_remove_file(led_cdev->dev, &dev_attr_acl_state);
#endif
#ifdef CONFIG_LEDS_TRIGGERS
	device_remove_file(led_cdev->dev, &dev_attr_trigger);
	down_write(&led_cdev->trigger_lock);
	if (led_cdev->trigger)
		led_trigger_set(led_cdev, NULL);
	up_write(&led_cdev->trigger_lock);
#endif

	device_unregister(led_cdev->dev);

	down_write(&leds_list_lock);
	list_del(&led_cdev->node);
	up_write(&leds_list_lock);
}
EXPORT_SYMBOL_GPL(led_classdev_unregister);

static int __init leds_init(void)
{
	leds_class = class_create(THIS_MODULE, "leds");
	if (IS_ERR(leds_class))
		return PTR_ERR(leds_class);
	leds_class->suspend = led_suspend;
	leds_class->resume = led_resume;
	return 0;
}

static void __exit leds_exit(void)
{
	class_destroy(leds_class);
}

subsys_initcall(leds_init);
module_exit(leds_exit);

MODULE_AUTHOR("John Lenz, Richard Purdie");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("LED Class Interface");
