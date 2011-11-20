/*
 * nt35510 LDI support
 *
 * Copyright (C) 2009 Samsung Corporation
 * Author: Samsung Electronics..
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
/*
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <mach/mux.h>
#include <asm/mach-types.h>
#include <mach/control.h>

#include <mach/display.h>
*/
#include <plat/gpio.h>
#include <plat/hardware.h>
#include <plat/mux.h>
#include <asm/mach-types.h>
#include <plat/control.h>

#include <plat/display.h>

//#include <linux/leds.h>
#include <linux/backlight.h>
#include  <../../../../arch/arm/mach-omap2/mux.h>
#include <linux/i2c/twl.h>



#define LCD_XRES		480
#define LCD_YRES		800

static int current_panel = 0;	// 1:Hitach , 0:sony

// default setting : sony panel. 
static u16 LCD_HBP =	10;//20; 
static u16 LCD_HFP =	10; 
static u16 LCD_HSW =	10; 
static u16 LCD_VBP =	9;// 10;//8; 
static u16 LCD_VFP =	4;// 14;//6; 
static u16 LCD_VSW =	2; 

#define LCD_PIXCLOCK_MAX	        24000 // 26000


#define GPIO_LEVEL_LOW   0
#define GPIO_LEVEL_HIGH  1

#define POWER_OFF	0	// set in lcd_poweroff function.
#define POWER_ON	1	// set in lcd_poweron function

static struct spi_device *nt35510lcd_spi;
    

static atomic_t lcd_power_state = ATOMIC_INIT(POWER_ON);	// default is power on because bootloader already turn on LCD.
static atomic_t ldi_power_state = ATOMIC_INIT(POWER_ON);	// ldi power state

int g_lcdlevel = 0x6C;

// ------------------------------------------ // 
//          For Regulator Framework                            //
// ------------------------------------------ // 

struct regulator *vaux3;
struct regulator *vaux4;

#define MAX_NOTIFICATION_HANDLER	10

void nt35510_lcd_poweron(void);
void nt35510_lcd_poweroff(void);
void nt35510_lcd_LDO_on(void);
void nt35510_lcd_LDO_off(void);
void nt35510_ldi_poweron_sony(void);
void nt35510_ldi_poweroff_sony(void);
void nt35510_ldi_poweron_hitach(void);
void nt35510_ldi_poweroff_hitach(void);


// paramter : POWER_ON or POWER_OFF
typedef void (*notification_handler)(const int);
typedef struct
{
	int  state;
	spinlock_t vib_lock;
}timer_state_t;
timer_state_t timer_state;


notification_handler power_state_change_handler[MAX_NOTIFICATION_HANDLER];

int nt35510_add_power_state_monitor(notification_handler handler);
void nt35510_remove_power_state_monitor(notification_handler handler);

EXPORT_SYMBOL(nt35510_add_power_state_monitor);
EXPORT_SYMBOL(nt35510_remove_power_state_monitor);

static void nt35510_notify_power_state_changed(void);
static void aat1402_set_brightness(void);
//extern int omap34xx_pad_set_configs(struct pin_config *pin_configs, int n);
extern int omap34xx_pad_set_config_lcd(u16,u16);

#define PM_RECEIVER                     TWL4030_MODULE_PM_RECEIVER

#define ENABLE_VPLL2_DEDICATED          0x05
#define ENABLE_VPLL2_DEV_GRP            0x20
#define TWL4030_VPLL2_DEV_GRP           0x33
#define TWL4030_VPLL2_DEDICATED       	0x36


static struct pin_config  omap34xx_lcd_pins[] = {
/*
 *		Name, reg-offset,
 *		mux-mode | [active-mode | off-mode]
 */
 
