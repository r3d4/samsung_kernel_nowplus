/*
 * Board support file for containing WiFi specific details for OMAP4430 SDP.
 *
 * Copyright (C) 2009 Texas Instruments
 *
 * Author: Pradeep Gurumath <pradeepgurumath@ti.com>
 *
 * Based on mach-omap2/board-3430sdp.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* linux/arch/arm/mach-omap2/board-4430sdp-wifi.c
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_ids.h>
#include <linux/err.h>

#include <asm/gpio.h>
#include <asm/io.h>
#include <linux/wifi_tiwlan.h>
#include <plat/mux_nowplus.h>


static int nowplus_wifi_cd;		/* WIFI virtual 'card detect' status */
static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

int omap_wifi_status_register(void (*callback)(int card_present,
						void *dev_id), void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;

	wifi_status_cb_devid = dev_id;

	return 0;
}

int omap_wifi_status(struct device *dev, int slot)
{
	return nowplus_wifi_cd;
}

int nowplus_wifi_set_carddetect(int val)
{
	printk(KERN_WARNING"%s: %d\n", __func__, val);
	nowplus_wifi_cd = val;
	if (wifi_status_cb)
		wifi_status_cb(val, wifi_status_cb_devid);
	else
		printk(KERN_WARNING "%s: Nobody to notify\n", __func__);
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(nowplus_wifi_set_carddetect);
#endif

static int nowplus_wifi_power_state;

int nowplus_wifi_power(int on)
{
	printk(KERN_WARNING"%s: %d\n", __func__, on);
	gpio_set_value(OMAP_GPIO_WLAN_EN, on);
	nowplus_wifi_power_state = on;
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(nowplus_wifi_power);
#endif

static int nowplus_wifi_reset_state;
int nowplus_wifi_reset(int on)
{
	printk(KERN_WARNING"%s: %d\n", __func__, on);
	nowplus_wifi_reset_state = on;
	return 0;
}
#ifndef CONFIG_WIFI_CONTROL_FUNC
EXPORT_SYMBOL(nowplus_wifi_reset);
#endif

struct wifi_platform_data nowplus_wifi_control = {
	.set_power	= nowplus_wifi_power,
	.set_reset	= nowplus_wifi_reset,
	.set_carddetect	= nowplus_wifi_set_carddetect,
};

#ifdef CONFIG_WIFI_CONTROL_FUNC
struct resource nowplus_wifi_resources[] = {
	[0] = {
		.name		= "device_wifi_irq",
		.start		= OMAP_GPIO_IRQ(OMAP_GPIO_WLAN_IRQ),
		.end		= OMAP_GPIO_IRQ(OMAP_GPIO_WLAN_IRQ),
		.flags          = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE,
	},
};

struct platform_device nowplus_wifi_device = {
	.name           = "device_wifi",
	.id             = 1,
	.num_resources  = ARRAY_SIZE(nowplus_wifi_resources),
	.resource       = nowplus_wifi_resources,
	.dev            = {
		.platform_data = &nowplus_wifi_control,
	},
};
#endif

static int __init nowplus_wifi_init(void)
{
	int ret;

	printk(KERN_WARNING"%s: start\n", __func__);
    
#if	1//TI	HS.Yoon	20100827	for	enabling	WLAN_IRQ	wakeup
	omap_writel(omap_readl(0x480025E8)|0x410C0000,	0x480025E8);
	omap_writew(0x10C,	0x48002194);
#endif

  	ret = gpio_request(OMAP_GPIO_WLAN_EN, "wifi_irq");
	if (ret < 0) {
		printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
			OMAP_GPIO_WLAN_EN);
		goto out;
	}  
    
	ret = gpio_request(OMAP_GPIO_WLAN_IRQ, "wifi_irq");
	if (ret < 0) {
		printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
			OMAP_GPIO_WLAN_IRQ);
		goto out;
	}
	gpio_direction_input(OMAP_GPIO_WLAN_IRQ);
    gpio_direction_output(OMAP_GPIO_WLAN_EN, 0);
#ifdef CONFIG_WIFI_CONTROL_FUNC
	ret = platform_device_register(&nowplus_wifi_device);
#endif
out:
	return ret;
}

//device_initcall(nowplus_wifi_init);
