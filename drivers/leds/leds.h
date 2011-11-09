/*
 * LED Core
 *
 * Copyright 2005 Openedhand Ltd.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef __LEDS_H_INCLUDED
#define __LEDS_H_INCLUDED

#include <linux/device.h>
#include <linux/rwsem.h>
#include <linux/leds.h>

#if 1 // Archer_LSJ DB10 
static inline void led_set_flashlight(struct led_classdev *led_cdev, enum flashlight_level level)
{
	if ((level < flash_off) || (level > flash_on)) 
    {   
        level = flash_off;
    }

	led_cdev->hand_flash = level;
    
    if (!(led_cdev->flags & LED_SUSPENDED))
    {
     	led_cdev->flashlight_set(led_cdev, level);
    }
}

static inline int led_get_flashlight(struct led_classdev *led_cdev)
{
	return led_cdev->hand_flash;
}
#endif 
#if 1 // Archer_LSJ DA25 
static inline void led_set_lcd_gamma(struct led_classdev *led_cdev, enum led_gamma value)
{
	if ((value < GAMMA_2_2) || (value > GAMMA_1_7)) 
    {   
        value = GAMMA_2_2;
    }
    
	led_cdev->lcd_gamma = value;

    if (!(led_cdev->flags & LED_SUSPENDED))
     	led_cdev->lcd_gamma_set(led_cdev, value);
}

static inline int led_get_lcd_gamma(struct led_classdev *led_cdev)
{
	return led_cdev->lcd_gamma;
}
#endif 
#ifdef SUPPORT_LCD_ACL_CTL 
static inline void led_set_lcd_ACL(struct led_classdev *led_cdev, enum led_ACL state)
{
    if (state != ACL_ON) 
    {   
        state = ACL_OFF;
    }
    
	led_cdev->acl_state = state;
    
    if (!(led_cdev->flags & LED_SUSPENDED))
     	led_cdev->lcd_ACL_set(led_cdev, state);
}

static inline int led_get_lcd_ACL(struct led_classdev *led_cdev)
{
	return led_cdev->acl_state;
}
#endif 
static inline void led_set_brightness(struct led_classdev *led_cdev,
					enum led_brightness value)
{
	if (value > led_cdev->max_brightness)
		value = led_cdev->max_brightness;
	led_cdev->brightness = value;
	if (!(led_cdev->flags & LED_SUSPENDED))
		led_cdev->brightness_set(led_cdev, value);
}

static inline int led_get_brightness(struct led_classdev *led_cdev)
{
	return led_cdev->brightness;
}

extern struct rw_semaphore leds_list_lock;
extern struct list_head leds_list;

#ifdef CONFIG_LEDS_TRIGGERS
void led_trigger_set_default(struct led_classdev *led_cdev);
void led_trigger_set(struct led_classdev *led_cdev,
			struct led_trigger *trigger);
void led_trigger_remove(struct led_classdev *led_cdev);
#else
#define led_trigger_set_default(x) do {} while (0)
#define led_trigger_set(x, y) do {} while (0)
#define led_trigger_remove(x) do {} while (0)
#endif

ssize_t led_trigger_store(struct device *dev, struct device_attribute *attr,
			const char *buf, size_t count);
ssize_t led_trigger_show(struct device *dev, struct device_attribute *attr,
			char *buf);

#endif	/* __LEDS_H_INCLUDED */