	// MCSPI1 AND MCSPI2 PIN CONFIGURATION
	// FOR ALL SPI, SPI_CS0 IS I/O AND OTHER SPI CS ARE PURE OUT.
	// CLK AND SIMO/SOMI LINES ARE I/O.
/*
	// 206 (AB3, L, MCSPI1_CLK, DISPLAY_CLK, O)
	MUX_CFG_34XX("AB3_MCSPI1_CLK", 0x01C8,  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
	// 207 (AB4, L, MCSPI1_SIMO, DISLPAY_SI, I)
	MUX_CFG_34XX("AB4_MCSPI1_SIMO", 0x01CA,  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN)
	// 208 (AA4, L, MCSPI1_SOMI, DISLPAY_SO, O)
	MUX_CFG_34XX("AA4_MCSPI1_SOMI", 0x01CC,  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN)
	// 209 (AC2, H, MCSPI1_CS0, DISPLAY_CS, O)
	MUX_CFG_34XX("AC2_MCSPI1_CS0", 0x01CE,  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP)
	*/
	 //omap_mux_init_signal("gpio_163", OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
	// omap_mux_init_signal("MCSPI1_SIMO", OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN);
	// omap_mux_init_signal("MCSPI1_SOMI", OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN);
	// 209 (AC2, H, MCSPI1_CS0, DISPLAY_CS, O)
	//omap_mux_init_signal("sys_nirq",OMAP_WAKEUP_EN | OMAP_PIN_INPUT_PULLUP);
	//omap_mux_init_signal("MCSPI1_CS0", OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
	 
	
	
};

static struct pin_config  omap34xx_lcd_off_pins[] = {
/*
 *		Name, reg-offset,
 *		mux-mode | [active-mode | off-mode]
 */
 
	// MCSPI1 AND MCSPI2 PIN CONFIGURATION
	// FOR ALL SPI, SPI_CS0 IS I/O AND OTHER SPI CS ARE PURE OUT.
	// CLK AND SIMO/SOMI LINES ARE I/O.

	// 206 (AB3, L, MCSPI1_CLK, DISPLAY_CLK, O)
	//MUX_CFG_34XX("AB3_MCSPI1_CLK", 0x01C8,  OMAP_MUX_MODE7 | OMAP_PIN_INPUT_PULLDOWN)
	// 207 (AB4, L, MCSPI1_SIMO, DISLPAY_SI, I)
	//MUX_CFG_34XX("AB4_MCSPI1_SIMO", 0x01CA,  OMAP_MUX_MODE7 | OMAP_PIN_INPUT_PULLDOWN)
	// 208 (AA4, L, MCSPI1_SOMI, DISLPAY_SO, O)
	//MUX_CFG_34XX("AA4_MCSPI1_SOMI", 0x01CC,  OMAP_MUX_MODE7 | OMAP_PIN_INPUT_PULLDOWN)
	// 209 (AC2, H, MCSPI1_CS0, DISPLAY_CS, O)
	//MUX_CFG_34XX("AC2_MCSPI1_CS0", 0x01CE,  OMAP_MUX_MODE7 | OMAP_PIN_INPUT_PULLDOWN)
	
};

/*  Manual
 * defines HFB, HSW, HBP, VFP, VSW, VBP as shown below
 */

