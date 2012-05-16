/*
 * tl2796 panel support
 *
 * Copyright (C) 2008 Nokia Corporation
 * Author: Tomi Valkeinen <tomi.valkeinen@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/i2c/twl.h>
#include <linux/spi/spi.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>

#include <mach/gpio.h>
#include <mach/hardware.h>
#include <plat/mux.h>
#include <asm/mach-types.h>
#include <plat/control.h>

#include <plat/display.h>
#include <linux/leds.h>

#define LCD_XRES		480
#define LCD_YRES		800

#define LCD_HBP		8
#define LCD_HFP		8	
#define LCD_HSW		1	
	
#define LCD_VBP		8
#define LCD_VFP		8
#define LCD_VSW		1	

#define LCD_PIXCLOCK_MAX	24284 //12000
#define LCD_DEFAULT_BRIGHTNESS	    6
#define LCD_DEFAULT_GAMMA           GAMMA_2_2   // 2.2 gamma table.
#define HAND_FLASH_DEFAULT          flash_off	// Archer_LSJ DB10
#define LCD_DEFAULT_ACL_STATE		ACL_OFF
#define LDI_POWER_ON                1
#define LDI_POWER_OFF               0

#define GPIO_LEVEL_LOW   0
#define GPIO_LEVEL_HIGH  1

static struct spi_device *tl2796lcd_spi;
int lcdlevel = LCD_DEFAULT_BRIGHTNESS;	
int lcdgamma = LCD_DEFAULT_GAMMA;
int lcdacl   = LCD_DEFAULT_ACL_STATE;
static int ldi_power_status = LDI_POWER_ON;	// ldi power state

// regulator
struct regulator *vaux3;
struct regulator *vaux4;
//struct regulator *vpll2;

void tl2796_lcd_LDO_on(void);
void tl2796_lcd_LDO_off(void);
void tl2796_lcd_poweron(void);
void tl2796_lcd_poweroff(void);
void tl2796_ldi_poweron(void);
void tl2796_ldi_poweroff(void);
void tl2796_ldi_standby(void);
void tl2796_ldi_wakeup(void);


extern int omap34xx_pad_set_config_lcd(u16,u16);



static u8 tl2796_300[] = {0x00, 0x00, 0x00, 0x24, 0x24, 0x1E, 0x3F, 0x00, 0x00, 0x00, 0x23, 0x24, 0x1E, 0x3D, 0x00, 0x00, 0x00, 0x22, 0x22, 0x1A, 0x56};
static u8 tl2796_280[] = {0x00, 0x00, 0x00, 0x24, 0x24, 0x1F, 0x3D, 0x00, 0x00, 0x00, 0x23, 0x24, 0x1F, 0x3B, 0x00, 0x00, 0x00, 0x22, 0x21, 0x1B, 0x54};
static u8 tl2796_260[] = {0x00, 0x00, 0x00, 0x23, 0x24, 0x20, 0x3B, 0x00, 0x00, 0x00, 0x22, 0x24, 0x20, 0x39, 0x00, 0x00, 0x00, 0x22, 0x21, 0x1C, 0x51};
static u8 tl2796_240[] = {0x00, 0x00, 0x00, 0x23, 0x24, 0x21, 0x39, 0x00, 0x00, 0x00, 0x23, 0x25, 0x20, 0x37, 0x00, 0x00, 0x00, 0x22, 0x22, 0x1C, 0x4F};
static u8 tl2796_220[] = {0x00, 0x00, 0x0B, 0x25, 0x24, 0x21, 0x37, 0x00, 0x00, 0x00, 0x23, 0x25, 0x21, 0x35, 0x00, 0x00, 0x11, 0x22, 0x23, 0x1D, 0x4B};
static u8 tl2796_200[] = {0x00, 0x00, 0x11, 0x25, 0x24, 0x22, 0x34, 0x00, 0x00, 0x00, 0x23, 0x25, 0x22, 0x32, 0x00, 0x00, 0x11, 0x23, 0x22, 0x1E, 0x48};
static u8 tl2796_180[] = {0x00, 0x00, 0x11, 0x24, 0x25, 0x22, 0x32, 0x00, 0x00, 0x00, 0x23, 0x25, 0x22, 0x30, 0x00, 0x00, 0x11, 0x23, 0x23, 0x1E, 0x45};
static u8 tl2796_160[] = {0x00, 0x00, 0x17, 0x25, 0x25, 0x22, 0x2F, 0x00, 0x00, 0x00, 0x23, 0x26, 0x22, 0x22, 0x00, 0x00, 0x14, 0x24, 0x23, 0x1E, 0x43};
static u8 tl2796_140[] = {0x00, 0x00, 0x1A, 0x26, 0x25, 0x22, 0x2D, 0x00, 0x00, 0x00, 0x23, 0x25, 0x22, 0x2B, 0x00, 0x00, 0x1A, 0x24, 0x23, 0x1E, 0x3F};
static u8 tl2796_120[] = {0x00, 0x00, 0x1A, 0x26, 0x27, 0x24, 0x29, 0x00, 0x00, 0x00, 0x23, 0x26, 0x24, 0x28, 0x00, 0x00, 0x1A, 0x23, 0x25, 0x20, 0x3B};
static u8 tl2796_100[] = {0x00, 0x00, 0x1A, 0x26, 0x28, 0x23, 0x27, 0x00, 0x00, 0x00, 0x22, 0x27, 0x24, 0x25, 0x00, 0x00, 0x1A, 0x24, 0x26, 0x20, 0x37};
static u8 tl2796_40[] = {0x00, 0x00, 0x1A, 0x2C, 0x2A, 0x28, 0x19, 0x00, 0x00, 0x00, 0x22, 0x29, 0x28, 0x18, 0x00, 0x00, 0x1A, 0x29, 0x29, 0x25, 0x25};

static u8 tl2796_250[] = {0x00, 0x00, 0x00, 0x28, 0x25, 0x1F, 0x3B, 0x00, 0x00, 0x00, 0x26, 0x26, 0x20, 0x38, 0x00, 0x00, 0x00, 0x26, 0x24, 0x1C, 0x4F};
static u8 tl2796_110[] = {0x00, 0x00, 0x1A, 0x2A, 0x28, 0x24, 0x28, 0x00, 0x00, 0x00, 0x26, 0x28, 0x24, 0x27, 0x00, 0x00, 0x1A, 0x28, 0x26, 0x21, 0x38};
static u8 tl2796_105[] = {0x0F, 0x00, 0x00, 0x01, 0x07, 0x0D, 0x29, 0x00, 0x3F, 0x2C, 0x27, 0x24, 0x21, 0x2A, 0x0F, 0x3F, 0x0A, 0x07, 0x0A, 0xD, 0x37};
static u8 tl2796_60[] = {0x00, 0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x11, 0x00, 0x00, 0x00, 0x14, 0x1E, 0x1D, 0x14, 0x00, 0x00, 0x00, 0x1B, 0x1D, 0x1C, 0x1E};
static u8 tl2796_50[] = {0x00, 0x00, 0x1A, 0x29, 0x28, 0x27, 0x1B, 0x00, 0x00, 0x00, 0x21, 0x27, 0x27, 0x1A, 0x00, 0x00, 0x1A, 0x27, 0x26, 0x25, 0x29};



	int cd[12] = {0, 40, 120, 140, 180, 180, 200, 220, 240, 260, 280, 300}; //here


void tl2796_gamma_ctl(u8 *gamma);



/*  Manual
 * defines HFB, HSW, HBP, VFP, VSW, VBP as shown below
 */

