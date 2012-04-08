/*
 * linux/arch/arm/mach-omap2/board-nowplus.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/switch.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/regulator/machine.h>
#include <linux/i2c/twl.h>
#include <linux/usb/android_composite.h>
#include <linux/ctype.h>
#include <linux/bootmem.h>

#include <linux/spi/spi_gpio.h>
#include <linux/spi/spi.h>
#include <linux/spi/tl2796.h>

#include <linux/input.h>
#include <linux/input/matrix_keypad.h>

#include <linux/leds-bd2802.h>
#include <linux/max17040_battery.h>
#include <linux/input/synaptics_i2c_rmi4.h>

#include <mach/hardware.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>



#include <plat/mcspi.h>
#include <plat/mux.h>
#include <plat/timer-gp.h>
#include <plat/board.h>
#include <plat/usb.h>
#include <plat/common.h>
#include <plat/dma.h>
#include <plat/gpmc.h>
#include <plat/onenand.h>
#include <plat/display.h>
#include <plat/control.h>
#include <plat/omap-pm.h>
#include <plat/mux_nowplus.h>
#include <plat/prcm.h>
#include "cm.h"

#include "mux.h"
#include "mmc-twl4030.h"
#include "sdram-nowplus.h"
#include "omap3-opp.h"
#include "pm.h"


#ifdef CONFIG_PM
#include <plat/vrfb.h>
#include <media/videobuf-dma-sg.h>
#include <media/v4l2-device.h>
#include <../../../drivers/media/video/omap/omap_voutdef.h>
#endif

#if 0
#include "../../../drivers/media/video/cam_pmic.h"
#include "../../../drivers/media/video/m4mo.h"
struct m4mo_platform_data nowplus_m4mo_platform_data;
#include "../../../drivers/media/video/s5ka3dfx.h"
struct s5ka3dfx_platform_data nowplus1_s5ka3dfx_platform_data;
#else
// camera build as modules
#define M4MO_DRIVER_NAME                "m-4mo"
#define M4MO_I2C_ADDR   				0x1F
#define S5KA3DFX_DRIVER_NAME            "s5ka3dfx"
#define S5KA3DFX_I2C_ADDR               0xC4>>1
#define CAM_PMIC_DRIVER_NAME            "cam_pmic"
#define CAM_PMIC_I2C_ADDR               0x7D
#endif

#if defined(CONFIG_MACH_NOWPLUS)
/* make easy to register param to sysfs */
#define REGISTER_PARAM(idx,name)     \
static ssize_t name##_show(struct device *dev, struct device_attribute *attr, char *buf)\
{\
	return get_integer_param(idx, buf);\
}\
static ssize_t name##_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)\
{\
	return set_integer_param(idx, buf, size);\
}\
static DEVICE_ATTR(name, S_IRUGO | S_IWUGO | S_IRUSR | S_IWUSR, name##_show, name##_store)

static struct device *param_dev;

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct timezone sec_sys_tz;
EXPORT_SYMBOL(sec_sys_tz);

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);

/* begins - andre.b.kim : added sec_setprio as module { */
struct class *sec_setprio_class;
EXPORT_SYMBOL(sec_setprio_class);

static struct device *sec_set_prio;

void (*sec_setprio_set_value)(const char *buf);
EXPORT_SYMBOL(sec_setprio_set_value);

void (*sec_setprio_get_value)(void);
EXPORT_SYMBOL(sec_setprio_get_value);
/* } ends - andre.b.kim : added sec_setprio as module */
#endif

u32 hw_revision;
EXPORT_SYMBOL(hw_revision);

#define NOWPLUS_CHARGER_ENABLE_GPIO	157
#define NOWPLUS_CHARGING_STATUS_GPIO	16
//#define OMAP_MICROUSBSW_GPIO		10
#define ENABLE_MOVIENAND

#ifndef CONFIG_TWL4030_CORE
#error "no power companion board defined!"
#else
#define TWL4030_USING_BROADCAST_MSG
#endif

#ifdef CONFIG_WL127X_RFKILL
#include <linux/wl127x-rfkill.h>
#endif

extern int always_opp5;
int usbsel = 1;
EXPORT_SYMBOL(usbsel);
void (*usbsel_notify)(int) = NULL;
EXPORT_SYMBOL(usbsel_notify);

extern unsigned get_last_off_on_transaction_id(struct device *dev);
extern int omap34xx_pad_set_config_lcd(u16 pad_pin,u16 pad_val);


/* FIXME: These are not the optimal setup values to be used on 3430sdp*/
static struct prm_setup_vc omap3_setuptime_table = {
#if 1 //original
	.clksetup = 0xff,
	.voltsetup_time1 = 0xfff,
	.voltsetup_time2 = 0xfff,
	.voltoffset = 0xff,
	.voltsetup2 = 0xff,
	.vdd0_on = 0x30,
	.vdd0_onlp = 0x20,
	.vdd0_ret = 0x1e,
	.vdd0_off = 0x00,
	.vdd1_on = 0x2c,
	.vdd1_onlp = 0x20,
	.vdd1_ret = 0x1e,
	.vdd1_off = 0x00,
#else	//mephisto
	.clksetup = 0xff,
	.voltsetup_time1 = 0xfff,
	.voltsetup_time2 = 0xfff,
	.voltoffset = 0xff,
	.voltsetup2 = 0xff,
	.vdd0_on = 0x44,
	.vdd0_onlp = 0x24,
	.vdd0_ret = 0x24,
	.vdd0_off = 0x18,
	.vdd1_on = 0x30,
	.vdd1_onlp = 0x24,
	.vdd1_ret = 0x24,
	.vdd1_off = 0x24,
#endif
};

/* FIXME: These values need to be updated based on more profiling on 3430sdp*/
static struct cpuidle_params omap3_cpuidle_params_table[] = {
#if 1 //original
	/* C1 */
	{1, 2, 2, 5},
	/* C2 */
	{1, 10, 10, 30},
	/* C3 */
	{1, 50, 50, 300},
	/* C4 */
	{1, 1500, 1800, 4000},
	/* C5 */
	{1, 2500, 7500, 12000},
	/* C6 */
	{1, 3000, 8500, 15000},
	/* C7 */
	{1, 10000, 30000, 300000},
#else	//mephisto
		/* C1 */
	{1, 0, 0, 0},
	/* C2 */
	{1, 10, 10, 30},
	/* C3 */
	{1, 50, 50, 300},
	/* C4 */
	{1, 1500, 1800, 13500},
	/* C5 */
	{1, 2500, 7500, 13800},
	/* C6 */
	{1, 3000, 8500, 350000},
	/* C7 */
	{1, 10000, 30000, 500000},
#endif
};


int nowplus_enable_touch_pins(int enable) {
#if 1
// doesn't make sense to disable i2c
    printk("[TOUCH] %s touch pins\n", enable?"enable":"disable");
    if (enable) {
        omap34xx_pad_set_config_lcd(0x01C2, OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_OUTPUT_LOW);
        omap34xx_pad_set_config_lcd(0x01C4, OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_OUTPUT_LOW);
    }
    else {
        omap34xx_pad_set_config_lcd(0x01C2, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW);
        omap34xx_pad_set_config_lcd(0x01C4, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW);
    }
    mdelay(50);
    return 0;
#endif
}
EXPORT_SYMBOL(nowplus_enable_touch_pins);

int nowplus_enable_uart_pins(int enable) {
    printk("[UART] %s uart pins\n", enable?"enable":"disable");
    if (enable) {
        omap34xx_pad_set_config_lcd(0x16e + 0x30, OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT);
        omap34xx_pad_set_config_lcd(0x170 + 0x30, OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT);
    }
    else {
        omap34xx_pad_set_config_lcd(0x16e + 0x30, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT );
        omap34xx_pad_set_config_lcd(0x170 + 0x30, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT);
    }
    mdelay(50);
    return 0;
}

EXPORT_SYMBOL(nowplus_enable_uart_pins);

#ifdef CONFIG_WL127X_RFKILL

int nowplus_bt_hw_enable(void) {
//    return 0;
    printk("[BT] %s bt pins\n", 1?"enable":"disable");
    //cts
    omap34xx_pad_set_config_lcd(0x144 + 0x30, OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP);
    //rts
    omap34xx_pad_set_config_lcd(0x146 + 0x30, OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW);
    //tx
    omap34xx_pad_set_config_lcd(0x148 + 0x30, OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLUP);
    //rx
    omap34xx_pad_set_config_lcd(0x14a + 0x30, OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP);
    mdelay(50);
    return 0;
};

int nowplus_bt_hw_disable(void){
 //   return 0;
    printk("[BT] %s bt pins", 0?"enable":"disable");
    omap34xx_pad_set_config_lcd(0x144 + 0x30, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN);
    omap34xx_pad_set_config_lcd(0x146 + 0x30, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN);
    omap34xx_pad_set_config_lcd(0x148 + 0x30, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN);
    omap34xx_pad_set_config_lcd(0x14a + 0x30, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN);
    mdelay(50);
    return 0;
};

static struct wl127x_rfkill_platform_data wl127x_plat_data = {
	.bt_nshutdown_gpio = OMAP_GPIO_BT_NSHUTDOWN , 	/* Bluetooth Enable GPIO */
	.fm_enable_gpio = -1,		/* FM Enable GPIO */
	.bt_hw_enable = nowplus_bt_hw_enable,
	.bt_hw_disable = nowplus_bt_hw_disable,
};

static struct platform_device nowplus_wl127x_device = {
	.name           = "wl127x-rfkill",
	.id             = -1,
	.dev.platform_data = &wl127x_plat_data,
};
#endif

static int nowplus_twl4030_keymap[] = {
	KEY(0, 0, KEY_FRONT),
	KEY(1, 0, KEY_SEARCH),
	KEY(2, 0, KEY_CAMERA_FOCUS),
	KEY(0, 1, KEY_PHONE),
	KEY(2, 1, KEY_CAMERA),
	KEY(0, 2, KEY_EXIT),
	KEY(1, 2, KEY_VOLUMEUP),
	KEY(2, 2, KEY_VOLUMEDOWN),
};

static struct matrix_keymap_data nowplus_keymap_data = {
	.keymap			= nowplus_twl4030_keymap,
	.keymap_size		= ARRAY_SIZE(nowplus_twl4030_keymap),
};

static struct twl4030_keypad_data nowplus_kp_data = {
	.keymap_data	= &nowplus_keymap_data,
	.rows		= 3,
	.cols		= 3,
	.rep		= 1,
};

static struct resource nowplus_power_key_resource = {
	.start  = OMAP_GPIO_IRQ(OMAP_GPIO_KEY_PWRON),
	.end    = OMAP_GPIO_IRQ(OMAP_GPIO_KEY_PWRON),
	.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
};

static struct platform_device nowplus_power_key_device = {
	.name           = "power_key_device",
	.id             = -1,
	.num_resources  = 1,
	.resource       = &nowplus_power_key_resource,
};

static struct resource nowplus_ear_key_resource = {
                .start  = 0,
                .end    = 0,
                .flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
};

static struct platform_device nowplus_ear_key_device = {
        .name           = "ear_key_device",
        .id             = -1,
        .num_resources  = 1,
        .resource       = &nowplus_ear_key_resource,
};

