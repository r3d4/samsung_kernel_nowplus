/*
 * linux/arch/arm/mach-omap2/board-3430ldp.c
 *
 * Copyright (C) 2008 Texas Instruments Inc.
 *
 * Modified from mach-omap2/board-3430sdp.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *****************************************************
 *****************************************************
 * modules/camera/m4mo_platform.c
 *
 * M4MO sensor driver file related to platform
 *
 * Modified by paladin in Samsung Electronics
 */
 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <plat/mux.h>
#include <media/v4l2-int-device.h>
#include "omap34xxcam.h"
#include "m4mo.h"
#include <../drivers/media/video/isp/ispreg.h>
#include "dprintk.h"

#define M4MO_BIGGEST_FRAME_BYTE_SIZE  PAGE_ALIGN(1280 * 720 * 2 *6) //fix for usage of 6 buffers for 720p capture and avoiding camera launch issues.

static struct v4l2_ifparm ifparm_m4mo = {
	//.capability = 1,
	.if_type = V4L2_IF_TYPE_BT656, 
	.u = {
		.bt656 = {
			.frame_start_on_rising_vs = 0,
			.latch_clk_inv = 0,
			.mode = V4L2_IF_TYPE_BT656_MODE_NOBT_8BIT, 
			.clock_min = M4MO_XCLK,
			.clock_max = M4MO_XCLK,
			.clock_curr = M4MO_XCLK,
		},
	},
};

static struct omap34xxcam_sensor_config m4mo_hwc = {
	.sensor_isp = 1,
	.xclk = OMAP34XXCAM_XCLK_A,
//    .capture_mem =  M4MO_BIGGEST_FRAME_BYTE_SIZE, 
};

struct isp_interface_config m4mo_if_config = {
	.ccdc_par_ser = ISP_PARLL,
	.dataline_shift = 0x2,
	.hsvs_syncdetect = ISPCTRL_SYNC_DETECT_VSFALL,
    .wait_hs_vs = 0x03,
	.strobe = 0x0,
	.prestrobe = 0x0,
	.shutter = 0x0,
	.u.par.par_bridge = 0x3,
	.u.par.par_clk_pol = 0x1,
};

static int m4mo_enable_gpio(void)
{
    if (gpio_request(OMAP_GPIO_CAMERA_LEVEL_CTRL,"CAM LEVEL CTRL") != 0) {
        dprintk(CAM_ERR, M4MO_MOD_NAME "Could not request GPIO %d",
            OMAP_GPIO_CAMERA_LEVEL_CTRL);
        return -EIO;
    }

    if (gpio_request(OMAP_GPIO_CAM_EN,"CAM EN") != 0) {
        dprintk(CAM_ERR, M4MO_MOD_NAME "Could not request GPIO %d",
            OMAP_GPIO_CAM_EN);
        return -EIO;
    }

    if (gpio_request(OMAP_GPIO_CAM_RST,"CAM RST") != 0) {
        dprintk(CAM_ERR, M4MO_MOD_NAME "Could not request GPIO %d",
            OMAP_GPIO_CAM_RST);
        return -EIO;
    }

    if (gpio_request(OMAP_GPIO_ISP_INT,"ISP INT") != 0) {
        dprintk(CAM_ERR, M4MO_MOD_NAME "Could not request GPIO %d",
            OMAP_GPIO_ISP_INT);
        return -EIO;
    }


    if (gpio_request(OMAP_GPIO_VGA_STBY,"VGA STDBY") != 0) {
        dprintk(CAM_ERR, M4MO_MOD_NAME "Could not request GPIO %d",
            OMAP_GPIO_VGA_STBY);
        return -EIO;
    }
    if (gpio_request(OMAP_GPIO_VGA_RST,"VGA CAM RST") != 0) {
        dprintk(CAM_ERR, M4MO_MOD_NAME "Could not request GPIO %d",
            OMAP_GPIO_VGA_RST);
        
        return -EIO;
    }

    /* Reset the GPIO pins */
    gpio_direction_output(OMAP_GPIO_CAM_EN, 0);
    gpio_direction_output(OMAP_GPIO_CAMERA_LEVEL_CTRL, 1);
    gpio_direction_output(OMAP_GPIO_CAM_RST, 0);
    isp_set_xclk(0,0);

    gpio_direction_output(OMAP_GPIO_VGA_RST, 0);
    gpio_direction_output(OMAP_GPIO_VGA_STBY, 0);
    mdelay(10);

    gpio_direction_output(OMAP_GPIO_CAMERA_LEVEL_CTRL, 0);
    mdelay(2);

    /* Enable sensor module power */
    gpio_direction_output(OMAP_GPIO_CAM_EN, 1);
    mdelay(10);

    /* Clock Enable */
    isp_set_xclk(M4MO_XCLK, 0);


    /* no get clk API available */
    #if 0
    clock = isp_get_xclk(0);
    dprintk(CAM_DBG, M4MO_MOD_NAME "xclka: %d\n", clock); 
    clock = isp_get_xclk(1);
    dprintk(CAM_DBG, M4MO_MOD_NAME "xclkb: %d\n", clock); 
    #endif

    mdelay(10);


    /* Activate Reset */
    gpio_direction_output(OMAP_GPIO_CAM_RST, 1);
    mdelay(10);
 
    return 0;
}