static struct omap_video_timings tl2796_panel_timings = {
	/* 800 x 480 @ 60 Hz  Reduced blanking VESA CVT 0.31M3-R */
	.x_res          = LCD_XRES,
	.y_res          = LCD_YRES,
	.pixel_clock    = LCD_PIXCLOCK_MAX,
	.hfp            = LCD_HFP,
	.hsw            = LCD_HSW,
	.hbp            = LCD_HBP,
	.vfp            = LCD_VFP,
	.vsw            = LCD_VSW,
	.vbp            = LCD_VBP,
};

static int tl2796_panel_probe(struct omap_dss_device *dssdev)
{
    int ret;
    
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |
				OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_RF |
				OMAP_DSS_LCD_ONOFF; /*OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |OMAP_DSS_LCD_IEO |OMAP_DSS_LCD_IPC |
						OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_ONOFF */;
	dssdev->panel.acb = 0;

	//dssdev->panel.recommended_bpp= 32;

	dssdev->panel.timings = tl2796_panel_timings;
//have to reserve GPIO	
	// ret = gpio_request(OMAP_GPIO_MLCD_RST, "MLCD_RST");
	// if (ret < 0) {
                // printk(KERN_ERR "%s: can't reserve GPIO: %d\n", __func__,
                        // OMAP_GPIO_MLCD_RST);
		// return -1;
	// }
	gpio_direction_output(OMAP_GPIO_MLCD_RST, GPIO_LEVEL_HIGH);
	
	vaux4 = regulator_get( &dssdev->dev, "vaux4" );
	if( IS_ERR( vaux4 ) )
		printk("Fail to register vaux4 using regulator framework!\n");

	vaux3 = regulator_get( &dssdev->dev, "vaux3" );
	if( IS_ERR( vaux3 ) )
		printk("Fail to register vaux3 using regulator framework!\n");

	//vpll2 = regulator_get( &dssdev->dev, "vpll2" );
	//if( IS_ERR( vpll2 ) )
	//	printk("Fail to register vpll2 using regulator framework!\n");
	
	tl2796_lcd_LDO_on();
	
	return 0;
}

