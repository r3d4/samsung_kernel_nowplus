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
 * modules/camera/s5ka3dfx_platform.c
 *
 * S5KA3DFX sensor driver file related to platform
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
#include <../drivers/media/video/isp/ispreg.h>
#include "s5ka3dfx.h"
#include "cam_pmic.h"

#if (CAM_S5KA3DFX_DBG_MSG)
#define	dprintk(x, y...)	\
	do{\
	printk("["#x"] "y);\
	}while(0)
#else
#define dprintk(x, y...)
#endif

#ifdef CONFIG_MACH_HALO
#include <mach/control.h>
static u16 control_pbias_offset;
#endif

static struct v4l2_ifparm ifparm_s5ka3dfx = {
  .if_type = V4L2_IF_TYPE_BT656, // fix me
  .u = {
    .bt656 = {
      .frame_start_on_rising_vs = 0,
      .latch_clk_inv = 0,
      .mode = V4L2_IF_TYPE_BT656_MODE_NOBT_8BIT, 
      .clock_min = S5KA3DFX_XCLK,
      .clock_max = S5KA3DFX_XCLK,
      .clock_curr = S5KA3DFX_XCLK,
    },
  },
};

static struct omap34xxcam_sensor_config s5ka3dfx_hwc = {
  .sensor_isp = 1,
  .xclk = OMAP34XXCAM_XCLK_A,
};

struct isp_interface_config s5ka3dfx_if_config = {
  .ccdc_par_ser = ISP_PARLL,
  .dataline_shift = 0x2,
  .hsvs_syncdetect = ISPCTRL_SYNC_DETECT_VSFALL,
  .wait_hs_vs = 0x03,
  .strobe = 0x0,
  .prestrobe = 0x0,
  .shutter = 0x0,
  .u.par.par_bridge = 0x3,
  .u.par.par_clk_pol = 0x0,
};

static int s5ka3dfx_enable_gpio(void)
{
  dprintk(CAM_INF, S5KA3DFX_MOD_NAME "s5ka3dfx_enable_gpio is called...\n");


  /* Request and configure gpio pins */
  if (gpio_request(OMAP_GPIO_CAM_EN,"CAM EN") != 0) 
  {
    printk(S5KA3DFX_MOD_NAME "Could not request GPIO %d\n", OMAP_GPIO_CAM_EN);
    return -EIO;
  }
  
  if (gpio_request(OMAP_GPIO_VGA_STBY,"VGA STDBY") != 0)
  {
    printk(S5KA3DFX_MOD_NAME "Could not request GPIO %d", OMAP_GPIO_VGA_STBY);
    return -EIO;
  }

  if (gpio_request(OMAP_GPIO_VGA_RST,"CAM VGA RST") != 0) 
  {
    printk(S5KA3DFX_MOD_NAME "Could not request GPIO %d", OMAP_GPIO_VGA_RST);
    return -EIO;
  } 
  
  if (gpio_request(OMAP_GPIO_CAM_RST,"CAMERA_RST EN") != 0) 
  {
    printk(S5KA3DFX_MOD_NAME "Could not request GPIO %d\n", OMAP_GPIO_CAM_RST);
    return -EIO;
  }   
    

  if (gpio_request(OMAP_GPIO_CAMERA_LEVEL_CTRL,"OMAP_GPIO_CAMERA_LEVEL_CTRL") != 0) 
  {
    printk(S5KA3DFX_MOD_NAME "Could not request GPIO %d\n", OMAP_GPIO_CAMERA_LEVEL_CTRL);
    return -EIO;
  }  

  /* Reset the GPIO pins */
  gpio_direction_output(OMAP_GPIO_CAM_EN, 0);
  gpio_direction_output(OMAP_GPIO_VGA_STBY, 0);        
  gpio_direction_output(OMAP_GPIO_VGA_RST, 0);    

  isp_set_xclk(0,0);
  
  /* Enable sensor module power */     
  gpio_direction_output(OMAP_GPIO_CAM_RST, 0);
  gpio_direction_output(OMAP_GPIO_CAMERA_LEVEL_CTRL, 1);
  mdelay(10);

  gpio_direction_output(OMAP_GPIO_CAM_EN, 1);
  mdelay(10);

  gpio_direction_output(OMAP_GPIO_VGA_STBY, 1);
  mdelay(10);

  /* Clock Enable */
  isp_set_xclk(S5KA3DFX_XCLK, 0);
  mdelay(10); 

  /* Activate Reset */
  gpio_direction_output(OMAP_GPIO_VGA_RST, 1);
  mdelay(10);

  return 0; 
}

static int s5ka3dfx_disable_gpio(void)
{
  dprintk(CAM_INF, S5KA3DFX_MOD_NAME "s5ka3dfx_disable_gpio is called...\n");

  gpio_direction_output(OMAP_GPIO_VGA_RST, 0);
  mdelay(2);

  isp_set_xclk(0, 0);
  mdelay(2);

  gpio_direction_output(OMAP_GPIO_VGA_STBY, 0);
  mdelay(2);         

  gpio_direction_output(OMAP_GPIO_CAM_EN, 0);
  mdelay(10);

  gpio_free(OMAP_GPIO_CAM_EN); 
  gpio_free(OMAP_GPIO_VGA_STBY);
  gpio_free(OMAP_GPIO_VGA_RST);

  gpio_free(OMAP_GPIO_CAM_RST); 
  gpio_free(OMAP_GPIO_CAMERA_LEVEL_CTRL);
  

  return 0;
}

static int s5ka3dfx_sensor_power_set(enum v4l2_power power)
{
  static enum v4l2_power s_previous_pwr = V4L2_POWER_OFF;

  int err = 0;

  printk(S5KA3DFX_MOD_NAME "s5ka3dfx_sensor_power_set is called...[%x] (0:OFF, 1:ON)\n", power);

  switch (power) 
  {
    case V4L2_POWER_OFF:
    {
      err = s5ka3dfx_disable_gpio();
    }      
    break;

    case V4L2_POWER_ON:
    {
      isp_configure_interface(&s5ka3dfx_if_config);


      err = s5ka3dfx_enable_gpio();       
    }
    break;

    case V4L2_POWER_STANDBY:
      break;

    case V4L2_POWER_RESUME:
      isp_configure_interface(&s5ka3dfx_if_config);
      break;
  }
  
  s_previous_pwr = power;

  return err;
}


static int s5ka3dfx_ifparm(struct v4l2_ifparm *p)
{
  dprintk(CAM_INF, S5KA3DFX_MOD_NAME "s5ka3dfx_ifparm is called...\n");
  
  *p = ifparm_s5ka3dfx;
  
  return 0;
}


static int s5ka3dfx_sensor_set_prv_data(void *priv)
{
  struct omap34xxcam_hw_config *hwc = priv;

  dprintk(CAM_INF, S5KA3DFX_MOD_NAME "s5ka3dfx_sensor_set_prv_data is called...\n");

  hwc->u.sensor.xclk = s5ka3dfx_hwc.xclk;
  hwc->u.sensor.sensor_isp = s5ka3dfx_hwc.sensor_isp;
  hwc->dev_index = 1;
  hwc->dev_minor = 5;
  hwc->dev_type = OMAP34XXCAM_SLAVE_SENSOR;

  return 0;
}


struct s5ka3dfx_platform_data nowplus_s5ka3dfx_platform_data = {
  .power_set      = s5ka3dfx_sensor_power_set,
  .priv_data_set  = s5ka3dfx_sensor_set_prv_data,
  .ifparm         = s5ka3dfx_ifparm,
};