static struct omap_video_timings panel_timings = {0,};
#if 0 
= {
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
#endif
int nt35510_add_power_state_monitor(notification_handler handler)
{
	int index = 0;
	if(handler == NULL)
	{
		printk(KERN_ERR "[LCD][%s] param is null\n", __func__);
		return -EINVAL;
	}

	for(; index < MAX_NOTIFICATION_HANDLER; index++)
	{
		if(power_state_change_handler[index] == NULL)
		{
			power_state_change_handler[index] = handler;
			return 0;
		}
	}

	// there is no space this time
	printk(KERN_INFO "[LCD][%s] No spcae\n", __func__);

	return -ENOMEM;
}

void nt35510_remove_power_state_monitor(notification_handler handler)
{
	int index = 0;
	if(handler == NULL)
	{
		printk(KERN_ERR "[LCD][%s] param is null\n", __func__);
		return;
	}
	
	for(; index < MAX_NOTIFICATION_HANDLER; index++)
	{
		if(power_state_change_handler[index] == handler)
		{
			power_state_change_handler[index] = NULL;
		}
	}
}
	
static void nt35510_notify_power_state_changed(void)
{
	int index = 0;
	for(; index < MAX_NOTIFICATION_HANDLER; index++)
	{
		if(power_state_change_handler[index] != NULL)
		{
			power_state_change_handler[index](atomic_read(&lcd_power_state));
		}
	}

}

static int nt35510_panel_probe(struct omap_dss_device *dssdev)
{

      
	int lcd_id1, lcd_id2;
		
	vaux4 = regulator_get( &dssdev->dev, "vaux4" );
	if( IS_ERR( vaux4 ) )
		printk( "Fail to register vaux4 using regulator framework!\n" );	

	vaux3 = regulator_get( &dssdev->dev, "vaux3" );
	if( IS_ERR( vaux3 ) )
		printk( "Fail to register vaux3 using regulator framework!\n" );	

	nt35510_lcd_poweron(); //	nt35510_lcd_LDO_on();

	//MLCD pin set to InputPulldown.
	omap_ctrl_writew(0x010C, 0x1c6);	

	lcd_id1=gpio_get_value(OMAP_GPIO_LCD_ID1);
	lcd_id2=gpio_get_value(OMAP_GPIO_LCD_ID2);

	if(lcd_id1==0 && lcd_id2==1)
		current_panel=0;	// Sony
	else if(lcd_id1==1 && lcd_id2==0)
		current_panel=1;	// Hitach

	printk("[LCD] %s() : current_panel=%d(%s)\n", __func__, current_panel, (current_panel?"Hitach":"Sony"));

	if(current_panel == 1) // if hitach
	{
		LCD_HBP=	2; 
		LCD_HFP=	100; 
		LCD_HSW=	2;	
		LCD_VBP=	10; 	
		LCD_VFP=	8; 
		LCD_VSW=	2; 	
	}

	/* 800 x 480 @ 60 Hz  Reduced blanking VESA CVT 0.31M3-R */
	panel_timings.x_res          = LCD_XRES,
	panel_timings.y_res          = LCD_YRES,
	panel_timings.pixel_clock    = LCD_PIXCLOCK_MAX,
	panel_timings.hfp            = LCD_HFP,
	panel_timings.hsw            = LCD_HSW,
	panel_timings.hbp            = LCD_HBP,
	panel_timings.vfp            = LCD_VFP,
	panel_timings.vsw            = LCD_VSW,
	panel_timings.vbp            = LCD_VBP;

	if(current_panel==0) // Sony
	{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS |  OMAP_DSS_LCD_IPC |
						OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_ONOFF | OMAP_DSS_LCD_IEO;
	}

	else if(current_panel==1) // Hitach
	{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS | OMAP_DSS_LCD_IPC |
						OMAP_DSS_LCD_IHS | OMAP_DSS_LCD_ONOFF ;
	}

	//dssdev->panel.recommended_bpp= 32;  /* 35 kernel  recommended_bpp field is removed */
	dssdev->panel.acb = 0;
	dssdev->panel.timings = panel_timings;
	
	return 0;
}

static void nt35510_panel_remove(struct omap_dss_device *dssdev)
{
	regulator_put( vaux4 );
	regulator_put( vaux3 );
}

static int nt35510_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;
	mdelay(4);
	if (dssdev->platform_enable)
		r = dssdev->platform_enable(dssdev);

	return r;
}

static void nt35510_panel_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);
	mdelay(4);
}

static int nt35510_panel_suspend(struct omap_dss_device *dssdev)
{
	spi_setup(nt35510lcd_spi);

	gpio_set_value(OMAP_GPIO_LCD_EN_SET, GPIO_LEVEL_LOW);
	mdelay(1);

    #if 1
	nt35510_lcd_poweroff();
    #else
	nt35510_ldi_standby();
	nt35510_panel_disable(dssdev);
    #endif
   
    return 0;
}