static void tl2796_panel_remove(struct omap_dss_device *dssdev)
{
	regulator_put( vaux4 );
	regulator_put( vaux3 );
	//regulator_put( vpll2 );
	 //gpio_free(OMAP_GPIO_MLCD_RST);
}

static int tl2796_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;
	mdelay(4);
	if (dssdev->platform_enable)
		r = dssdev->platform_enable(dssdev);
		
	if (dssdev->state != OMAP_DSS_DISPLAY_DISABLED) {
			r = -EINVAL;
			goto err;
	}

	dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

err:
	return r;
}

static void tl2796_panel_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);
		
	if (dssdev->state == OMAP_DSS_DISPLAY_DISABLED)
		goto end;

	if (dssdev->state == OMAP_DSS_DISPLAY_SUSPENDED) {
			printk(KERN_WARNING "[LCD] ams353_panel_disabled - called when panel is in suspend\n");
			goto end;
	}

    dssdev->state = OMAP_DSS_DISPLAY_DISABLED;

end:
	mdelay(4);
}

static int tl2796_panel_suspend(struct omap_dss_device *dssdev)
{
	printk("\n **** tl2796_panel_suspend");
	spi_setup(tl2796lcd_spi);

    tl2796_lcd_poweroff();
     
	 
	dssdev->state = OMAP_DSS_DISPLAY_SUSPENDED;

	return 0;
}

static int tl2796_panel_resume(struct omap_dss_device *dssdev)
{
	printk("\n **** tl2796_panel_resume");

	spi_setup(tl2796lcd_spi);
    tl2796_lcd_poweron();
    dssdev->state = OMAP_DSS_DISPLAY_ACTIVE;

	return 0;
}

static struct omap_dss_driver tl2796_driver = {
	.probe          = tl2796_panel_probe,
	.remove         = tl2796_panel_remove,

	.enable         = tl2796_panel_enable,
	.disable        = tl2796_panel_disable,
	.suspend        = tl2796_panel_suspend,
	.resume         = tl2796_panel_resume,