static struct resource samsung_charger_resources[] = {
	[0] = {
		// USB IRQ
		.start = 0,
		.end   = 0,
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE,
	},
	[1] = {
		// TA IRQ
		.start = 0,
		.end   = 0,
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_LOWEDGE,
	},
	[2] = {
		// CHG_ING_N
		.start = 0,
		.end   = 0,
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_LOWEDGE,
	},
	[3] = {
		// CHG_EN
		.start = 0,
		.end   = 0,
		.flags = IORESOURCE_IRQ,
	},
};
static int samsung_charger_config_data[] = {
	// [ CHECK VF USING ADC ]
	/*   1. ENABLE  (true, flase) */
	true,

	/*   2. ADCPORT (ADCPORT NUM) */
	1,


	// [ SUPPORT CHG_ING IRQ FOR CHECKING FULL ]
	/*   1. ENABLE  (true, flase) */
	true,
};

static int samsung_battery_config_data[] = {
	// [ SUPPORT MONITORING CHARGE CURRENT FOR CHECKING FULL ]
	/*   1. ENABLE  (true, flase) */
	false,
	/*   2. ADCPORT (ADCPORT NUM) */
	4,


	// [ SUPPORT MONITORING TEMPERATURE OF THE SYSTEM FOR BLOCKING CHARGE ]
	/*   1. ENABLE  (true, flase) */
	true,

	/*   2. ADCPORT (ADCPORT NUM) */
	5,
};

static struct platform_device samsung_charger_device = {
	.name           = "secChargerDev",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(samsung_charger_resources),
	.resource       = samsung_charger_resources,

	.dev = {
		.platform_data = &samsung_charger_config_data,
	}
};

static struct platform_device samsung_battery_device = {
	.name           = "secBattMonitor",
	.id             = -1,
	.num_resources  = 0,
	.dev = {
		.platform_data = &samsung_battery_config_data,
	}
};


static struct platform_device samsung_virtual_rtc_device = {
    .name           = "virtual_rtc",
    .id             = -1,
};

static struct platform_device samsung_vibrator_device = {
	.name           = "secVibrator",
	.id             = -1,
	.num_resources  = 0,
};

static struct platform_device samsung_pl_sensor_power_device = {
	.name           = "secPLSensorPower",
	.id             = -1,
	.num_resources  = 0,
};

static struct bd2802_led_platform_data nowplus_led_data = {
	.reset_gpio = OMAP_GPIO_RGB_RST,
	.rgb_time = (3<<6)|(3<<4)|(4), /* refer to application note */
};

static struct omap2_mcspi_device_config nowplus_mcspi_config = {
	.turbo_mode	= 0,
	.single_channel	= 1,	/* 0: slave, 1: master */
};

static struct spi_board_info nowplus_spi_board_info[] __initdata = {
	{
		.modalias		= "tl2796_disp_spi",
		.bus_num		= 1,
		.chip_select		= 0,
		.max_speed_hz		= 375000,
		.controller_data	= &nowplus_mcspi_config,
		.irq			= 0,
	}
};


static struct omap_dss_device nowplus_lcd_device = {
	.name			= "lcd",
	.driver_name		= "tl2796_panel",
	.type			= OMAP_DISPLAY_TYPE_DPI,
	.phy.dpi.data_lines	= 24,
	.platform_enable	= NULL,
	.platform_disable	= NULL,
};

static struct omap_dss_device *nowplus_dss_devices[] = {
	&nowplus_lcd_device,
};

static struct omap_dss_board_info nowplus_dss_data = {
	.get_last_off_on_transaction_id = (void *) get_last_off_on_transaction_id,
	.num_devices	= ARRAY_SIZE(nowplus_dss_devices),
	.devices	= nowplus_dss_devices,
	.default_device	= &nowplus_lcd_device,
};

static struct platform_device nowplus_dss_device = {
	.name	= "omapdss",
	.id		= -1,
	.dev	= {
		.platform_data = &nowplus_dss_data,
	},
};


#ifdef CONFIG_PM
struct vout_platform_data nowplus_vout_data = {
        .set_min_bus_tput = omap_pm_set_min_bus_tput,
        .set_max_mpu_wakeup_lat =  omap_pm_set_max_mpu_wakeup_lat,
        .set_min_mpu_freq = omap_pm_set_min_mpu_freq,
};
#endif


#ifdef CONFIG_ANDROID_RAM_CONSOLE
#define RAM_CONSOLE_START   0x8E000000
#define RAM_CONSOLE_SIZE    0x20000
static struct resource ram_console_resource = {
       .start  = RAM_CONSOLE_START,
       .end    = (RAM_CONSOLE_START + RAM_CONSOLE_SIZE - 1),
       .flags  = IORESOURCE_MEM,
};

static struct platform_device ram_console_device = {
       .name = "ram_console",
       .id = 0,
       .num_resources  = 1,
       .resource       = &ram_console_resource,
};

static inline void nowplus_ramconsole_init(void)
{
	platform_device_register(&ram_console_device);
}

static inline void omap2_ramconsole_reserve_sdram(void)
{
	reserve_bootmem(RAM_CONSOLE_START, RAM_CONSOLE_SIZE, 0);
}
#else
static inline void nowplus_ramconsole_init(void) {}

static inline void omap2_ramconsole_reserve_sdram(void) {}
#endif

static struct gpio_switch_platform_data headset_switch_data = {
       .name = "h2w",
       .gpio =  OMAP_GPIO_DET_3_5, /* Omap3430 GPIO_27 For samsung zeus */
};

static struct platform_device headset_switch_device = {
       .name = "switch-gpio",
       .dev = {
	       .platform_data = &headset_switch_data,
        }
};

static struct platform_device sec_device_dpram = {
	.name	= "dpram-device",
	.id		= -1,
};


struct platform_device sec_sio_switch = {
	.name	= "switch-sio",
	.id		= -1,
};

#define OMAP_GPIO_SYS_DRM_MSECURE 22
static int __init msecure_init(void)
{
        int ret = 0;

#ifdef CONFIG_RTC_DRV_TWL4030
        /* 3430ES2.0 doesn't have msecure/gpio-22 line connected to T2 */
 if (omap_type() == OMAP2_DEVICE_TYPE_GP){
                void __iomem *msecure_pad_config_reg = omap_ctrl_base_get() + 0xA3C;
                int mux_mask = 0x04;
                u16 tmp;
#if 1
                ret = gpio_request(OMAP_GPIO_SYS_DRM_MSECURE, "msecure");
                if (ret < 0) {
                        printk(KERN_ERR  "msecure_init: can't"
                                "reserve GPIO:%d !\n", OMAP_GPIO_SYS_DRM_MSECURE);
                        goto out;
                }
                /*
 *  *                  * TWL4030 will be in secure mode if msecure line from OMAP
 *   *                                   * is low. Make msecure line high in order to change the
 *    *                                                    * TWL4030 RTC time and calender registers.
 *     *                                                                     */
                tmp = __raw_readw(msecure_pad_config_reg);
                tmp &= 0xF8;    /* To enable mux mode 03/04 = GPIO_RTC */
                tmp |= mux_mask;/* To enable mux mode 03/04 = GPIO_RTC */
                __raw_writew(tmp, msecure_pad_config_reg);

                gpio_direction_output(OMAP_GPIO_SYS_DRM_MSECURE, 1);
#endif
        }
out:
#endif
        return ret;
}