static int nt35510_panel_resume(struct omap_dss_device *dssdev)
{

	spi_setup(nt35510lcd_spi);
    
	msleep(150);
    
    #if 1
	nt35510_lcd_poweron();
    #else
	nt35510_ldi_wakeup();
	nt35510_panel_enable(dssdev);
    #endif

	gpio_set_value(OMAP_GPIO_LCD_EN_SET, GPIO_LEVEL_LOW);
	mdelay(1);
	gpio_set_value(OMAP_GPIO_LCD_EN_SET, GPIO_LEVEL_HIGH);

	return 0;
}

static struct omap_dss_driver nt35510_driver = {
	.probe          = nt35510_panel_probe,
	.remove         = nt35510_panel_remove,

	.enable         = nt35510_panel_enable,
	.disable        = nt35510_panel_disable,
	.suspend        = nt35510_panel_suspend,
	.resume         = nt35510_panel_resume,

	.driver		= {
		.name	= "nt35510_panel",
		.owner 	= THIS_MODULE,
	},
};

static void spi1writeindex(u8 index)
{
	volatile unsigned short cmd = 0;
	cmd= 0x0000|index;

	spi_write(nt35510lcd_spi,(unsigned char*)&cmd,2);

	udelay(100);
	udelay(100);
}

static void spi1writedata(u8 data)
{
	volatile unsigned short datas = 0;
	datas= 0x0100|data;
	spi_write(nt35510lcd_spi,(unsigned char*)&datas,2);

	udelay(100);
	udelay(100);
}


static void spi1write(u8 index, u8 data)
{
	volatile unsigned short cmd = 0;
	volatile unsigned short datas=0;

	cmd = 0x0000 | index;
	datas = 0x0100 | data;
	
	spi_write(nt35510lcd_spi,(unsigned char*)&cmd,2);
	udelay(100);
	spi_write(nt35510lcd_spi,(unsigned char *)&datas,2);
	udelay(100);
	udelay(100);
}

void nt35510_ldi_poweron_hitach(void)
{
	printk("[LCD] %s() + \n", __func__);

	spi1writeindex(0x36);
	spi1writedata(0xD4); // reverse display
	
	spi1writeindex(0x3A);
	spi1writedata(0x77);

	spi1writeindex(0xB0);
	spi1writedata(0x04);

	spi1writeindex(0xD6);
	spi1writedata(0x28);
	
	spi1writeindex(0xFD);
	spi1writedata(0x00);
	spi1writedata(0x00);
	spi1writedata(0x70);
	spi1writedata(0x00);
	spi1writedata(0x32);
	spi1writedata(0x31);
	spi1writedata(0x34);
	spi1writedata(0x30);
	spi1writedata(0x32);
	spi1writedata(0x31);
	spi1writedata(0x04);
	spi1writedata(0x00);
	spi1writedata(0x00);
	
	spi1writeindex(0xFE);
	spi1writedata(0x00);
	spi1writedata(0x00);
	spi1writedata(0x00);
	spi1writedata(0x20);

	spi1writeindex(0xB0);
	spi1writedata(0x03);

	mdelay(1);
	spi1writeindex(0x11);

	msleep(150);
/*
	spi1writeindex(0x51);
	spi1writedata(0x6C); // default brightness : 108

	spi1writeindex(0x55);
	spi1writedata(0x0); // CABC Off, 01:User interface mode, 02:still image mode, 03:moving image mode

	spi1writeindex(0x5E);
	spi1writedata(0x00); // example value. *3)

	spi1writeindex(0x53);
	spi1writedata(0x2C);
*/
	spi1writeindex(0x29);	

	atomic_set(&ldi_power_state, POWER_ON);
	printk("[LCD] %s() -\n", __func__);
}