	.driver		= {
		.name	= "tl2796_panel",
		.owner 	= THIS_MODULE,
	},
};





void tl2796_lcd_brightness_setting(int idx)
{
	printk(KERN_DEBUG "%s:%d\n", __FUNCTION__, idx);

	if ((idx > 11) || (idx < 0))
		idx = 7;

	switch (cd[idx])
	{
		case	300 :	tl2796_gamma_ctl(tl2796_300);	break;
		case	280 :	tl2796_gamma_ctl(tl2796_280);	break;
		case	260 :	tl2796_gamma_ctl(tl2796_260);	break;
		case	250 :	tl2796_gamma_ctl(tl2796_250);	break;
		case	240 :	tl2796_gamma_ctl(tl2796_240);	break;
		case	220 :	tl2796_gamma_ctl(tl2796_220);	break;
		case	200 :	tl2796_gamma_ctl(tl2796_200);	break;
		case	180 :	tl2796_gamma_ctl(tl2796_180);	break;
		case	160 :	tl2796_gamma_ctl(tl2796_160);	break;
		case	140 :	tl2796_gamma_ctl(tl2796_140);	break;
		case	120 :	tl2796_gamma_ctl(tl2796_120);	break;
		case	110 :	tl2796_gamma_ctl(tl2796_110);	break;
		case	105 :	tl2796_gamma_ctl(tl2796_105);	break;
		case	100 :	tl2796_gamma_ctl(tl2796_100);	break;
		case	60	:	tl2796_gamma_ctl(tl2796_60);		break;
		case	50	:	tl2796_gamma_ctl(tl2796_50);		break;
		case	40	:	tl2796_gamma_ctl(tl2796_40);		break;
	}

}

void tl2796_lcd_brightness_on(void)
{
	printk(KERN_DEBUG "%s\n", __FUNCTION__);
	tl2796_lcd_brightness_setting(lcdlevel);
}


static void spi1writeindex(u8 index)
{
   volatile unsigned short cmd = 0;
   cmd= 0x7000+ index;

    spi_write(tl2796lcd_spi,(unsigned char*)&cmd,2);

     udelay(100);
     udelay(100);
}
static void spi1writedata(u8 data)
{
      volatile unsigned short datas = 0;
      datas= 0x7200+ data;
    	spi_write(tl2796lcd_spi,(unsigned char*)&datas,2);

     udelay(100);
     udelay(100);
}

static void spi1write(u8 index, u8 data)
{
      int ret = 0;
      volatile unsigned short cmd = 0;
      volatile unsigned short datas=0;
      cmd= 0x7000+ index;
      datas=0x7200+data;
	
      spi_write(tl2796lcd_spi,(unsigned char*)&cmd,2);
      udelay(100);
     spi_write(tl2796lcd_spi,(unsigned char *)&datas,2);
     udelay(100);
     udelay(100);
     
      return ret;
}

void tl2796_gamma_ctl(u8 *gamma)
{
	u16 reg = 0;
	u16 data = 0;
	
	for(reg = 0x40; reg<= 0x46; reg++)
	{
		spi1write(reg, gamma[data]);
		data++;
	}
	for(reg = 0x50; reg<= 0x56; reg++)
	{
		spi1write(reg, gamma[data]);
		data++;
	}
	for(reg = 0x60; reg<= 0x66; reg++)
	{
		spi1write(reg, gamma[data]);
		data++;
	}
}



void tl2796_ldi_standby(void)
{
	// Sleep-in

	// Wait for 200ms
	msleep(200);
}

void tl2796_ldi_wakeup(void)
{
	// Sleep-out

	// Wait for 200ms
	msleep(200);
}