static int m4mo_disable_gpio(void)
{
#ifndef ZEUS_CAM
	int clock;
#endif
    gpio_direction_output(OMAP_GPIO_CAM_RST, 0);
    mdelay(2);

    isp_set_xclk(0, 0);
#ifndef ZEUS_CAM // commented for porting to zeus, as no get clk API available
    clock = isp_get_xclk(0);
    dprintk(CAM_DBG, M4MO_MOD_NAME "xclka: %d\n", clock); 
    clock = isp_get_xclk(1);
    dprintk(CAM_DBG, M4MO_MOD_NAME "xclkb: %d\n", clock);  
#endif
    mdelay(2);

    gpio_direction_output(OMAP_GPIO_CAMERA_LEVEL_CTRL, 1);
    mdelay(2);
    gpio_direction_output(OMAP_GPIO_CAM_EN, 0);
    mdelay(10);

    /* Power Down Sequence */
    gpio_free(OMAP_GPIO_CAMERA_LEVEL_CTRL);
    gpio_free(OMAP_GPIO_CAM_EN);
    gpio_free(OMAP_GPIO_CAM_RST);
    gpio_free(OMAP_GPIO_ISP_INT);
    gpio_free(OMAP_GPIO_VGA_STBY);
    gpio_free(OMAP_GPIO_VGA_RST);	
    
    return 0;	
}

static int m4mo_sensor_power_set(enum v4l2_power power)
{
	static enum v4l2_power previous_pwr = V4L2_POWER_OFF;
    int err = 0;
    
    printk(M4MO_MOD_NAME "m4mo_sensor_power_set is called...[%x] (0:OFF, 1:ON)\n", power);

	switch (power) {
        case V4L2_POWER_OFF:

            err = m4mo_disable_gpio();
            break;

        case V4L2_POWER_ON:

            isp_configure_interface(&m4mo_if_config);
            
            if (previous_pwr != V4L2_POWER_OFF) 
                m4mo_disable_gpio();
 
            err = m4mo_enable_gpio();
            break;

        case V4L2_POWER_STANDBY:
        case V4L2_POWER_RESUME:
            break;
	}
	previous_pwr = power;
    
	return err;
}


static int m4mo_ifparm(struct v4l2_ifparm *p)
{
	*p = ifparm_m4mo;
	return 0;
}


static int m4mo_sensor_set_prv_data(void *priv)
{
	struct omap34xxcam_hw_config *hwc = priv;

	hwc->u.sensor.xclk = m4mo_hwc.xclk;
	hwc->u.sensor.sensor_isp = m4mo_hwc.sensor_isp;
	hwc->dev_index = 0;
	hwc->dev_minor = 0;
	hwc->dev_type = OMAP34XXCAM_SLAVE_SENSOR;
	//hwc->interface_type = ISP_PARLL;

	return 0;
}

struct m4mo_platform_data z_m4mo_platform_data = {
	.power_set      = m4mo_sensor_power_set,
	.priv_data_set  = m4mo_sensor_set_prv_data,
	.ifparm         = m4mo_ifparm,
};