static struct platform_device *nowplus_devices[] __initdata = {
	&nowplus_dss_device,
	&headset_switch_device,
#ifdef CONFIG_WL127X_RFKILL
	&nowplus_wl127x_device,
#endif
	&nowplus_power_key_device,
	//&nowplus_ear_key_device,
	&samsung_battery_device,
	&samsung_charger_device,
	&samsung_vibrator_device,   // cdy_100111 add vibrator device
	&sec_device_dpram,
	&samsung_pl_sensor_power_device,
	&sec_sio_switch,
#ifdef CONFIG_RTC_DRV_VIRTUAL
	&samsung_virtual_rtc_device,
#endif
};

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {
        OMAP3_MUX(SDRC_D0,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D1,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D2,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D3,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D4,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D5,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D6,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D7,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D8,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D9,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D10,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D11,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D12,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D13,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D14,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D15,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D16,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D17,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D18,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D19,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D20,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D21,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D22,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D23,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D24,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D25,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D26,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D27,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D28,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D29,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D30,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_D31,             OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_CKE0,            OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_CKE1,            OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SDRC_CLK,             OMAP34XX_MUX_MODE0),
        OMAP3_MUX(SDRC_DQS0,            OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SDRC_DQS1,            OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SDRC_DQS2,            OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SDRC_DQS3,            OMAP34XX_PIN_INPUT_PULLDOWN),

        OMAP3_MUX(GPMC_A1,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_A2,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_A3,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_A4,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_A5,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_A6,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_A7,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_A8,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_A9,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_A10,             OMAP34XX_PIN_INPUT),

        //"GPMC D0"
        OMAP3_MUX(GPMC_D0,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D1"
        OMAP3_MUX(GPMC_D1,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D2"
        OMAP3_MUX(GPMC_D2,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D3"
        OMAP3_MUX(GPMC_D3,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D4"
        OMAP3_MUX(GPMC_D4,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D5"
        OMAP3_MUX(GPMC_D5,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D6"
        OMAP3_MUX(GPMC_D6,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D7"
        OMAP3_MUX(GPMC_D7,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D8"
        OMAP3_MUX(GPMC_D8,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D9"
        OMAP3_MUX(GPMC_D9,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D10"
        OMAP3_MUX(GPMC_D10,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D11"
        OMAP3_MUX(GPMC_D11,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D12"
        OMAP3_MUX(GPMC_D12,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D13"
        OMAP3_MUX(GPMC_D13,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D14"
        OMAP3_MUX(GPMC_D14,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"GPMC D15"
        OMAP3_MUX(GPMC_D15,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
	    //"GPMC CS0"
        OMAP3_MUX(GPMC_NCS0,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_HIGH),

        //"PCMSEL" gpio_52
        OMAP3_MUX(GPMC_NCS1,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT),
        //"PROXIMITY INT" gpio_53
        OMAP3_MUX(GPMC_NCS2,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT  | OMAP34XX_PIN_OFF_INPUT ),
        //"IF CON" gpio_54
        OMAP3_MUX(GPMC_NCS3,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT  | OMAP34XX_PIN_OFF_INPUT ),
        //"DPRAM CS"
        OMAP3_MUX(GPMC_NCS4,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_HIGH),
        //"VF" gpio_56
        OMAP3_MUX(GPMC_NCS5,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        OMAP3_MUX(GPMC_NCS6,            OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLUP ),
        OMAP3_MUX(GPMC_NCS7,            OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLUP),
        OMAP3_MUX(GPMC_CLK,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(GPMC_NADV_ALE,        OMAP34XX_MUX_MODE0),

	    //"GPMC NOE"
        OMAP3_MUX(GPMC_NOE,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_HIGH),
        //"GPMC NWE"
        OMAP3_MUX(GPMC_NWE,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_HIGH),

        OMAP3_MUX(GPMC_NBE0_CLE,        OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_NBE1,            OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(GPMC_NWP,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(GPMC_WAIT0,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP),
        //"BT_nSHUTDOWN" gpio_63
        OMAP3_MUX(GPMC_WAIT1,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"CAMERA VGA RST" gpio_64
        OMAP3_MUX(GPMC_WAIT2,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"FM_nRST" gpio_65
        OMAP3_MUX(GPMC_WAIT3,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT),

        //"LCD MCLK"
        OMAP3_MUX(DSS_PCLK,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD HSYNC"
        OMAP3_MUX(DSS_HSYNC,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD VSYNC"
        OMAP3_MUX(DSS_VSYNC,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD CE"
        OMAP3_MUX(DSS_ACBIAS,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),

        //"LCD DAT<0>"
        OMAP3_MUX(DSS_DATA0,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<1>"
        OMAP3_MUX(DSS_DATA1,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<2>"
        OMAP3_MUX(DSS_DATA2,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<3>"
        OMAP3_MUX(DSS_DATA3,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<4>"
        OMAP3_MUX(DSS_DATA4,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<5>"
        OMAP3_MUX(DSS_DATA5,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<6>"
        OMAP3_MUX(DSS_DATA6,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<7>"
        OMAP3_MUX(DSS_DATA7,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<8>"
        OMAP3_MUX(DSS_DATA8,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<9>"
        OMAP3_MUX(DSS_DATA9,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<10>"
        OMAP3_MUX(DSS_DATA10,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<11>"
        OMAP3_MUX(DSS_DATA11,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<12>"
        OMAP3_MUX(DSS_DATA12,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<13>"
        OMAP3_MUX(DSS_DATA13,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<14>"
        OMAP3_MUX(DSS_DATA14,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<15>"
        OMAP3_MUX(DSS_DATA15,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<16>"
        OMAP3_MUX(DSS_DATA16,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<17>"
        OMAP3_MUX(DSS_DATA17,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<18>"
        OMAP3_MUX(DSS_DATA18,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<19>"
        OMAP3_MUX(DSS_DATA19,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<20>"
        OMAP3_MUX(DSS_DATA20,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<21>"
        OMAP3_MUX(DSS_DATA21,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<22>"
        OMAP3_MUX(DSS_DATA22,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LCD DAT<23>"
        OMAP3_MUX(DSS_DATA23,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),

        //"PIXEL HSYNC"
        OMAP3_MUX(CAM_HS,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"PIXEL VSYNC"
        OMAP3_MUX(CAM_VS,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"PIXEL MCLK"
        OMAP3_MUX(CAM_XCLKA,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"PIXEL PCLK"
        OMAP3_MUX(CAM_PCLK,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"CAMERA RESET" gpio_98
        OMAP3_MUX(CAM_FLD,              OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT),
	    //"LCD REG RST" gpio_99
        OMAP3_MUX(CAM_D0,               OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"ISP INTERRUPT" gpio_100
        OMAP3_MUX(CAM_D1,               OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"CAMERA VGA STBY" gpio_101
        OMAP3_MUX(CAM_D2,               OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        OMAP3_MUX(CAM_D3,               OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),

	    //"CAM DD<0>"
		OMAP3_MUX(CAM_D4,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
		//"CAM DD<1>"
        OMAP3_MUX(CAM_D5,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"CAM DD<2>"
        OMAP3_MUX(CAM_D6,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"CAM DD<3>"
        OMAP3_MUX(CAM_D7,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"CAM DD<4>"
        OMAP3_MUX(CAM_D8,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"CAM DD<5>"
        OMAP3_MUX(CAM_D9,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"CAM DD<6>"
        OMAP3_MUX(CAM_D10,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"CAM DD<7>"
        OMAP3_MUX(CAM_D11,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"PDA ACTIVE" gpio_111
        OMAP3_MUX(CAM_XCLKB,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"CAMERA LEVEL CTRL" gpio_112
        OMAP3_MUX(CAM_WEN,              OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"UART SEL" gpio_113
        OMAP3_MUX(CAM_STROBE,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),

	    OMAP3_MUX(CSI2_DX0,             OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(CSI2_DY0,             OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
	    OMAP3_MUX(CSI2_DX1,             OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
	    //"ACC_INT" gpio_115
        OMAP3_MUX(CSI2_DY1,             OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        //"MCBSP2 Sync"
        OMAP3_MUX(MCBSP2_FSX,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MCBSP2 Clock"
        OMAP3_MUX(MCBSP2_CLKX,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"MCBSP2 Dout"
        OMAP3_MUX(MCBSP2_DR,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MCBSP2 Din"
        OMAP3_MUX(MCBSP2_DX,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),

	    //"MMC1 CLK"
		OMAP3_MUX(SDMMC1_CLK,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
		//"MMC1 CMD"
        OMAP3_MUX(SDMMC1_CMD,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MMC1 D0"
        OMAP3_MUX(SDMMC1_DAT0,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MMC1 D1"
        OMAP3_MUX(SDMMC1_DAT1,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MMC1 D2"
        OMAP3_MUX(SDMMC1_DAT2,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MMC1 D3"
        OMAP3_MUX(SDMMC1_DAT3,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        OMAP3_MUX(SDMMC1_DAT4,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SDMMC1_DAT5,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SDMMC1_DAT6,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SDMMC1_DAT7,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
#if 1	//start with save values
        //"MOVI CLOCK"
 	    OMAP3_MUX(SDMMC2_CLK,           OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
 	    //"MOVI COMMAND"
        OMAP3_MUX(SDMMC2_CMD,           OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 0"
        OMAP3_MUX(SDMMC2_DAT0,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 1"
        OMAP3_MUX(SDMMC2_DAT1,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 2"
        OMAP3_MUX(SDMMC2_DAT2,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 3"
        OMAP3_MUX(SDMMC2_DAT3,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 4"
        OMAP3_MUX(SDMMC2_DAT4,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 5"
        OMAP3_MUX(SDMMC2_DAT5,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 6"
        OMAP3_MUX(SDMMC2_DAT6,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 7"
        OMAP3_MUX(SDMMC2_DAT7,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
#else
        //"MOVI CLOCK"
 	    OMAP3_MUX(SDMMC2_CLK,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
 	    //"MOVI COMMAND"
        OMAP3_MUX(SDMMC2_CMD,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 0"
        OMAP3_MUX(SDMMC2_DAT0,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 1"
        OMAP3_MUX(SDMMC2_DAT1,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 2"
        OMAP3_MUX(SDMMC2_DAT2,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 3"
        OMAP3_MUX(SDMMC2_DAT3,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 4"
        OMAP3_MUX(SDMMC2_DAT4,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 5"
        OMAP3_MUX(SDMMC2_DAT5,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 6"
        OMAP3_MUX(SDMMC2_DAT6,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"MOVI DATA 7"
        OMAP3_MUX(SDMMC2_DAT7,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),



#endif
        //"PHONE ON" gpio_140
        OMAP3_MUX(MCBSP3_DX,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"MOTOR EN" gpio_141
        OMAP3_MUX(MCBSP3_DR,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"TOUCH INT" gpio_142
        OMAP3_MUX(MCBSP3_CLKX,          OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT),
    	OMAP3_MUX(MCBSP3_FSX,           OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),

	    //"BTUART CTS"
        OMAP3_MUX(UART2_CTS,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //suspend
        //OMAP3_MUX(UART2_CTS,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //"BTUART RTS"
        OMAP3_MUX(UART2_RTS,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //suspend
        //OMAP3_MUX(UART2_RTS,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"BTUART TXD"
        OMAP3_MUX(UART2_TX,             OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //suspend
        //OMAP3_MUX(UART2_TX,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //"BTUART RXD"
        OMAP3_MUX(UART2_RX,             OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //suspend
        //OMAP3_MUX(UART2_RX,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),

        OMAP3_MUX(UART1_TX,             OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(UART1_RTS,            OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
        //"USB SEL" gpio_150
        OMAP3_MUX(UART1_CTS,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"RGB RST" gpio_151
        OMAP3_MUX(UART1_RX,             OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"CAMERA EN" gpio_152
        OMAP3_MUX(MCBSP4_CLKX,          OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
    	OMAP3_MUX(MCBSP4_DR,            OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
    	//"PHONE ACTIVE" gpio_154
        OMAP3_MUX(MCBSP4_DX,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        //"MOVI POWER EN" gpio_155
        OMAP3_MUX(MCBSP4_FSX,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"HW REV0" gpio_156
        OMAP3_MUX(MCBSP1_CLKR,          OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        //"TA Enable" gpio_157
        OMAP3_MUX(MCBSP1_FSR,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW ),
        //"HW REV1" gpio_158
        OMAP3_MUX(MCBSP1_DX,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        //"HW REV2" gpio_159
        OMAP3_MUX(MCBSP1_DR,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
		//"WLAN EN" gpio_160
		OMAP3_MUX(MCBSP_CLKS,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
		//"LCD ID" gpio_161
        OMAP3_MUX(MCBSP1_FSX,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT ),
        //"EAR MIC PWR EN" gpio_162
        OMAP3_MUX(MCBSP1_CLKX,          OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT),

        OMAP3_MUX(UART3_CTS_RCTX,       OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLUP),
        OMAP3_MUX(UART3_RTS_SD,         OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLUP),
        //OMAP3_MUX(UART3_RX_IRRX,        OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT ),
        //OMAP3_MUX(UART3_TX_IRTX,        OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        //"UART1 RXD"
        OMAP3_MUX(UART3_RX_IRRX,        OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT ),
        //"UART1 TXD"
        OMAP3_MUX(UART3_TX_IRTX,        OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT),

        // HSUSB SIGNALS - skip
        //"USB CLK"
        OMAP3_MUX(HSUSB0_CLK,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB STP"
        OMAP3_MUX(HSUSB0_STP,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB DIR"
        OMAP3_MUX(HSUSB0_DIR,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB NEXT"
        OMAP3_MUX(HSUSB0_NXT,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB DATA0"
        OMAP3_MUX(HSUSB0_DATA0,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB DATA1"
        OMAP3_MUX(HSUSB0_DATA1,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB DATA2"
        OMAP3_MUX(HSUSB0_DATA2,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB DATA3"
        OMAP3_MUX(HSUSB0_DATA3,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB DATA4"
        OMAP3_MUX(HSUSB0_DATA4,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB DATA5"
        OMAP3_MUX(HSUSB0_DATA5,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB DATA6"
        OMAP3_MUX(HSUSB0_DATA6,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),
        //"USB DATA7"
        OMAP3_MUX(HSUSB0_DATA7,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT),

        //"PWR I2C SCL"
        OMAP3_MUX(I2C1_SCL,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //"PWR I2C SDA"
        OMAP3_MUX(I2C1_SDA,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),

        //"I2C SCL"
        OMAP3_MUX(I2C2_SCL,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //"I2C SDA"
        OMAP3_MUX(I2C2_SDA,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
#if 0
        //"TOUCH I2C SCL"
        OMAP3_MUX(I2C3_SCL,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //"TOUCH I2C SDA"
        OMAP3_MUX(I2C3_SDA,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
#else
// pins will be enabled in rmi driver with call to nowplus_enable_touch_pins(..)
// -> fix touch fail after boot
        //"TOUCH I2C SCL"
        OMAP3_MUX(I2C3_SCL,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"TOUCH I2C SDA"
        OMAP3_MUX(I2C3_SDA,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
#endif
        //"MLCD RST" gpio_170
        OMAP3_MUX(HDQ_SIO,              OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"DISPLAY CLK"
        OMAP3_MUX(MCSPI1_CLK,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"DISPLAY SI"
        OMAP3_MUX(MCSPI1_SIMO,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"DISPLAY SO"
        OMAP3_MUX(MCSPI1_SOMI,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        //"DISPLAY CS"
        OMAP3_MUX(MCSPI1_CS0,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
	    //"WLAN SDIO CMD"
        OMAP3_MUX(MCSPI1_CS1,           OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //OMAP3_MUX(MCSPI1_CS1,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //wlan off
        //"WLAN SDIO CLK"
        OMAP3_MUX(MCSPI1_CS2,           OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //OMAP3_MUX(MCSPI1_CS2,           OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //wlan off
        //"TVOUT SEL" gpio_177
        OMAP3_MUX(MCSPI1_CS3,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT),
        //"PHONE RESET" gpio_178
        OMAP3_MUX(MCSPI2_CLK,           OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_HIGH),
        //"VIB FREQ"
        OMAP3_MUX(MCSPI2_SIMO,          OMAP34XX_MUX_MODE1 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
    	OMAP3_MUX(MCSPI2_SOMI,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(MCSPI2_CS0,           OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLUP),
        OMAP3_MUX(MCSPI2_CS1,           OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),

	    //"SYS nIRQ"
        OMAP3_MUX(SYS_NIRQ,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLUP | OMAP3_WAKEUP_EN),
    	OMAP3_MUX(SYS_CLKOUT2,          OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD0,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD1,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD2,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD3,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD4,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD5,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD6,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD7,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD8,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD9,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD10,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD11,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD12,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD13,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD14,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD15,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD16,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD17,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD18,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD19,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD20,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD21,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD22,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD23,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD24,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD25,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD26,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD27,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD28,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD29,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD30,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD31,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD32,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD33,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD34,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD35,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_MCAD36,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(CHASSIS_CLK26MI,      OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(CHASSIS_NRESPWRON,    OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        //"WARM RESET"
        OMAP3_MUX(CHASSIS_NRESWARW,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        OMAP3_MUX(CHASSIS_NIRQ,                 OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        OMAP3_MUX(CHASSIS_FIQ,                  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        OMAP3_MUX(CHASSIS_ARMIRQ,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(CHASSIS_IVAIRQ,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(CHASSIS_DMAREQ0,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(CHASSIS_DMAREQ1,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(CHASSIS_DMAREQ2,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(CHASSIS_DMAREQ3,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),

        OMAP3_MUX(CHASSIS_NTRST,                OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(CHASSIS_TDI,                  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(CHASSIS_TDO,                  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(CHASSIS_TMS,                  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(CHASSIS_TCK,                  OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(CHASSIS_RTCK,                 OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(CHASSIS_MSTDBY,               OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        OMAP3_MUX(CHASSIS_IDLEREQ,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(CHASSIS_IDLEACK,              OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_OUTPUT_LOW),

        OMAP3_MUX(SAD2D_MWRITE,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_SWRITE,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SAD2D_MREAD,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_SREAD,          OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SAD2D_MBUSFLAG,       OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(SAD2D_SBUSFLAG,       OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),

        //"SR I2C SCL"
        OMAP3_MUX(I2C4_SCL,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //"SR I2C SDA"
        OMAP3_MUX(I2C4_SDA,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),

        OMAP3_MUX(SYS_32K,              OMAP34XX_PIN_INPUT),
        OMAP3_MUX(SYS_CLKREQ,           OMAP34XX_MUX_MODE0),
        OMAP3_MUX(SYS_NRESWARM,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP),

        OMAP3_MUX(SYS_BOOT0,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT),
        //"FM_INT" gpio_3
        OMAP3_MUX(SYS_BOOT1,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        //gpio_4
        OMAP3_MUX(SYS_BOOT2,            OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT),
        //"SYS BOOT3"
        OMAP3_MUX(SYS_BOOT3,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLUP ),
        //"SYS BOOT4"
        OMAP3_MUX(SYS_BOOT4,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLUP ),
        //"SYS BOOT5"
        OMAP3_MUX(SYS_BOOT5,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLUP ),
        //"SYS BOOT6"
        OMAP3_MUX(SYS_BOOT6,            OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLUP ),
        //"SYS OFF"
        OMAP3_MUX(SYS_OFF_MODE,         OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_LOW),
        //"MICRO nINT" gpio_10
        OMAP3_MUX(SYS_CLKOUT1,          OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT | OMAP3_WAKEUP_EN),

        OMAP3_MUX(JTAG_NTRST,           OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(JTAG_TCK,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),
        OMAP3_MUX(JTAG_TMS_TMSC,        OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP),
        OMAP3_MUX(JTAG_TDI,             OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLUP),
        OMAP3_MUX(JTAG_EMU0,            OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        OMAP3_MUX(JTAG_EMU1,            OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //"LOW BAT DET" gpio_12
        OMAP3_MUX(ETK_CLK,                      OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //"NAND INT" gpio_13
        OMAP3_MUX(ETK_CTL,                      OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        //"ALARM_AP" gpio_14
        OMAP3_MUX(ETK_D0,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
        //"INTERRUPT FROM PHONE"  gpio_15
        OMAP3_MUX(ETK_D1,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP3_WAKEUP_EN),
        //"TA nCHG"  gpio_16
        OMAP3_MUX(ETK_D2,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT),
	    //"WLAN SDIO DAT0"
        OMAP3_MUX(ETK_D3,                       OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //OMAP3_MUX(ETK_D3,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //wlan off
        //"WLAN SDIO DAT1"
        OMAP3_MUX(ETK_D4,                       OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //OMAP3_MUX(ETK_D4,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //wlan off
        //"WLAN SDIO DAT2"
        OMAP3_MUX(ETK_D5,                       OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //OMAP3_MUX(ETK_D5,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //wlan off
        //"WLAN SDIO DAT3"
        OMAP3_MUX(ETK_D6,                       OMAP34XX_MUX_MODE2 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP),
        //OMAP3_MUX(ETK_D6,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //wlan off
        //"WLAN IRQ"
        OMAP3_MUX(ETK_D7,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),
        //OMAP3_MUX(ETK_D7,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN), //wlan off
        //"TWL4030 MSECURE"
        OMAP3_MUX(ETK_D8,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT),
        //"MMC1 DETECT"
        OMAP3_MUX(ETK_D9,                       OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP |OMAP3_WAKEUP_EN),
        //"KEY PWRON"
        OMAP3_MUX(ETK_D10,                      OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN | OMAP34XX_PIN_OFF_INPUT_PULLDOWN | OMAP3_WAKEUP_EN),
        //"PS HOLD"
        OMAP3_MUX(ETK_D11,                      OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_OUTPUT | OMAP34XX_PIN_OFF_OUTPUT_HIGH),
        //"EARKEY DETECT"
        OMAP3_MUX(ETK_D12,                      OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT | OMAP3_WAKEUP_EN),
        //"35PI DETECT"
        OMAP3_MUX(ETK_D13,                      OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN | OMAP3_WAKEUP_EN),
        OMAP3_MUX(ETK_D14,                      OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN),

        //"NO USED"
        OMAP3_MUX(ETK_D15,                      OMAP34XX_MUX_MODE7 | OMAP34XX_PIN_INPUT | OMAP34XX_PIN_OFF_INPUT_PULLDOWN),

        OMAP3_MUX(SAD2D_SWAKEUP,        OMAP34XX_MUX_MODE0 | OMAP34XX_PIN_INPUT_PULLDOWN),

        { .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define board_mux       NULL
#endif

static inline void __init nowplus_init_power_key(void)
{
	nowplus_power_key_resource.start = OMAP_GPIO_IRQ(OMAP_GPIO_KEY_PWRON);

	if (gpio_request(OMAP_GPIO_KEY_PWRON, "power_key_irq") < 0 ) {
		printk(KERN_ERR  "\n FAILED TO REQUEST GPIO %d for POWER KEY IRQ \n",OMAP_GPIO_KEY_PWRON);
		return;
	}
	gpio_direction_input(OMAP_GPIO_KEY_PWRON);

}


static inline void __init nowplus_init_battery(void)
{
//	samsung_charger_resources[0].start = IH_USBIC_BASE;
//	samsung_charger_resources[1].start = IH_USBIC_BASE + 1;
	samsung_charger_resources[0].start = OMAP_GPIO_IRQ(OMAP_GPIO_USBSW_NINT);
	samsung_charger_resources[1].start = OMAP_GPIO_IRQ(56); // NC

	samsung_charger_resources[2].start = OMAP_GPIO_IRQ(OMAP_GPIO_CHG_ING_N);
	samsung_charger_resources[3].start = OMAP_GPIO_IRQ(OMAP_GPIO_CHG_EN);
}

static inline void __init nowplus_init_ear_key(void)
{
  nowplus_ear_key_resource.start = OMAP_GPIO_IRQ(OMAP_GPIO_EAR_KEY);
  if(gpio_request(OMAP_GPIO_EAR_KEY, "ear_key_irq") < 0 ){
    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d for POWER KEY IRQ \n",OMAP_GPIO_EAR_KEY);
    return;
  }
  gpio_direction_input(OMAP_GPIO_EAR_KEY);
}

static struct regulator_consumer_supply nowplus_vdda_dac_supply = {
	.supply		= "vdda_dac",
	.dev		= &nowplus_dss_device.dev,
};

static struct regulator_consumer_supply nowplus_vmmc1_supply = {
	.supply			= "vmmc",
};

static struct regulator_consumer_supply nowplus_vmmc2_supply = {
	.supply			= "vmmc",
};

static struct regulator_consumer_supply nowplus_vaux1_supply = {
	.supply			= "vaux1",
};

static struct regulator_consumer_supply nowplus_vaux2_supply = {
	.supply			= "vaux2",
};

static struct regulator_consumer_supply nowplus_vaux3_supply = {
	.supply			= "vaux3",
};

static struct regulator_consumer_supply nowplus_vaux4_supply = {
	.supply			= "vaux4",
};

static struct regulator_consumer_supply nowplus_vpll2_supply = {
	.supply			= "vpll2",
};

static struct regulator_consumer_supply nowplus_vsim_supply = {
	.supply			= "vmmc_aux",
};

static struct regulator_init_data nowplus_vdac = {
	.constraints = {
		.min_uV                 = 1800000,
		.max_uV                 = 1800000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask         = REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &nowplus_vdda_dac_supply,
};

/* VMMC1 for MMC1 card */
static struct regulator_init_data nowplus_vmmc1 = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &nowplus_vmmc1_supply,
};

/* VMMC2 for MoviNAND */
static struct regulator_init_data nowplus_vmmc2 = {
	.constraints = {
		.min_uV			= 1800000,	// !!! -> also have to change TLW VIO voltage to 1.85V
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS ,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &nowplus_vmmc2_supply,
};


/* VAUX1 for PL_SENSOR */
static struct regulator_init_data nowplus_vaux1 = {
	.constraints = {
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies      = &nowplus_vaux1_supply,
};

/* VAUX2 for touch screen */
static struct regulator_init_data nowplus_vaux2 = {
	.constraints = {
		.min_uV			= 2800000,
		.max_uV			= 2800000,
		.apply_uV		= true,
#ifndef CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4 /*macroliu,112.04.06 tw tsp*/
		.always_on		= true,
#endif
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &nowplus_vaux2_supply,
};

/* VAUX3 for display */
static struct regulator_init_data nowplus_vaux3 = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.boot_on		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies	= &nowplus_vaux3_supply,
};

/* VAUX4 for display */
static struct regulator_init_data nowplus_vaux4 = {
	.constraints = {
		.min_uV			= 2800000,
		.max_uV			= 2800000,
		.boot_on		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		=  REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies  = 1,
	.consumer_supplies	= &nowplus_vaux4_supply,
};

/* VPLL2 for digital video outputs */
static struct regulator_init_data nowplus_vpll2 = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		//.apply_uV		= true,
		.boot_on		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &nowplus_vpll2_supply,
};

/* VSIM for WiFi SDIO */
static struct regulator_init_data nowplus_vsim = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		//.apply_uV               = true,
		.boot_on		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &nowplus_vsim_supply,
};

//ZEUS_LCD
static struct omap_lcd_config nowplus_lcd_config __initdata = {
        .ctrl_name      = "internal",
};
//ZEUS_LCD
static struct omap_uart_config nowplus_uart_config __initdata = {
#ifdef CONFIG_SERIAL_OMAP_CONSOLE
	.enabled_uarts	= ( (1 << 0) | (1 << 1) | (1 << 2) ),
#else
	.enabled_uarts	= ( (1 << 0) | (1 << 1) ),
#endif
};

static struct omap_board_config_kernel nowplus_config[] __initdata = {
	 { OMAP_TAG_UART,	&nowplus_uart_config },
	 { OMAP_TAG_LCD,         &nowplus_lcd_config }, //ZEUS_LCD
};
static int nowplus_batt_table[] = {
/* 0 C*/
30800, 29500, 28300, 27100,
26000, 24900, 23900, 22900, 22000, 21100, 20300, 19400, 18700, 17900,
17200, 16500, 15900, 15300, 14700, 14100, 13600, 13100, 12600, 12100,
11600, 11200, 10800, 10400, 10000, 9630,   9280,   8950,   8620,   8310,
8020,   7730,   7460,   7200,   6950,   6710,   6470,   6250,   6040,   5830,
5640,   5450,   5260,   5090,   4920,   4760,   4600,   4450,   4310,   4170,
4040,   3910,   3790,   3670,   3550
};

static struct twl4030_bci_platform_data nowplus_bci_data = {
	.battery_tmp_tbl	= nowplus_batt_table,
	.tblsize		= ARRAY_SIZE(nowplus_batt_table),
};

static struct twl4030_hsmmc_info nowplus_mmc[] = {
	{
		.name		= "external",
		.mmc		= 1,
		.wires		= 4,
		.gpio_wp	= -EINVAL,
	},
	{
		.name		= "internal",
		.mmc		= 2,
		.wires		= 8,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.nonremovable	= true,
	},
	{
		.mmc		= 3,
		.wires		= 4,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
	{}	/* Terminator */
};


static int nowplus_twl_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	nowplus_mmc[0].gpio_cd = OMAP_GPIO_TF_DETECT;
	twl4030_mmc_init(nowplus_mmc);

	/* link regulators to MMC adapters */
	nowplus_vmmc1_supply.dev = nowplus_mmc[0].dev;
#ifdef ENABLE_MOVIENAND
	nowplus_vmmc2_supply.dev = nowplus_mmc[1].dev;
#endif
	nowplus_vsim_supply.dev = nowplus_mmc[2].dev;

	return 0;
}

static struct twl4030_gpio_platform_data nowplus_gpio_data = {
	.gpio_base	= OMAP_MAX_GPIO_LINES,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.setup		= nowplus_twl_gpio_setup,
};

static struct twl4030_usb_data nowplus_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

static struct twl4030_madc_platform_data nowplus_madc_data = {
	.irq_line	= 1,
};

static struct twl4030_ins __initdata sleep_on_seq[] = {
#ifdef TWL4030_USING_BROADCAST_MSG
#if 1
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_RC, RES_TYPE_ALL,0x0,RES_STATE_SLEEP), 4},
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_ALL,0x0,RES_STATE_SLEEP), 2},
#else
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_RC, RES_TYPE_ALL,0x0,RES_STATE_OFF), 2},
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_ALL,0x0,RES_STATE_SLEEP), 2},
#endif

#else
	/* Turn off HFCLKOUT */
	{MSG_SINGULAR(DEV_GRP_P1, RES_HFCLKOUT, RES_STATE_OFF), 2},
	/* Turn OFF VDD1 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD1, RES_STATE_OFF), 2},
	/* Turn OFF VDD2 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD2, RES_STATE_OFF), 2},
	/* Turn OFF VPLL1 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VPLL1, RES_STATE_OFF), 2},
#endif
};

static struct twl4030_script sleep_on_script __initdata = {
	.script	= sleep_on_seq,
	.size	= ARRAY_SIZE(sleep_on_seq),
	.flags	= TRITON_SLEEP_SCRIPT,
};

static struct twl4030_ins wakeup_p12_seq[] __initdata = {
#ifdef TWL4030_USING_BROADCAST_MSG
#if 1
	//{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_RES, RES_TYPE_ALL,0x2,RES_STATE_ACTIVE), 2},
	{MSG_SINGULAR(DEV_GRP_P1, RES_CLKEN, RES_STATE_ACTIVE), 2},
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_ALL,0x0,RES_STATE_ACTIVE), 2},
#else
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_RES, RES_TYPE_ALL,0x2,RES_STATE_ACTIVE), 2},
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_ALL,0x0,RES_STATE_ACTIVE), 2},
#endif
#else
	/* Turn on HFCLKOUT */
	{MSG_SINGULAR(DEV_GRP_P1, RES_HFCLKOUT, RES_STATE_ACTIVE), 2},
	/* Turn ON VDD1 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD1, RES_STATE_ACTIVE), 2},
	/* Turn ON VDD2 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD2, RES_STATE_ACTIVE), 2},
	/* Turn ON VPLL1 */
	{MSG_SINGULAR(DEV_GRP_P1, RES_VPLL1, RES_STATE_ACTIVE), 2},
#endif
};

static struct twl4030_script wakeup_p12_script __initdata = {
	.script = wakeup_p12_seq,
	.size   = ARRAY_SIZE(wakeup_p12_seq),
	.flags  = TWL4030_WAKEUP12_SCRIPT,
};
static struct twl4030_ins wakeup_p3_seq[] __initdata = {
#ifdef TWL4030_USING_BROADCAST_MSG
#if 1
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_RC, RES_TYPE_ALL,0x0,RES_STATE_ACTIVE), 0x37},
#else
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_RC, RES_TYPE_ALL,0x0,RES_STATE_ACTIVE), 2},
#endif
#else
	{MSG_SINGULAR(DEV_GRP_P1, RES_HFCLKOUT, RES_STATE_ACTIVE), 2},
#endif
};

static struct twl4030_script wakeup_p3_script __initdata = {
	.script = wakeup_p3_seq,
	.size   = ARRAY_SIZE(wakeup_p3_seq),
	.flags  = TWL4030_WAKEUP3_SCRIPT,
};

static struct twl4030_ins wrst_seq[] __initdata = {
/*
 * Reset twl4030.
 * Reset VDD1 regulator.
 * Reset VDD2 regulator.
 * Reset VPLL1 regulator.
 * Enable sysclk output.
 * Reenable twl4030.
 */
#if 1
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_OFF), 2},
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD1, RES_STATE_WRST), 0xE},          // VDD1
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD2, RES_STATE_WRST), 0xE},          // VDD2
	{MSG_SINGULAR(DEV_GRP_P1, RES_VPLL1, RES_STATE_WRST), 0x60},           // VPLL1
	{MSG_SINGULAR(DEV_GRP_P1, RES_HFCLKOUT, RES_STATE_ACTIVE), 2},
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_ACTIVE), 2},
#else

#ifdef NOT_SUPPORT_HW_RESET_DURING_SLEEP
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_OFF), 2},
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD1, RES_STATE_WRST), 15},
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD2, RES_STATE_WRST), 15},
	{MSG_SINGULAR(DEV_GRP_P1, RES_VPLL1, RES_STATE_WRST), 0x60},
	{MSG_SINGULAR(DEV_GRP_P1, RES_HFCLKOUT, RES_STATE_ACTIVE), 2},
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_ACTIVE), 2},
#else
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_OFF), 2},
	{MSG_SINGULAR(DEV_GRP_P1, RES_Main_Ref, RES_STATE_WRST), 2},
	{MSG_SINGULAR(DEV_GRP_P1, RES_VIO, RES_STATE_ACTIVE), 15},            // VIO active
	{MSG_SINGULAR(DEV_GRP_P3, RES_CLKEN, RES_STATE_ACTIVE), 2},          // CLKEN active
	{MSG_SINGULAR(DEV_GRP_P1, RES_VPLL1, RES_STATE_ACTIVE), 2},           // VPLL1 active
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD2, RES_STATE_ACTIVE), 15},          // VDD2 active
	{MSG_SINGULAR(DEV_GRP_P1, RES_VDD1, RES_STATE_ACTIVE), 25},          // VDD1 active
// {MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_ALL, RES_TYPE_ALL, RES_TYPE_ALL, RES_STATE_WRST), 75},            // All resources reset
	{MSG_BROADCAST(DEV_GRP_P1, RES_GRP_RC, RES_TYPE_ALL,RES_TYPE_ALL,RES_STATE_ACTIVE), 2},                    // All RC resources active
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_ACTIVE), 2},
#endif
#endif
};
static struct twl4030_script wrst_script __initdata = {
	.script = wrst_seq,
	.size   = ARRAY_SIZE(wrst_seq),
	.flags  = TWL4030_WRST_SCRIPT,
};

static struct twl4030_script *twl4030_scripts[] __initdata = {
	&sleep_on_script,
	&wakeup_p12_script,
	&wakeup_p3_script,
	&wrst_script,
};

static struct twl4030_resconfig twl4030_rconfig[] = {
#if 1
	{ .resource = RES_HFCLKOUT, .devgroup = DEV_GRP_P3, .type = -1, .type2 = -1 },
	{ .resource = RES_VDD1, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
	{ .resource = RES_VDD2, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
	{ .resource = RES_CLKEN, .devgroup = DEV_GRP_P3, .type = -1, .type2 = -1 },
	{ .resource = RES_VIO, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
	{ .resource = RES_REGEN, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VINTANA1, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
	{ .resource = RES_VINTDIG, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
#else
	{ .resource = RES_HFCLKOUT, .devgroup = DEV_GRP_P3, .type = -1, .type2 = -1 },
#ifdef TWL4030_USING_BROADCAST_MSG
	{ .resource = RES_CLKEN, .devgroup = DEV_GRP_P3, .type = -1, .type2 = -1 },
	{ .resource = RES_VAUX1, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VAUX2, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
#ifndef CONFIG_FB_OMAP_BOOTLOADER_INIT
	{ .resource = RES_VAUX3, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VAUX4, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
#endif
	{ .resource = RES_VMMC1, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VMMC2, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VPLL2, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
	{ .resource = RES_VSIM, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
	{ .resource = RES_VDAC, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VUSB_1V5, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VUSB_1V8, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VUSB_3V1, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VINTANA1, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
	{ .resource = RES_VINTANA2, .devgroup = DEV_GRP_NULL, .type = -1, .type2 = -1 },
	{ .resource = RES_VINTDIG, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
#endif
	{ .resource = RES_VDD1, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
	{ .resource = RES_VDD2, .devgroup = DEV_GRP_P1, .type = -1, .type2 = -1 },
#endif
	{ 0, 0},
};

static struct twl4030_power_data nowplus_t2scripts_data __initdata = {
	.scripts	= twl4030_scripts,
	.num		= ARRAY_SIZE(twl4030_scripts),
	.resource_config = twl4030_rconfig,
};

static struct twl4030_codec_audio_data nowplus_audio_data = {
	.audio_mclk = 26000000,
};

static struct twl4030_codec_data nowplus_codec_data = {
	.audio_mclk = 26000000,
	.audio = &nowplus_audio_data,
};

static struct twl4030_platform_data nowplus_twl_data __initdata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,

	.bci		= &nowplus_bci_data,
	.madc		= &nowplus_madc_data,
	.usb		= &nowplus_usb_data,
	.gpio		= &nowplus_gpio_data,
	.keypad		= &nowplus_kp_data,
	.codec		= &nowplus_codec_data,
	.power		= &nowplus_t2scripts_data,

	.vmmc1		= &nowplus_vmmc1,
#ifdef ENABLE_MOVIENAND
	.vmmc2		= &nowplus_vmmc2, //temp disable movinand
#endif
	.vsim		= &nowplus_vsim,
	.vdac		= &nowplus_vdac,
	.vaux1		= &nowplus_vaux1,
	.vaux2		= &nowplus_vaux2,
	.vaux3		= &nowplus_vaux3,
	.vaux4		= &nowplus_vaux4,
	.vpll2		= &nowplus_vpll2,
};

u8 fsa9480_get_charger_status(void);
static int nowplus_battery_online(void) {
	return 1;
};

static int nowplus_charger_online(void) {
	return  fsa9480_get_charger_status();
};

static int nowplus_charger_enable(void) {

	if( fsa9480_get_charger_status() )
	{
		gpio_direction_output(OMAP_GPIO_CHG_EN, 0);
		set_irq_type (OMAP_GPIO_IRQ(OMAP_GPIO_CHG_ING_N), IRQ_TYPE_EDGE_RISING);
        return 1;
	}

	return 0;
};

static void nowplus_charger_disable(void)
{
	set_irq_type (OMAP_GPIO_IRQ(OMAP_GPIO_CHG_ING_N), IRQ_TYPE_NONE);
	gpio_direction_output(OMAP_GPIO_CHG_EN, 1);
}

static int nowplus_charger_done(void)
{
	return 0;
}

static struct max17040_platform_data nowplus_max17040_data = {
	.battery_online = &nowplus_battery_online,
	.charger_online = &nowplus_charger_online,
	.charger_enable = &nowplus_charger_enable,
	.charger_done = &nowplus_charger_done,
	.charger_disable = &nowplus_charger_disable,
};

static struct i2c_board_info __initdata nowplus_i2c1_boardinfo[] = {
	{
		I2C_BOARD_INFO("twl5030", 0x48),
		.flags = I2C_CLIENT_WAKE,
		.irq = INT_34XX_SYS_NIRQ,
		.platform_data = &nowplus_twl_data,
	},
};

static struct i2c_board_info __initdata nowplus_i2c2_boardinfo[] = {
	{
		I2C_BOARD_INFO(S5KA3DFX_DRIVER_NAME, S5KA3DFX_I2C_ADDR),
//		.platform_data = &nowplus1_s5ka3dfx_platform_data,
	},
	{
		I2C_BOARD_INFO(M4MO_DRIVER_NAME, M4MO_I2C_ADDR),
//		.platform_data = &nowplus_m4mo_platform_data,
	},
	{
		I2C_BOARD_INFO(CAM_PMIC_DRIVER_NAME, CAM_PMIC_I2C_ADDR),
	},
	{
		I2C_BOARD_INFO("kxsd9", 0x18),
	},
	{
		I2C_BOARD_INFO("Si4709_driver", 0x10),
	},
	{
		I2C_BOARD_INFO("BD2802", 0x1A),
		.platform_data = &nowplus_led_data,
	},
/*	{
		I2C_BOARD_INFO("akm8976", 0x1c),
	},*/
#if defined(CONFIG_FSA9480_MICROUSB)
	{
		I2C_BOARD_INFO("fsa9480", 0x25),
		.flags = I2C_CLIENT_WAKE,
		.irq = OMAP_GPIO_IRQ(OMAP_GPIO_USBSW_NINT),
	},
#endif
	{
		I2C_BOARD_INFO("max17040", 0x36),
		.platform_data = &nowplus_max17040_data,
	},
	{
		I2C_BOARD_INFO("PL_driver", 0x44),
	},
	{
		I2C_BOARD_INFO("max9877", 0x4d),
	},
};

static struct synaptics_rmi4_platform_data synaptics_platform_data[] = {
	{
		.name = "synaptics",
		.irq_number = OMAP_GPIO_IRQ(OMAP_GPIO_TOUCH_IRQ),
		.irq_type = IRQF_TRIGGER_FALLING,
		.x_flip = false,
		.y_flip = false ,
		.regulator_en = true,
	}
};

static struct i2c_board_info __initdata nowplus_i2c3_boardinfo[] = {
	{
		I2C_BOARD_INFO("synaptics_rmi4_i2c", 0x2C),
                .irq = 0,
                .platform_data = &synaptics_platform_data,
	},
};

static int __init nowplus_i2c_init(void)
{
	omap_register_i2c_bus(2, 200, nowplus_i2c2_boardinfo,
			ARRAY_SIZE(nowplus_i2c2_boardinfo));

	omap_register_i2c_bus(1, 400, nowplus_i2c1_boardinfo,
			ARRAY_SIZE(nowplus_i2c1_boardinfo));

	//omap_register_i2c_bus(3, 100, nowplus_i2c3_boardinfo,
	omap_register_i2c_bus(3, 400, nowplus_i2c3_boardinfo,
			ARRAY_SIZE(nowplus_i2c3_boardinfo));
	return 0;
}

#if defined(CONFIG_MTD_ONENAND_OMAP2) || \
	defined(CONFIG_MTD_ONENAND_OMAP2_MODULE)
/*
Adresses of the BML partitions

minor position size blocks id

  1: 0x00000000-0x00040000 0x00040000      2        0
  2: 0x00040000-0x00640000 0x00600000     48        1
  3: 0x00640000-0x00780000 0x00140000     10        2
  4: 0x00780000-0x008c0000 0x00140000     10        3
  5: 0x008c0000-0x00dc0000 0x00500000     40        4
  6: 0x00dc0000-0x012c0000 0x00500000     40        5
  7: 0x012c0000-0x02640000 0x01380000    156        6   initrd
  8: 0x02640000-0x0da40000 0x0b400000   1440        7   factoryfs
  9: 0x0da40000-0x1f280000 0x11840000   2242        8   datafs
 10: 0x1f280000-0x1f380000 0x00100000      8        9
 11: 0x1f380000-0x1f480000 0x00100000      8       10

 
 Partition IDs as seen by Linux

major minor #blocks name

139     0     513280 tbmlc
139     1        256 tbml1
139     2       6144 tbml2
139     3       1280 tbml3
139     4       1280 tbml4
139     5       5120 tbml5
139     6       5120 tbml6
139     7      19968 tbml7
139     8     184320 tbml8
139     9     286976 tbml9
139    10       1024 tbml10
139    11       1024 tbml11
137     0     513280 bml0/c
137     1        256 bml1
137     2       6144 bml2
137     3       1280 bml3
137     4       1280 bml4
137     5       5120 bml5
137     6       5120 bml6
137     7      19968 bml7
137     8     184320 bml8
137     9     286976 bml9
137    10       1024 bml10
137    11       1024 bml11
138     2       5796 stl2
138     5       4674 stl5
138     9     275520 stl9



 Data from the Firmware Image contained on OneNAND

bml1   = boot.bin 262144 byte
bml2   = ??? userdata? 6291456 byte
bml3   = Sbl.bin 1310720 byte - (offset: 001000B80000000000000000 +bml3 = Sbl.bin)
bml4   = ??? not to be found in flash-image 1310720 byte
bml5   = params.lfs without a small sub-header and huge trailer (trailer seems to be the same content again -.-) 5242880 byte
bml6   = kernel image zImage (j4fs,rw) 5242880 byte
bml7   = initrd.cramfs 20447232 byte
bml8   = factoryfs.cramfs 188743680 byte
bml9   = datafs.rfs?! datafs is 1 MB bigger but content is similar 293863424 byte
bml10  = EMPTY (bad block reserve?) 1048576 byte
bml11  = EMPTY (bad block reserve?) 1048576 byte
bml0!c = boot.bin + FFFF trailer 525598720 byte
bml0/c = boot.bin + FFFF trailer 525598720 byte
stl2   = /csa (rfs,rw) 5935104 byte
stl5   = params.lfs (j4fs,rw) 4786176 byte
stl9   = datafs.rfs userdata? /mnt/rsv (rfs,rw) 282132480 byte

*/
static struct mtd_partition onenand_partitions[] = {
	{
		.name           = "initrd",
		.offset         = 0x012c0000,
		.size           = 0x01380000,
		//.mask_flags     = MTD_WRITEABLE,	/* Force read-only */
	},
	{
		.name           = "system",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 0x0b400000,
	},
	{
		.name           = "data",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 0x11340000,
		//.size           = 0x0e640000,
	},
    {
		.name           = "cache",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 0x00500000,
		//.size           = 0x03200000,
	},

};

static struct omap_onenand_platform_data board_onenand_data = {
	.cs		= 0,
	//.gpio_irq	= OMAP_GPIO_AP_NAND_INT,
    .dma_channel = -1,  /* disable DMA in OMAP OneNAND driver */
	.parts		= onenand_partitions,
	.nr_parts	= ARRAY_SIZE(onenand_partitions),
	.flags		= ONENAND_SYNC_READWRITE,
};

static void __init board_onenand_init(void)
{
	gpmc_onenand_init(&board_onenand_data);
}

#else

static inline void board_onenand_init(void)
{
}

#endif

static void config_wlan_gpio(void)
{
        int ret = 0;

#if 1//TI HS.Yoon 20100827 for enabling WLAN_IRQ wakeup
	omap_writel(omap_readl(0x480025E8)|0x410C0000, 0x480025E8);
	omap_writew(0x10C, 0x48002194);
#endif
        ret = gpio_request(OMAP_GPIO_WLAN_IRQ, "wifi_irq");
        if (ret < 0) {
                printk(KERN_ERR  "%s: can't reserve GPIO: %d\n", __func__,
                        OMAP_GPIO_WLAN_IRQ);
                return;
        }
        ret = gpio_request(OMAP_GPIO_WLAN_EN, "wifi_pmena");
        if (ret < 0) {
                printk(KERN_ERR  "%s: can't reserve GPIO: %d\n", __func__,
                        OMAP_GPIO_WLAN_EN);
                gpio_free(OMAP_GPIO_WLAN_EN);
                return;
        }
        gpio_direction_input(OMAP_GPIO_WLAN_IRQ);
        gpio_direction_output(OMAP_GPIO_WLAN_EN, 0);

#ifdef __TRISCOPE__
        gpio_set_value(OMAP_GPIO_WLAN_EN, 1);
#endif
}

static void config_camera_gpio(void)
{
	if (gpio_request(OMAP_GPIO_CAMERA_LEVEL_CTRL,"OMAP_GPIO_CAMERA_LEVEL_CTRL") != 0)
	{
		printk("Could not request GPIO %d\n", OMAP_GPIO_CAMERA_LEVEL_CTRL);
		return;
	}

	if (gpio_request(OMAP_GPIO_CAM_EN,"OMAP_GPIO_CAM_EN") != 0)
	{
		printk("Could not request GPIO %d\n", OMAP_GPIO_CAM_EN);
		return;
	}

	if (gpio_request(OMAP_GPIO_ISP_INT,"OMAP_GPIO_ISP_INT") != 0)
	{
		printk("Could not request GPIO %d\n", OMAP_GPIO_ISP_INT);
		return;
	}

	if (gpio_request(OMAP_GPIO_CAM_RST,"OMAP_GPIO_CAM_RST") != 0)
	{
		printk("Could not request GPIO %d\n", OMAP_GPIO_CAM_RST);
		return;
	}

	if (gpio_request(OMAP_GPIO_VGA_STBY,"OMAP_GPIO_VGA_STBY") != 0)
	{
		printk("Could not request GPIO %d\n", OMAP_GPIO_VGA_STBY);
		return;
	}

	if (gpio_request(OMAP_GPIO_VGA_RST,"OMAP_GPIO_VGA_RST") != 0)
	{
		printk("Could not request GPIO %d", OMAP_GPIO_VGA_RST);
		return;
	}

	gpio_direction_output(OMAP_GPIO_CAMERA_LEVEL_CTRL, 0);
	gpio_direction_output(OMAP_GPIO_CAM_EN, 0);
	gpio_direction_input(OMAP_GPIO_ISP_INT);
	gpio_direction_output(OMAP_GPIO_CAM_RST, 0);
	gpio_direction_output(OMAP_GPIO_VGA_STBY, 0);
	gpio_direction_output(OMAP_GPIO_VGA_RST, 0);


	gpio_free(OMAP_GPIO_CAMERA_LEVEL_CTRL);
	gpio_free(OMAP_GPIO_CAM_EN);
	gpio_free(OMAP_GPIO_ISP_INT);
	gpio_free(OMAP_GPIO_CAM_RST);
	gpio_free(OMAP_GPIO_VGA_STBY);
	gpio_free(OMAP_GPIO_VGA_RST);

}

static int __init wl127x_vio_leakage_fix(void)
{
	int ret = 0;

	ret = gpio_request(OMAP_GPIO_BT_NSHUTDOWN, "wl127x_bten");
	if (ret < 0) {
		printk(KERN_ERR  "wl127x_bten gpio_%d request fail",
						OMAP_GPIO_BT_NSHUTDOWN);
		goto fail;
	}

	gpio_direction_output(OMAP_GPIO_BT_NSHUTDOWN, 1);
	mdelay(10);
	gpio_direction_output(OMAP_GPIO_BT_NSHUTDOWN, 0);
	udelay(64);

	gpio_free(OMAP_GPIO_BT_NSHUTDOWN);

fail:
	return ret;
}

static void mod_clock_correction(void)
{
    cm_write_mod_reg(0x00,OMAP3430ES2_SGX_MOD,CM_CLKSEL);
    cm_write_mod_reg(0x04,OMAP3430_CAM_MOD,CM_CLKSEL);
}

int config_twl4030_resource_remap( void )
{
	int ret = 0;
	printk("Start config_twl4030_resource_remap\n");
#if 1
	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_CLKEN_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_CLKEN_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VDD1_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VDD1_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VDD2_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VDD2_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VPLL1_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VPLL1_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VPLL2_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VPLL2_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_HFCLKOUT_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_HFCLKOUT_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VINTANA2_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VINTANA2_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_SLEEP, TWL4030_VINTDIG_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VINTDIG_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_SLEEP, TWL4030_VIO_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VIO_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VSIM_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VSIM_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VAUX1_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VAUX1_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_ACTIVE, TWL4030_VAUX2_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VAUX2_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VAUX3_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VAUX3_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VAUX4_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VAUX4_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VMMC1_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VMMC1_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VMMC2_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VMMC2_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VDAC_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VDAC_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VUSB1V5_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VUSB1V5_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VUSB1V8_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VUSB1V8_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VUSB3V1_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VUSB3V1_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_REGEN_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_REGEN_REMAP\n");

	// [ - In order to prevent damage to the PMIC(TWL4030 or 5030),
	//     VINTANA1 should maintain active state even though the system is in offmode.
	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_ACTIVE, TWL4030_VINTANA1_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VINTANA1_REMAP\n");
	// ]

#if 1//TI HS.Yoon 20101018 for increasing VIO level to 1.85V
	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x1, TWL4030_VIO_VSEL );
	if (ret)
		printk(" board-file: fail to set TWL4030_VIO_VSEL\n");
#endif
#else
	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VDD1_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VDD1_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VDD2_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VDD2_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VPLL1_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VPLL1_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VPLL2_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VPLL2_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_HFCLKOUT_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_HFCLKOUT_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VINTANA2_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VINTANA2_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_SLEEP, TWL4030_VINTDIG_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VINTDIG_REMAP\n");

	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_ACTIVE, TWL4030_VSIM_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VSIM_REMAP\n");

	// [ - In order to prevent damage to the PMIC(TWL4030 or 5030),
	//     VINTANA1 should maintain active state even though the system is in offmode.
	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_ACTIVE, TWL4030_VINTANA1_REMAP );
	if (ret)
		printk(" board-file: fail to set reousrce remap TWL4030_VINTANA1_REMAP\n");
	// ]

#if 1//TI HS.Yoon 20101018 for increasing VIO level to 1.85V
	ret = twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x1, TWL4030_VIO_VSEL );
	if (ret)
		printk(" board-file: fail to set TWL4030_VIO_VSEL\n");
#endif
#endif
	return ret;
}

static struct omap_musb_board_data musb_board_data = {
	.interface_type		= MUSB_INTERFACE_ULPI,
	.mode			= MUSB_OTG,
	.power			= 100,
};
#define POWERUP_REASON "androidboot.mode"
int get_powerup_reason(char *str)
{
	extern char *saved_command_line;
	char *next, *start = NULL;
	int i;

	i = strlen(POWERUP_REASON);
	next = saved_command_line;

	while ((next = strchr(next, 'a')) != NULL) {
		if (!strncmp(next, POWERUP_REASON, i)) {
			start = next;
			break;
		} else {
			next++;
		}
	}
	if (!start)
		return -EPERM;
	i = 0;
	start = strchr(start, '=') + 1;
	while (*start != ' ') {
		str[i++] = *start++;
		if (i > 14) {
			printk(KERN_INFO "Invalid Powerup reason\n");
			return -EPERM;
		}
	}
	str[i] = '\0';
	return 0;
}


static inline void __init nowplus_init_lcd(void)
{
	printk(KERN_ERR "\n%s\n",__FUNCTION__);
	if (gpio_request(OMAP_GPIO_MLCD_RST, "OMAP_GPIO_MLCD_RST") < 0 ) {
		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d\n",OMAP_GPIO_MLCD_RST);
		return;
	}
	gpio_direction_output(OMAP_GPIO_MLCD_RST,1);

	if (gpio_request(OMAP_GPIO_LCD_REG_RST, "OMAP_GPIO_LCD_REG_RST") < 0 ) {
		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d\n",OMAP_GPIO_LCD_REG_RST);
		return;
	}
	gpio_direction_input(OMAP_GPIO_LCD_REG_RST);

	if (gpio_request(OMAP_GPIO_LCD_ID, "OMAP_GPIO_LCD_ID") < 0 ) {
		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d\n",OMAP_GPIO_LCD_ID);
		return;
	}
	gpio_direction_output(OMAP_GPIO_LCD_ID,0);

}


static inline void __init nowplus_init_fmradio(void)
{

  if(gpio_request(OMAP_GPIO_FM_nRST, "OMAP_GPIO_FM_nRST") < 0 ){
    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d\n",OMAP_GPIO_FM_nRST);
    return;
  }
  gpio_direction_output(OMAP_GPIO_FM_nRST, 0);
 // mdelay(1);
 // gpio_direction_output(OMAP_GPIO_FM_nRST, 1); //temp disable


  if(gpio_request(OMAP_GPIO_FM_INT, "OMAP_GPIO_FM_INT") < 0 ){
    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d\n",OMAP_GPIO_FM_INT);
    return;
  }
  gpio_direction_input(OMAP_GPIO_FM_INT);
}

static inline void __init nowplus_init_mmc1(void)
{
	if(gpio_request(OMAP_GPIO_TF_DETECT, "OMAP_GPIO_TF_DETECT") < 0 ){
	    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d\n",OMAP_GPIO_TF_DETECT);
	    return;
	  }
	  gpio_direction_input(OMAP_GPIO_TF_DETECT);
	  set_irq_type(OMAP_GPIO_IRQ(OMAP_GPIO_TF_DETECT), IRQ_TYPE_EDGE_BOTH);

	return;
}

static inline void __init nowplus_init_movi_nand(void)
{
  if(gpio_request(OMAP_GPIO_MOVI_EN, "OMAP_GPIO_MOVI_EN") < 0 ){
    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_MOVI_EN);
    return;
  }
  gpio_direction_output(OMAP_GPIO_MOVI_EN,0);
}

static inline void __init nowplus_init_sound(void)
{
  if(gpio_request(OMAP_GPIO_PCM_SEL, "OMAP_GPIO_PCM_SEL") < 0 ){
    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_PCM_SEL);
    return;
  }
  gpio_direction_output(OMAP_GPIO_PCM_SEL,0);

  if(gpio_request(OMAP_GPIO_TVOUT_SEL, "OMAP_GPIO_TVOUT_SEL") < 0 ){
    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_TVOUT_SEL);
    return;
  }
  gpio_direction_output(OMAP_GPIO_TVOUT_SEL,0);
}

static inline void __init nowplus_init_PL(void)
{
  if(gpio_request(OMAP_GPIO_PS_OUT, "OMAP_GPIO_PS_OUT") < 0 ){
    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_PS_OUT);
    return;
  }
  gpio_direction_input(OMAP_GPIO_PS_OUT);
}

static inline void __init nowplus_init_earphone(void)
{
  if(gpio_request(OMAP_GPIO_DET_3_5, "OMAP_GPIO_DET_3_5") < 0 ){
    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_DET_3_5);
    return;
  }
  gpio_direction_input(OMAP_GPIO_DET_3_5);


  if(gpio_request(OMAP_GPIO_EAR_MIC_LDO_EN, "OMAP_GPIO_EAR_MIC_LDO_EN") < 0 ){
    printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_EAR_MIC_LDO_EN);
    return;
  }
  gpio_direction_output(OMAP_GPIO_EAR_MIC_LDO_EN,0);

  gpio_free( OMAP_GPIO_DET_3_5 );
  gpio_free( OMAP_GPIO_EAR_MIC_LDO_EN );
}

static inline void __init nowplus_init_platform(void)
{
	if(gpio_request(OMAP_GPIO_CHG_ING_N, "OMAP_GPIO_CHG_ING_N") < 0 ){
    		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_CHG_ING_N);
    		return;
  	}
	gpio_direction_input(OMAP_GPIO_CHG_ING_N);

	if(gpio_request(OMAP_GPIO_CHG_EN, "OMAP_GPIO_CHG_EN") < 0 ){
    		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_CHG_EN);
    		return;
  	}

	/*CHG_EN low or high depend on bootloader configuration*/
	if(gpio_get_value(OMAP_GPIO_CHG_EN))
	{
		gpio_direction_output(OMAP_GPIO_CHG_EN, 1);
	}
	else
	{
		gpio_direction_output(OMAP_GPIO_CHG_EN, 0);
	}

	/*NAND INT_GPIO*/
	if(gpio_request(OMAP_GPIO_AP_NAND_INT, "OMAP_GPIO_AP_NAND_INT") < 0 ){
    		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_AP_NAND_INT);
    		return;
  	}
	gpio_direction_input(OMAP_GPIO_AP_NAND_INT);

	if(gpio_request(OMAP_GPIO_IF_CON_SENSE, "OMAP_GPIO_IF_CON_SENSE") < 0 ){
    		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_IF_CON_SENSE);
    		return;
  	}
	gpio_direction_input(OMAP_GPIO_IF_CON_SENSE);

	if(gpio_request(OMAP_GPIO_VF, "OMAP_GPIO_VF") < 0 ){
    		printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d \n",OMAP_GPIO_VF);
    		return;
  	}
	gpio_direction_input(OMAP_GPIO_VF);

}


static void __init get_board_hw_rev(void)
{
	int ret;

	ret = gpio_request( OMAP_GPIO_HW_REV0, "HW_REV0");
	if (ret < 0)
	{
		printk( "fail to get gpio : %d, res : %d\n", OMAP_GPIO_HW_REV0, ret );
		return;
	}

	ret = gpio_request( OMAP_GPIO_HW_REV1, "HW_REV1");
	if (ret < 0)
	{
		printk( "fail to get gpio : %d, res : %d\n", OMAP_GPIO_HW_REV1, ret );
		return;
	}

	ret = gpio_request( OMAP_GPIO_HW_REV2, "HW_REV2");
	if (ret < 0)
	{
		printk( "fail to get gpio : %d, res : %d\n", OMAP_GPIO_HW_REV2, ret );
		return;
	}

	gpio_direction_input( OMAP_GPIO_HW_REV0 );
	gpio_direction_input( OMAP_GPIO_HW_REV1 );
	gpio_direction_input( OMAP_GPIO_HW_REV2 );

	hw_revision = gpio_get_value(OMAP_GPIO_HW_REV0);
	hw_revision |= (gpio_get_value(OMAP_GPIO_HW_REV1) << 1);
	hw_revision |= (gpio_get_value(OMAP_GPIO_HW_REV2) << 2);

	gpio_free( OMAP_GPIO_HW_REV0 );
	gpio_free( OMAP_GPIO_HW_REV1 );
	gpio_free( OMAP_GPIO_HW_REV2 );

	// safe mode reconfiguration
	//omap_ctrl_writew(OMAP34XX_MUX_MODE7, 0x018C);
	//omap_ctrl_writew(OMAP34XX_MUX_MODE7, 0x0190);
	//omap_ctrl_writew(OMAP34XX_MUX_MODE7, 0x0192);

	// REV03 : 000b, REV04 : 001b, REV05 : 010b, REV06 : 011b
	printk("BOARD HW REV : %d\n", hw_revision);
}

/* integer parameter get/set functions */
static ssize_t set_integer_param(param_idx idx, const char *buf, size_t size)
{
	int ret = 0;
	char *after;
	unsigned long value = simple_strtoul(buf, &after, 10);
	unsigned int count = (after - buf);
	if (*after && isspace(*after))
		count++;
	if (count == size) {
		ret = count;

		if (sec_set_param_value)	sec_set_param_value(idx, &value);
	}

	return ret;
}
static ssize_t get_integer_param(param_idx idx, char *buf)
{
	int value;

	if (sec_get_param_value)	sec_get_param_value(idx, &value);

	return sprintf(buf, "%d\n", value);
}
REGISTER_PARAM(__DEBUG_LEVEL, debug_level);
REGISTER_PARAM(__DEBUG_BLOCKPOPUP, block_popup);
static void *dev_attr[] = {
	&dev_attr_debug_level,
	&dev_attr_block_popup,
};
static int nowplus_param_sysfs_init(void)
{
	int ret, i = 0;

	param_dev = device_create(sec_class, NULL, MKDEV(0,0), NULL, "param");

	if (IS_ERR(param_dev)) {
		pr_err("Failed to create device(param)!\n");
		return PTR_ERR(param_dev);
	}

	for (; i < ARRAY_SIZE(dev_attr); i++) {
		ret = device_create_file(param_dev, dev_attr[i]);
		if (ret < 0) {
			pr_err("Failed to create device file(%s)!\n",
				((struct device_attribute *)dev_attr[i])->attr.name);
			goto fail;
		}
	}

	return 0;

fail:
	for (--i; i >= 0; i--)
		device_remove_file(param_dev, dev_attr[i]);

	return -1;
}

/* begins - andre.b.kim : added sec_setprio as module { */
static ssize_t sec_setprio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (sec_setprio_get_value) {
		sec_setprio_get_value();
	}

	return 0;
}

static ssize_t sec_setprio_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	if (sec_setprio_set_value) {
		sec_setprio_set_value(buf);
	}

	return size;
}

static DEVICE_ATTR(sec_setprio, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, sec_setprio_show, sec_setprio_store);

static int nowplus_sec_setprio_sysfs_init(void)
{
	/* file creation at '/sys/class/sec_setprio/sec_setprio  */
	sec_setprio_class = class_create(THIS_MODULE, "sec_setprio");
	if (IS_ERR(sec_setprio_class))
		pr_err("Failed to create class (sec_setprio)\n");

	sec_set_prio = device_create(sec_setprio_class, NULL, 0, NULL, "sec_setprio");
	if (IS_ERR(sec_set_prio))
		pr_err("Failed to create device(sec_set_prio)!\n");
	if (device_create_file(sec_set_prio, &dev_attr_sec_setprio) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_sec_setprio.attr.name);

	return 0;
}
/* } ends - andre.b.kim : added sec_setprio as module */

static void synaptics_dev_init(void)
{
        /* Set the ts_gpio pin mux */
        if (gpio_request(OMAP_GPIO_TOUCH_IRQ, "touch_synaptics") < 0) {
                printk(KERN_ERR  "can't get synaptics pen down GPIO\n");
                return;
        }
        gpio_direction_input(OMAP_GPIO_TOUCH_IRQ);
}

void __init nowplus_peripherals_init(void)
{

	spi_register_board_info(nowplus_spi_board_info,
			ARRAY_SIZE(nowplus_spi_board_info));

//	platform_device_register(&archer_dss_device);
	board_onenand_init();
	synaptics_dev_init();
	omap_serial_init();
	config_wlan_gpio();

    config_camera_gpio();
	mod_clock_correction();
	usb_musb_init(&musb_board_data);
	//usb_ehci_init();  //Proper arguments should be passed
}

static void __init nowplus_init_irq(void)
{
	omap_board_config = nowplus_config;
	omap_board_config_size = ARRAY_SIZE(nowplus_config);

	omap_init_irq();
	omap3_pm_init_vc(&omap3_setuptime_table);
	omap3_pm_init_cpuidle(omap3_cpuidle_params_table);

	omap2_init_common_hw(nowplus_sdrc_params, nowplus_sdrc_params,omap3_mpu_rate_table,
				 omap3_dsp_rate_table, omap3_l3_rate_table);
}

static void __init nowplus_init(void)
{
	char str_powerup_reason[15];
	u32 regval;

	printk("\n.....[Nowplus] Initializing...\n");

	// change reset duration (PRM_RSTTIME register)
	regval = omap_readl(0x48307254);
	regval |= 0xFF;
	omap_writew(regval, 0x48307254);

	omap3_mux_init(board_mux, OMAP_PACKAGE_CBB);

	if( omap_gpio_out_init() ){
		printk( "nowplus gpio ouput set fail!!!!\n" );
	}
	printk("Gpio Output Setting Done");

    set_wakeup_gpio();

    get_powerup_reason(str_powerup_reason);
    printk( "\n\n <Powerup Reason : %s>\n\n", str_powerup_reason);

	nowplus_i2c_init();

	platform_add_devices(nowplus_devices,
			ARRAY_SIZE(nowplus_devices));

	nowplus_vaux3_supply.dev = &nowplus_lcd_device.dev;
	nowplus_vaux4_supply.dev = &nowplus_lcd_device.dev;
	nowplus_vpll2_supply.dev = &nowplus_lcd_device.dev;

	nowplus_usb_data.batt_dev = &samsung_battery_device.dev;
	nowplus_usb_data.charger_dev = &samsung_charger_device.dev;
    nowplus_usb_data.switch_dev = &headset_switch_device.dev; //Added for Regulator

	msecure_init();

	nowplus_ramconsole_init();
	nowplus_init_power_key();
	nowplus_init_ear_key();
	nowplus_init_battery();
	nowplus_init_platform();
	nowplus_init_lcd();
	nowplus_init_PL();

	nowplus_init_fmradio();
	wl127x_vio_leakage_fix();

	//nowplus_init_mmc1();
//	nowplus_init_movi_nand();
//	nowplus_init_sound();
//	nowplus_init_earphone();

	get_board_hw_rev();

    sec_class = class_create(THIS_MODULE, "sec");
    if (IS_ERR(sec_class))
    {
            pr_err("Failed to create class(sec)!\n");
    }

	nowplus_param_sysfs_init();

	/* andre.b.kim : added sec_setprio as module */
	nowplus_sec_setprio_sysfs_init();

	nowplus_peripherals_init();
}

#if 0
static void __init bootloader_reserve_sdram(void)
{
	u32 paddr;
	u32 size = 0x80000;

	paddr = 0x8D000000;

	paddr -= size;

	if (reserve_bootmem(paddr, size, BOOTMEM_EXCLUSIVE) < 0) {
		pr_err("FB: failed to reserve VRAM\n");
	}
}
#endif

static void __init nowplus_map_io(void)
{
	omap2_set_globals_343x();
	omap2_map_common_io();
	omap2_ramconsole_reserve_sdram();
	#if 0
	bootloader_reserve_sdram();
	#endif
}

static void __init nowplus_fixup(struct machine_desc *desc,
					struct tag *tags, char **cmdline,
					struct meminfo *mi)
{
	mi->bank[0].start = 0x80000000;
	mi->bank[0].size = 256 * SZ_1M; // DDR 128MB
	mi->bank[0].node = 0;

	mi->nr_banks = 1;

}

MACHINE_START(NOWPLUS, "Samsung NOWPLUS board")
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xfa000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.fixup		= nowplus_fixup,
	.map_io		= nowplus_map_io,
	.init_irq	= nowplus_init_irq,
	.init_machine	= nowplus_init,
	.timer		= &omap_timer,
MACHINE_END