void tl2796_ldi_poweron(void)
{
	printk(KERN_DEBUG " ****  tl2796_ldi_poweron. IN \n");
		
	//System Power On 1 (VBATT)
	//System Power On 2 (IOVCC)
	//System Power On 3 (VCI)
	//Wait for minimum 25ms

	//STB off 1Dh xxA0h
	spi1write(0x1D, 0xA0);
	mdelay(200);
	
	//DDISP ON 14h xxx3h
	spi1write(0x14, 0x03);

	//Wait for minimum 250ms
	//mdelay(250);

	//. Panel Condition SET
	spi1write(0x31, 0x08);	//SCTE set
	spi1write(0x32, 0x14);	//SC set
	spi1write(0x30, 0x02);	//Gateless signal
	spi1write(0x27, 0x01);	//Gateless signal


	//Display Condition SET
	spi1write(0x12, 0x08);	//VBP set
	spi1write(0x13, 0x08);	//VFP set
	spi1write(0x15, 0x03);	//Gateless signal
	spi1write(0x16, 0x00);	//Color depth set
	
	spi1writeindex(0xEF);	//Pentile Key
	spi1writedata(0xD0);
	spi1writedata(0xE8);

	spi1write(0x39, 0x44); //KRANTI
	tl2796_lcd_brightness_on();

	//Analog power Condition SET
	spi1write(0x17, 0x22);	//Boosting Freq
	spi1write(0x18, 0x33);	//AMP set
	spi1write(0x19, 0x03);	//spi1write(0x19, 0x33);	//Gamma Amp
	spi1write(0x1A, 0x01);	//Power Boosting Rate
	spi1write(0x22, 0xA4);	//Internal Logic voltage
	spi1write(0x23, 0x00);	//Power set (VCI1OUT)
	spi1write(0x26, 0xA0);	//Display Condition set


	// spi1write(0x27, 0x02) ;//SAM_SOC

	ldi_power_status =  LDI_POWER_ON;
	printk(KERN_DEBUG " ****  tl2796_ldi_poweron. OUT \n");
}

void tl2796_ldi_poweroff(void)
{
  	printk("\n **** tl2796_ldi_poweroff");

	spi1write(0x1D, 0xA1);
	
	//Wait 200ms
	msleep(250);	//mdelay(200);
	
	
	ldi_power_status = LDI_POWER_OFF;
}


void tl2796_lcd_LDO_on(void)
{
	int ret;
	
	ret = regulator_enable( vaux3 ); //VAUX2 - 1.8V
	if ( ret )
		printk("Regulator vaux2 error!!\n");

	ret = regulator_enable( vaux4 ); //VAUX3 - 2.8V
	if ( ret )
		printk("Regulator vaux3 error!!\n");

	//ret = regulator_enable( vpll2 );
	//if ( ret )
	//	printk("Regulator vpll2 error!!\n");

}

void tl2796_lcd_LDO_off(void)
{
	int ret;

	ret = regulator_disable( vaux4 );
	if ( ret )
		printk("Regulator vaux4 error!!\n");

	ret = regulator_disable( vaux3 );
	if ( ret )
		printk("Regulator vaux3 error!!\n");

	//ret = regulator_disable( vpll2 );
	//if ( ret )
	//	printk("Regulator vpll2 error!!\n");
}