void nt35510_ldi_poweroff_hitach(void)
{
	printk(" **** %s\n", __func__);

	// Display Off
	spi1writeindex(0x28);	// SEQ_DISPOFF_SET

	// SLEEP IN
	spi1writeindex(0x10);	// SEQ_SLEEP IN_SET

	//Wait 120ms
	msleep(150);	
	
	atomic_set(&ldi_power_state, POWER_OFF);
}

void nt35510_ldi_poweron_sony(void)
{
	printk("[LCD] %s() + \n", __func__);
#if 0	
	printk("[LCD] %s() current_panel=%d \n", __func__, current_panel);

	if(0) // Hydis
	{
		/* Test Commands */
		spi1writeindex(0xFF);	
		spi1writedata(0xAA);
		spi1writedata(0x55);
		spi1writedata(0x25);
		spi1writedata(0x01);
		
		spi1writeindex(0xFA);	
		spi1writedata(0x00);
		spi1writedata(0x80);
		spi1writedata(0x00);
		spi1writedata(0x00);		
		spi1writedata(0x00);
		spi1writedata(0x00);
		spi1writedata(0x30);
		spi1writedata(0x02);					
		spi1writedata(0x20);
		spi1writedata(0x84);
		spi1writedata(0x0F);
		spi1writedata(0x0F);				
		spi1writedata(0x20);
		spi1writedata(0x40);
		spi1writedata(0x40);		
		spi1writedata(0x00);
		spi1writedata(0x00);		
		spi1writedata(0x00);
		spi1writedata(0x00);					
		spi1writedata(0x00);
		spi1writedata(0x00);
		spi1writedata(0x00);		
		spi1writedata(0x00);
		spi1writedata(0x00);
		spi1writedata(0x00);
		spi1writedata(0x00);
		spi1writedata(0x00);		
		spi1writedata(0x00);
		spi1writedata(0x3F);					
		spi1writedata(0xA0);
		spi1writedata(0x30);
		
		spi1writeindex(0xF3);
		spi1writedata(0x00);
		spi1writedata(0x32);
		spi1writedata(0x00);
		spi1writedata(0x38);		
		spi1writedata(0x00);
		spi1writedata(0x08);
		spi1writedata(0x11);
		spi1writedata(0x00);

		/* Enable CMD2_Page1 */
		spi1writeindex(0xF0);
		spi1writedata(0x55);
		spi1writedata(0xAA);
		spi1writedata(0x52);
		spi1writedata(0x08);		
		spi1writedata(0x01);	

		/* AVDD */
		spi1writeindex(0xB6);
		spi1writedata(0x04); 
		spi1writedata(0x04);
		spi1writedata(0x04);

		spi1writeindex(0xB0);		
		spi1writedata(0x05);			 
		spi1writedata(0x0A); //VBP 10
		spi1writedata(0x08); //VFP  8
		spi1writedata(0x1D); //HBP 29
		spi1writedata(0x02); //HFP  2
		
		/* ### VGH */
		spi1writeindex(0xBF);
		spi1writedata(0x01); 
				
		spi1writeindex(0xB9);			
		spi1writedata(0x34);
		spi1writedata(0x34);		
		spi1writedata(0x34);
				
		spi1writeindex(0xB3);				
		spi1writedata(0x0A);
		spi1writedata(0x0A);		
		spi1writedata(0x0A);
		
		/* ### VGL */
		spi1writeindex(0xBA);
		spi1writedata(0x24);
		spi1writedata(0x24);
		spi1writedata(0x24);
		
		spi1writeindex(0xB5);				
		spi1writedata(0x08);
		spi1writedata(0x08);		
		spi1writedata(0x08);

		/* ### VCOM */
		spi1writeindex(0xBE);
		spi1writedata(0x00);				
		spi1writedata(0x50);		
		
		/* ### Enable CMD2_Page0 */
		spi1writeindex(0xF0);		
		spi1writedata(0x55);
		spi1writedata(0xAA);
		spi1writedata(0x52);
		spi1writedata(0x08);
		spi1writedata(0x00); 
		
		/*	Hydis Initial */
		spi1writeindex(0xB1);
		spi1writedata(0xCC);				
		spi1writedata(0x00); 
				
		spi1writeindex(0xBC);
		spi1writedata(0x00); 
		spi1writedata(0x00);						
		spi1writedata(0x00);
				
		spi1writeindex(0xB8);
		spi1writedata(0x01);		
		spi1writedata(0x07);		
		spi1writedata(0x07);
		spi1writedata(0x07);
		
		spi1writeindex(0xCC);
		spi1writedata(0x01);			
		spi1writedata(0x3F);		
		spi1writedata(0x3F);
		
		spi1writeindex(0xB3);
		spi1writedata(0x00); 
		spi1writeindex(0x36);	// reverse display
		spi1writedata(0x03);

		spi1writeindex(0x51);	// PWM
		spi1writedata(0x7F);	// Default brightness. 
		spi1writeindex(0x53);
		spi1writedata(0x24);

		spi1writeindex(0x11);	// SLPOUT
		mdelay(120);
		spi1writeindex(0x29);	// DISPON

	}
	else // Sony
#else		
	{
		msleep(150);
		spi1writeindex(0x3A);
		spi1writedata(0x77);
		
		spi1writeindex(0x3B);
		spi1writedata(0x07);	
		spi1writedata(0x0A);
		spi1writedata(0x0E);
		spi1writedata(0x0A);
		spi1writedata(0x0A);

		spi1writeindex(0x2A);
		spi1writedata(0x00);
		spi1writedata(0x00);
		spi1writedata(0x01);
		spi1writedata(0xDF);

		spi1writeindex(0x2B);
		spi1writedata(0x00);
		spi1writedata(0x00);
		spi1writedata(0x03);
		spi1writedata(0x1F);

		spi1writeindex(0x36);
		spi1writedata(0xD4);

	 	msleep(25);
		
		spi1writeindex(0x11);

		msleep(150);

		spi1writeindex(0x51);
		spi1writedata(0x0); // default brightness : 0
//		spi1writedata(0x6C); // default brightness : 108
		aat1402_set_brightness();

		spi1writeindex(0x55);
		spi1writedata(0x01); // CABC Off, 01:User interface mode, 02:still image mode, 03:moving image mode

		spi1writeindex(0x5E);
		spi1writedata(0x00); // example value. *3)

		spi1writeindex(0x53);
		spi1writedata(0x2C);

		msleep(200);

		spi1writeindex(0x29);	

	}	
#endif
	atomic_set(&ldi_power_state, POWER_ON);
	printk("[LCD] %s() -\n", __func__);
}


