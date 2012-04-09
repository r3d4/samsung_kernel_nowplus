/*
 * linux/drivers/i2c/chips/twl4030_poweroff.c
 *
 * Power off device
 *
 * Copyright (C) 2008 Nokia Corporation
 *
 * Written by Peter De Schrijver <peter.de-schrijver@nokia.com>
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/pm.h>
#include <linux/i2c/twl.h>
#include <linux/delay.h>
#include <plat/gpio.h>
#include <plat/prcm.h>
#include <plat/mux.h>
#include <plat/microusbic.h>
#if defined(CONFIG_USB_ANDROID)
#include <linux/usb/android_composite.h>
#endif
#include <linux/reboot.h>

#define GPIO_PS_HOLD	25

#define GPIO_MSM_RST	178
#define GPIO_FONE_ACTIVE 140	

void (*__do_forced_modemoff)(void) = NULL;
EXPORT_SYMBOL(__do_forced_modemoff);

#define PWR_P1_SW_EVENTS	0x10
#define PWR_DEVOFF	(1<<0)
#define PWR_STOPON_POWERON (1<<6)

#define CFG_P123_TRANSITION	0x03
#define SEQ_OFFSYNC	(1<<0)

void twl4030_poweroff(void)
{
	u8 uninitialized_var(val);
	int err;
#if 0
	/* Make sure SEQ_OFFSYNC is set so that all the res goes to wait-on */
	err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
				   CFG_P123_TRANSITION);
	if (err) {
		pr_warning("I2C error %d while reading TWL4030 PM_MASTER CFG_P123_TRANSITION\n", err);
		return;
	}

	val |= SEQ_OFFSYNC;
	err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
				    CFG_P123_TRANSITION);
	if (err) {
		pr_warning("I2C error %d while writing TWL4030 PM_MASTER CFG_P123_TRANSITION\n", err);
		return;
	}
#endif
	err = twl_i2c_read_u8(TWL4030_MODULE_PM_MASTER, &val,
				  PWR_P1_SW_EVENTS);
	if (err) {
		pr_warning("I2C error %d while reading TWL4030 PM_MASTER P1_SW_EVENTS\n", err);
		return;
	}

//	val |= PWR_STOPON_POWERON | PWR_DEVOFF;
    val |= PWR_DEVOFF;

	err = twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, val,
				   PWR_P1_SW_EVENTS);

	if (err) {
		pr_warning("I2C error %d while writing TWL4030 PM_MASTER P1_SW_EVENTS\n", err);
		return;
	}

	return;
}


extern void omap_watchdog_reset(void);
extern void tl2796_lcd_poweroff(void);

static void nowplus_poweroff(void)
{

	int usbic_state=0;
    
	printk("\nNOWPLUS BOARD GOING TO SHUTDOWN!!!\n");

#if defined(CONFIG_USB_ANDROID)
	android_usb_set_connected(0);
#endif
	tl2796_lcd_poweroff();

	
	usbic_state= get_real_usbic_state();
	if (usbic_state==MICROUSBIC_TA_CHARGER
			|| usbic_state==MICROUSBIC_USB_CABLE
			|| usbic_state==MICROUSBIC_USB_CHARGER)
    { /* USB/USB CHARGER/CHARGER*/	
		printk("Warmreset by TA or USB or Jtag\n\n");
        preempt_disable();
		local_irq_disable();
		local_fiq_disable();
		/* using watchdog reset */
		omap_watchdog_reset();
		printk(KERN_ERR "Charging ... need to reboot\n");
	}
//	else if(gpio_get_value(OMAP3430_GPIO_ALARM_AP) && !gpio_get_value(OMAP3430_GPIO_PHONE_ACTIVE18))
	// else if(gpio_get_value(OMAP_GPIO_AP_ALARM))
	// {
		// printk(KERN_ERR "Alarm Booting Detecting... need to reboot\n");
		// omap_prcm_arch_reset(1);
	// }
	else 
	{
		printk(KERN_ERR "TWL4030 Power Off\n");
		//powerdown powersupply
		gpio_set_value(OMAP_GPIO_PS_HOLD_PU, 0);
		twl4030_poweroff();
	}
	while(1);

	return;
};


static int __init twl4030_poweroff_init(void)
{
	/*PS HOLD*/
	if(gpio_request(OMAP_GPIO_PS_HOLD_PU, "OMAP_GPIO_PS_HOLD_PU") < 0 ){
    		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_PS_HOLD_PU);
    		return 1;
  	}
    gpio_direction_output(OMAP_GPIO_PS_HOLD_PU, 1);

	pm_power_off = nowplus_poweroff;

	return 0;
}

static void __exit twl4030_poweroff_exit(void)
{
	pm_power_off = NULL;
}

module_init(twl4030_poweroff_init);
module_exit(twl4030_poweroff_exit);

MODULE_ALIAS("i2c:twl4030-poweroff");
MODULE_DESCRIPTION("Triton2 device power off");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter De Schrijver");