void tl2796_lcd_poweroff(void)
{
	int ret = 0;
	//lcd_poweroff_done = 0;

	tl2796_ldi_poweroff();
	
	tl2796_lcd_LDO_off();
	
	// disable spi pads, set to HiZ with pulldown
    omap34xx_pad_set_config_lcd(0x01C8,  OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN);
    omap34xx_pad_set_config_lcd(0x01CA,  OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN);
    omap34xx_pad_set_config_lcd(0x01CC,  OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN);
    omap34xx_pad_set_config_lcd(0x01CE,  OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN);
    
    return;	
}
void tl2796_lcd_poweron(void)
{
	int level;
	int ret = 0;

	//enable spi pads
	omap34xx_pad_set_config_lcd(0x01C8,  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP);
	omap34xx_pad_set_config_lcd(0x01CA,  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN);
	omap34xx_pad_set_config_lcd(0x01CC,  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN);
	omap34xx_pad_set_config_lcd(0x01CE,  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP);

	gpio_set_value(OMAP_GPIO_MLCD_RST, GPIO_LEVEL_HIGH);
		
	tl2796_lcd_LDO_on();

	// Wait 25ms
	mdelay(25);

	//Activate Reset
	gpio_set_value(OMAP_GPIO_MLCD_RST, GPIO_LEVEL_LOW);	
	mdelay(1); 
	gpio_set_value(OMAP_GPIO_MLCD_RST, GPIO_LEVEL_HIGH);
	// wait 20ms
	mdelay(20);
	
	tl2796_ldi_poweron();
	return;
}


static void tl2796_set_brightness(struct led_classdev *dev, enum led_brightness value)
{
	int retry_count = 10;
	int brightness = (value * 10) / 255;
	
	if( value < 0 || value > 255 )
		return;
	
	if (lcdlevel != brightness)
	{
        lcdlevel = brightness;
        
	  if ( ldi_power_status == LDI_POWER_ON )
	  {
		      tl2796_lcd_brightness_setting(lcdlevel);
	  }
	  else 
	  {
	      printk(KERN_INFO " LCD BRIGHTNESS is not set when LDI power status is Off.");
	  }
   	}
}

static struct led_classdev tl2796_backlight_led = {
	.name = "lcd-backlight",
	.brightness = LCD_DEFAULT_BRIGHTNESS * 25.5,
	.brightness_set = tl2796_set_brightness,
};

static int tl2796_spi_probe(struct spi_device *spi)
{
	tl2796lcd_spi = spi;
	tl2796lcd_spi->mode = SPI_MODE_0;
	tl2796lcd_spi->bits_per_word = 16 ;
	spi_setup(tl2796lcd_spi);

	omap_dss_register_driver(&tl2796_driver);
	led_classdev_register(&spi->dev, &tl2796_backlight_led);

#ifndef CONFIG_PANEL_BOOTLOADER_INIT
	tl2796_lcd_poweron();
#endif
	return 0;
}

static int tl2796_spi_remove(struct spi_device *spi)
{
	led_classdev_unregister(&tl2796_backlight_led);
	omap_dss_unregister_driver(&tl2796_driver);

	return 0;
}

static void tl2796_spi_shutdown(struct spi_device *spi)
{
	printk("First power off LCD.\n");
	tl2796_lcd_poweroff();
}

static int tl2796_spi_suspend(struct spi_device *spi, pm_message_t mesg)
{
	//spi_send(spi, 2, 0x01);  /* R2 = 01h */
//	mdelay(40);

#if 0
	tl2796lcd_spi = spi;
	tl2796lcd_spi->mode = SPI_MODE_0;
	tl2796lcd_spi->bits_per_word = 16 ;
	spi_setup(tl2796lcd_spi);

	tl2796_lcd_poweroff();
	zeus_panel_power_enable(0);
#endif

	return 0;
}

static int tl2796_spi_resume(struct spi_device *spi)
{
	/* reinitialize the panel */
#if 0
	zeus_panel_power_enable(1);
	tl2796lcd_spi = spi;
	tl2796lcd_spi->mode = SPI_MODE_0;
	tl2796lcd_spi->bits_per_word = 16 ;
	spi_setup(tl2796lcd_spi);

	tl2796_lcd_poweron();
#endif
	return 0;
}

static struct spi_driver tl2796_spi_driver = {
	.probe      = tl2796_spi_probe,
	.remove	    = tl2796_spi_remove,
	.shutdown   = tl2796_spi_shutdown,	
	.suspend    = tl2796_spi_suspend,
	.resume     = tl2796_spi_resume,
	.driver     = {
		.name   = "tl2796_disp_spi",
		.bus    = &spi_bus_type,
		.owner  = THIS_MODULE,
	},
};