void nt35510_ldi_poweroff_sony(void)
{
	printk(" **** %s\n", __func__);
		
	// Display Off
	spi1writeindex(0x28);	// SEQ_DISPOFF_SET

//	if(current_panel==0) // sony
	{
		msleep(25);	
	}

	// SLEEP IN
	spi1writeindex(0x10);	// SEQ_SLEEP IN_SET

	//Wait 120ms
	msleep(150);	
	
	atomic_set(&ldi_power_state, POWER_OFF);
}

void nt35510_lcd_LDO_on(void)
{
	int ret;
	printk("+++ %s\n", __func__);

#if 0	
	ret = regulator_enable( vaux3 ); //VAUX3 - 1.8V
	if ( ret )
		printk("Regulator vaux3 error!!\n");
#else
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0x00, 0x1F);
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0x1, 0x22);
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0xe0, 0x1F);
#endif

 
	mdelay(1);
	ret = regulator_enable( vaux4 ); //VAUX4 - 2.8V
	if ( ret )
		printk("Regulator vaux4 error!!\n");
	
	mdelay(1);
	printk("--- %s\n", __func__);
}

void nt35510_lcd_LDO_off(void)
{
	int ret;
	printk("+++ %s\n", __func__);

	// Reset Release (reset = L)
	gpio_set_value(OMAP_GPIO_MLCD_RST, GPIO_LEVEL_LOW); 
	mdelay(10);

	// VCI 2.8V OFF
	ret = regulator_disable( vaux4 );
	if ( ret )
		printk("Regulator vaux4 error!!\n");
	mdelay(1);

	// VDD3 1.8V OFF
#if 0
	ret = regulator_disable( vaux3 );
	if ( ret )
		printk("Regulator vaux3 error!!\n");
#else
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0x00, 0x1F);
#endif	

	printk("--- %s\n", __func__);
}

void nt35510_lcd_poweroff(void)
{
	if(current_panel==0) // Sony
		nt35510_ldi_poweroff_sony();
	else if(current_panel==1) // Hitach
		nt35510_ldi_poweroff_hitach();

	// turn OFF VCI (2.8V)
	// turn OFF VDD3 (1.8V)
	nt35510_lcd_LDO_off();

             
		omap_mux_init_signal("mcspi1_clk", OMAP_MUX_MODE7 | OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcspi1_simo",  OMAP_MUX_MODE7 | OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcspi1_somi", OMAP_MUX_MODE7 | OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcspi1_cs0", OMAP_MUX_MODE7 | OMAP_PIN_INPUT_PULLDOWN);
		
		
	return;	
}

void nt35510_lcd_poweron(void)
{

       u8 read=0;
	
	omap_mux_init_signal("mcspi1_clk",OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
	omap_mux_init_signal("mcspi1_simo",OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN);
	omap_mux_init_signal("mcspi1_somi", OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLDOWN);
	omap_mux_init_signal("mcspi1_cs0", OMAP_MUX_MODE0 | OMAP_PIN_INPUT_PULLUP);
	

	nt35510_lcd_LDO_on();

	// Activate Reset
	gpio_set_value(OMAP_GPIO_MLCD_RST, GPIO_LEVEL_LOW);	

	mdelay(1);
	gpio_set_value(OMAP_GPIO_MLCD_RST, GPIO_LEVEL_HIGH);
	mdelay(5);
	
	//MLCD pin set to InputPulldown.
	omap_ctrl_writew(0x010C, 0x1c6);

	if(current_panel==0)	// Sony
		{
		nt35510_ldi_poweron_sony();

		twl_i2c_read_u8( TWL4030_MODULE_PM_RECEIVER, &read, TWL4030_VPLL2_DEV_GRP );
		printk( "TWL4030_VPLL2_DEV_GRP = %x\n", read);
              twl_i2c_read_u8( TWL4030_MODULE_PM_RECEIVER, &read, TWL4030_VAUX4_DEV_GRP );
		printk(" TWL4030_VAUX4_DEV_GRP = %x\n", read);
              twl_i2c_read_u8( TWL4030_MODULE_PM_RECEIVER, &read, TWL4030_VAUX3_DEV_GRP );
		printk( "TWL4030_VAUX3_DEV_GRP = %x\n", read);

       
		//while(1);
		}
	else if(current_panel==1) // Hitach
		nt35510_ldi_poweron_hitach();
	return;
}

// [[ backlight control 
static int current_intensity = 108;	// DEFAULT BRIGHTNESS
static DEFINE_SPINLOCK(aat1402_bl_lock);

static void aat1402_set_brightness(void)
{
	printk(KERN_DEBUG" *** aat1402_set_brightness : %d\n", current_intensity);
	//spin_lock_irqsave(&aat1402_bl_lock, flags);
	//spin_lock(&aat1402_bl_lock);

    	spi1writeindex(0x51);
	spi1writedata(current_intensity);

	//spin_unlock_irqrestore(&aat1402_bl_lock, flags);
	//spin_unlock(&aat1402_bl_lock);

}

static int aat1402_bl_get_intensity(struct backlight_device *bd)
{
	return current_intensity;
}

static int aat1402_bl_set_intensity(struct backlight_device *bd)
{
	//unsigned long flags;
	int intensity = bd->props.brightness;
	
	if( intensity < 0 || intensity > 255 )
		return;
	while(atomic_read(&ldi_power_state)==POWER_OFF) 
		mdelay(5);
	
	current_intensity = intensity;

	aat1402_set_brightness();

	return 0;
}

static struct backlight_ops aat1402_bl_ops = {
	.get_brightness = aat1402_bl_get_intensity,
	.update_status  = aat1402_bl_set_intensity,
};
// ]] backlight control 

static int nt35510_spi_probe(struct spi_device *spi)
{
    struct backlight_properties props;
     int status =0;
		
	nt35510lcd_spi = spi;
	nt35510lcd_spi->mode = SPI_MODE_0;
	nt35510lcd_spi->bits_per_word = 9 ;

	printk(" nt35510lcd_spi->chip_select = %x\t, mode = %x\n", nt35510lcd_spi->chip_select,  nt35510lcd_spi->mode);
	printk("ax_speed_hz  = %x\t modalias = %s", nt35510lcd_spi->max_speed_hz, nt35510lcd_spi->modalias );
	
	status = spi_setup(nt35510lcd_spi);
	printk(" spi_setup ret = %x\n",status );
	
	omap_dss_register_driver(&nt35510_driver);
//	led_classdev_register(&spi->dev, &nt35510_backlight_led);
	struct backlight_device *bd;
	bd = backlight_device_register("omap_bl", &spi->dev, NULL, &aat1402_bl_ops, &props);
	bd->props.max_brightness = 255;
	bd->props.brightness = 125;
	
#ifndef CONFIG_FB_OMAP_BOOTLOADER_INIT
	nt35510_lcd_poweron();
#endif

	return 0;
}

static int nt35510_spi_remove(struct spi_device *spi)
{
//	led_classdev_unregister(&nt35510_backlight_led);
	omap_dss_unregister_driver(&nt35510_driver);

	return 0;
}
static void nt35510_spi_shutdown(struct spi_device *spi)
{
	printk("*** First power off LCD.\n");
	nt35510_lcd_poweroff();
	printk("*** power off - backlight.\n");
	gpio_set_value(OMAP_GPIO_LCD_EN_SET, GPIO_LEVEL_LOW);
}

static int nt35510_spi_suspend(struct spi_device *spi, pm_message_t mesg)
{
    //spi_send(spi, 2, 0x01);  /* R2 = 01h */
    //mdelay(40);

#if 0
	nt35510lcd_spi = spi;
	nt35510lcd_spi->mode = SPI_MODE_0;
	nt35510lcd_spi->bits_per_word = 16 ;
	spi_setup(nt35510lcd_spi);

	lcd_poweroff();
	zeus_panel_power_enable(0);
#endif

	return 0;
}

static int nt35510_spi_resume(struct spi_device *spi)
{
	/* reinitialize the panel */
#if 0
	zeus_panel_power_enable(1);
	nt35510lcd_spi = spi;
	nt35510lcd_spi->mode = SPI_MODE_0;
	nt35510lcd_spi->bits_per_word = 16 ;
	spi_setup(nt35510lcd_spi);

	lcd_poweron();
#endif
	return 0;
}

static struct spi_driver nt35510_spi_driver = {
	.probe    = nt35510_spi_probe,
	.remove   = nt35510_spi_remove,
	.shutdown = nt35510_spi_shutdown,
	.suspend  = nt35510_spi_suspend,
	.resume   = nt35510_spi_resume,
	.driver   = {
		.name   = "nt35510_disp_spi",
		.bus    = &spi_bus_type,
		.owner  = THIS_MODULE,
	},
};

static int __init nt35510_lcd_init(void)
{

   	return spi_register_driver(&nt35510_spi_driver);
}

static void __exit nt35510_lcd_exit(void)
{
	return spi_unregister_driver(&nt35510_spi_driver);
}

module_init(nt35510_lcd_init);
module_exit(nt35510_lcd_exit);
MODULE_LICENSE("GPL");