static int __init tl2796_lcd_init(void)
{
	u8 gamma_3th_40[] = {0x0F, 0x3F, 0x00, 0x01, 0x01, 0x0C, 0x1A, 0x00, 0x3F, 0x30, 0x2A, 0x25, 0x25, 0x1A, 0x0F, 0x00, 0x15, 0x09, 0x06, 0x0E, 0x24};
	u8 gamma_3th_50[] = {0x0F, 0x00, 0x05, 0x00, 0x05, 0x0C, 0x1D, 0x00, 0x00, 0x2F, 0x29, 0x26, 0x24, 0x1D, 0x0F, 0x3F, 0x0F, 0x09, 0x09, 0x0E, 0x27};
	u8 gamma_3th_60[] = {0x0F, 0x00, 0x05, 0x01, 0x07, 0x0B, 0x20, 0x00, 0x3F, 0x2F, 0x29, 0x26, 0x23, 0x20, 0x0F, 0x3F, 0x0F, 0x09, 0x0A, 0x0D, 0x2B};
	u8 gamma_3th_100[] = {0x0F, 0x00, 0x00, 0x01, 0x06, 0x0C, 0x29, 0x00, 0x00, 0x2F, 0x27, 0x24, 0x21, 0x29, 0x0F, 0x3F, 0x0D, 0x07, 0x09, 0x0D, 0x36};
	u8 gamma_3th_105[] = {0x0F, 0x00, 0x00, 0x01, 0x07, 0x0D, 0x29, 0x00, 0x3F, 0x2C, 0x27, 0x24, 0x21, 0x2A, 0x0F, 0x3F, 0x0A, 0x07, 0x0A, 0x0D, 0x37};
	u8 gamma_3th_110[] = {0x0F, 0x00, 0x00, 0x01, 0x07, 0x0D, 0x2B, 0x00, 0x3F, 0x2C, 0x27, 0x23, 0x21, 0x2C, 0x0F, 0x00, 0x0A, 0x07, 0x09, 0x0E, 0x39};	//=115
	u8 gamma_3th_120[] = {0x0F, 0x00, 0x00, 0x01, 0x08, 0x0D, 0x2C, 0x00, 0x3F, 0x2C, 0x27, 0x24, 0x20, 0x2D, 0x0F, 0x00, 0x0A, 0x07, 0x0B, 0x0D, 0x3A};
	u8 gamma_3th_140[] = {0x0F, 0x3F, 0x00, 0x00, 0x07, 0x0C, 0x2F, 0x00, 0x3F, 0x2D, 0x26, 0x23, 0x1F, 0x30, 0x0F, 0x00, 0x0F, 0x05, 0x0A, 0x0C, 0x3E};
	u8 gamma_3th_160[] = {0x0F, 0x3F, 0x00, 0x00, 0x08, 0x0D, 0x32, 0x00, 0x3F, 0x2E, 0x25, 0x23, 0x1F, 0x33, 0x0F, 0x00, 0x0E, 0x05, 0x0A, 0x0D, 0x42};
	u8 gamma_3th_180[] = {0x0F, 0x3F, 0x02, 0x01, 0x09, 0x0C, 0x35, 0x00, 0x3F, 0x2E, 0x25, 0x23, 0x1E, 0x36, 0x0F, 0x00, 0x11, 0x05, 0x0B, 0x0C, 0x46};
	u8 gamma_3th_200[] = {0x0F, 0x3F, 0x00, 0x01, 0x0B, 0x0C, 0x37, 0x00, 0x00, 0x2C, 0x25, 0x23, 0x1F, 0x38, 0x0F, 0x00, 0x05, 0x07, 0x0C, 0x0E, 0x48};//here
	u8 gamma_3th_220[] = {0x0F, 0x3F, 0x01, 0x01, 0x09, 0x0A, 0x3A, 0x00, 0x3F, 0x2D, 0x25, 0x22, 0x1D, 0x3A, 0x0F, 0x00, 0x0D, 0x06, 0x0A, 0x0B, 0x4B};
	u8 gamma_3th_240[] = {0x0F, 0x3F, 0x00, 0x02, 0x09, 0x0B, 0x3C, 0x00, 0x3F, 0x2D, 0x25, 0x22, 0x1C, 0x3D, 0x0F, 0x00, 0x0D, 0x06, 0x0B, 0x0B, 0x4E};
	u8 gamma_3th_250[] = {0x0F, 0x3F, 0x00, 0x00, 0x0A, 0x0B, 0x3D, 0x00, 0x3F, 0x2D, 0x24, 0x22, 0x1C, 0x3E, 0x0F, 0x00, 0x0A, 0x05, 0x0B, 0x0B, 0x50};
	u8 gamma_3th_260[] = {0x0F, 0x3F, 0x00, 0x01, 0x09, 0x0B, 0x3F, 0x00, 0x3F, 0x2D, 0x24, 0x21, 0x1C, 0x40, 0x0F, 0x00, 0x08, 0x06, 0x0A, 0x0B, 0x52};
	u8 gamma_3th_280[] = {0x0F, 0x3F, 0x00, 0x00, 0x09, 0x0A, 0x41, 0x00, 0x3F, 0x2F, 0x23, 0x21, 0x1B, 0x42, 0x0F, 0x00, 0x0D, 0x04, 0x0A, 0x0A, 0x55};
	u8 gamma_3th_300[] = {0x0F, 0x3F, 0x00, 0x00, 0x09, 0x0B, 0x43, 0x00, 0x3F, 0x2C, 0x23, 0x21, 0x1B, 0x44, 0x0F, 0x00, 0x0D, 0x04, 0x0A, 0x0B, 0x57};
	int cd_rev05[7] = {0, 40, 105, 160, 200, 250, 300};

	{
		memcpy(tl2796_40,gamma_3th_40, sizeof(gamma_3th_40));
		memcpy(tl2796_50,gamma_3th_50, sizeof(gamma_3th_50));
		memcpy(tl2796_60,gamma_3th_60, sizeof(gamma_3th_60));
		memcpy(tl2796_100,gamma_3th_100, sizeof(gamma_3th_100));
		memcpy(tl2796_105,gamma_3th_105, sizeof(gamma_3th_105));
		memcpy(tl2796_110,gamma_3th_110, sizeof(gamma_3th_110));
		memcpy(tl2796_120,gamma_3th_120, sizeof(gamma_3th_120));
		memcpy(tl2796_140,gamma_3th_140, sizeof(gamma_3th_140));
		memcpy(tl2796_160,gamma_3th_160, sizeof(gamma_3th_160));
		memcpy(tl2796_180,gamma_3th_180, sizeof(gamma_3th_180));
		memcpy(tl2796_200,gamma_3th_200, sizeof(gamma_3th_200));
		memcpy(tl2796_220,gamma_3th_220, sizeof(gamma_3th_220));
		memcpy(tl2796_240,gamma_3th_240, sizeof(gamma_3th_240));
		memcpy(tl2796_250,gamma_3th_250, sizeof(gamma_3th_250));
		memcpy(tl2796_260,gamma_3th_260, sizeof(gamma_3th_260));
		memcpy(tl2796_280,gamma_3th_280, sizeof(gamma_3th_280));
		memcpy(tl2796_300,gamma_3th_300, sizeof(gamma_3th_300));

		memcpy(cd, cd_rev05, sizeof(cd));
	}

	return spi_register_driver(&tl2796_spi_driver);
}

static void __exit tl2796_lcd_exit(void)
{
	return spi_unregister_driver(&tl2796_spi_driver);
}


module_init(tl2796_lcd_init);
module_exit(tl2796_lcd_exit);
MODULE_LICENSE("GPL");

