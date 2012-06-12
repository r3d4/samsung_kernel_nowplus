/*
	*	linux/arch/arm/mach-omap2/board-nowplus.c
	*
	*	This	program	is	free	software;	you	can	redistribute	it	and/or	modify
	*	it	under	the	terms	of	the	GNU	General	Public	License	version	2	as
	*	published	by	the	Free	Software	Foundation.
	*/

#include	<linux/kernel.h>
#include	<linux/init.h>
#include	<linux/platform_device.h>
#include	<linux/delay.h>
#include	<linux/err.h>
#include	<linux/clk.h>
#include	<linux/io.h>
#include	<linux/gpio.h>
#include	<linux/gpio_keys.h>
#include	<linux/switch.h>
#include	<linux/irq.h>
#include	<linux/interrupt.h>
#include	<linux/regulator/machine.h>
#include	<linux/i2c/twl.h>
#include	<linux/usb/android_composite.h>
#include	<linux/ctype.h>
#include	<linux/bootmem.h>
#include 	<linux/mmc/host.h>
#include	<linux/spi/spi_gpio.h>
#include	<linux/spi/spi.h>
#include	<linux/spi/tl2796.h>
#include	<linux/input.h>
#include	<linux/input/matrix_keypad.h>

#include	<linux/leds-bd2802.h>
#include	<linux/max17040_battery.h>
#include	<linux/input/synaptics_i2c_rmi4.h>

#include	<mach/hardware.h>
#include	<asm/mach-types.h>
#include	<asm/mach/arch.h>
#include	<asm/mach/map.h>
#include	<asm/setup.h>

#include 	<plat/sdrc.h>
#include	<plat/mcspi.h>
#include	<plat/mux.h>
#include	<plat/timer-gp.h>
#include	<plat/board.h>
#include	<plat/usb.h>
#include	<plat/common.h>
#include	<plat/dma.h>
#include	<plat/gpmc.h>
#include    <plat/onenand.h>
#include	<plat/display.h>
#include 	<plat/opp_twl_tps.h>
#include	<plat/control.h>
#include	<plat/omap-pm.h>
#include	<plat/mux_nowplus.h>
#include	<plat/prcm.h>
#include	"cm.h"

#include	"mux.h"
#include	"hsmmc.h"
#include	"pm.h"

#ifdef	CONFIG_SERIAL_OMAP
#include	<plat/omap-serial.h>
#include	<plat/serial.h>
#endif

//#include	<mach/param.h>
#include	<mach/sec_switch.h>

#if	defined(CONFIG_USB_SWITCH_FSA9480)
#include	<linux/fsa9480.h>
#endif

#ifdef	CONFIG_PM
#include	<plat/vrfb.h>
#include	<media/videobuf-dma-sg.h>
#include	<media/v4l2-device.h>
#include	<../../../drivers/media/video/omap/omap_voutdef.h>
#endif

#define M4MO_DRIVER_NAME            "m-4mo"
#define M4MO_I2C_ADDR               0x1F
#define S5KA3DFX_DRIVER_NAME        "s5ka3dfx"
#define S5KA3DFX_I2C_ADDR           0xC4>>1
#define CAM_PMIC_DRIVER_NAME        "cam_pmic"
#define CAM_PMIC_I2C_ADDR           0x7D

#define EAR_ADC_OFF                 1   // STG5123 S1 enabled
#define EAR_ADC_ON                  0   // STG5123 S2 enabled

#define TWL4030_RESCONFIG(res,grp,typ1,typ2,state) \
{ \
    .resource = res, \
    .devgroup = grp, \
    .type = typ1, \
    .type2 = typ2, \
    .remap_sleep = state \
}
    
/*	make	easy	to	register	param	to	sysfs	*/
#define	REGISTER_PARAM(idx,name)		\
static	ssize_t	name##_show(struct	device	*dev,	struct	device_attribute	*attr,	char	*buf)\
{\
	return	get_integer_param(idx,	buf);\
}\
static	ssize_t	name##_store(struct	device	*dev,	struct	device_attribute	*attr,	const	char	*buf,	size_t	size)\
{\
	return	set_integer_param(idx,	buf,	size);\
}\
static	DEVICE_ATTR(name,	S_IRUGO	|	S_IWUGO	|	S_IRUSR	|	S_IWUSR,	name##_show,	name##_store)

static	struct	device	*param_dev;

struct	class	*sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

struct	timezone	sec_sys_tz;
EXPORT_SYMBOL(sec_sys_tz);

void	(*sec_set_param_value)(int	idx,	void	*value);
EXPORT_SYMBOL(sec_set_param_value);

void	(*sec_get_param_value)(int	idx,	void	*value);
EXPORT_SYMBOL(sec_get_param_value);

#if	defined(CONFIG_SAMSUNG_SETPRIO)
struct	class	*sec_setprio_class;
EXPORT_SYMBOL(sec_setprio_class);

static	struct	device	*sec_set_prio;

void	(*sec_setprio_set_value)(const	char	*buf);
EXPORT_SYMBOL(sec_setprio_set_value);

void	(*sec_setprio_get_value)(void);
EXPORT_SYMBOL(sec_setprio_get_value);
#endif


u32	hw_revision;
EXPORT_SYMBOL(hw_revision);

extern int set_wakeup_gpio( unsigned int gpio[], int cnt );
extern int __init nowplus_wifi_init(void);

#define	ENABLE_EMMC


#ifndef	CONFIG_TWL4030_CORE
#error	"no	power	companion	board	defined!"
#else
#define	TWL4030_USING_BROADCAST_MSG
#endif

#ifdef	CONFIG_WL127X_RFKILL
#include	<linux/wl127x-rfkill.h>
#endif

#ifdef CONFIG_WIFI_CONTROL_FUNC
extern struct platform_device nowplus_wifi_device;
#endif

int	usbsel	=	1;
EXPORT_SYMBOL(usbsel);
void	(*usbsel_notify)(int)	=	NULL;
EXPORT_SYMBOL(usbsel_notify);

extern	unsigned	get_last_off_on_transaction_id(struct	device	*dev);
extern	int	omap34xx_pad_set_config_lcd(u16	pad_pin,u16	pad_val);


static	int	sec_switch_status	=	0;
static	int	sec_switch_inited	=	0;


#if	defined(CONFIG_USB_SWITCH_FSA9480)
static	bool	fsa9480_jig_status	=	0;
static	enum	cable_type_t	set_cable_status;

extern	void	twl4030_phy_enable(void);
extern	void	twl4030_phy_disable(void);
#endif

static struct cpuidle_params omap3_cpuidle_params_table[] = {
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
};
#define PAD_OFFS_I2C3_SCL   0x01C2 
#define PAD_OFFS_I2C3_SDA   0x01C4 
#define PAD_OFFS_ETK_D11    0x05f2
#define PAD_OFFS_UART3_RX   0x019e
#define PAD_OFFS_UART3_TX   0x01a0

#define PS_HOLD_USE_PULL        //set output value via pullup/-down

void set_ps_hold(int state)
{
#ifdef PS_HOLD_USE_PULL
    printk("[%s] enable PS_HOLD %s \n",	__func__, state?"pullup":"pulldown");
    if (state)
        omap34xx_pad_set_config_lcd(PAD_OFFS_ETK_D11, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLUP | OMAP34XX_PIN_OFF_INPUT_PULLUP);
    else
        omap34xx_pad_set_config_lcd(PAD_OFFS_ETK_D11, OMAP34XX_MUX_MODE4 | OMAP34XX_PIN_INPUT_PULLDOWN| OMAP34XX_PIN_OFF_INPUT_PULLDOWN);
#else
    printk("[%s] set GPIO %d\n", __func__, state);
    gpio_set_value(OMAP_GPIO_PS_HOLD_PU, state);
#endif       
}
EXPORT_SYMBOL(set_ps_hold);


int	nowplus_enable_touch_pins(int	enable)	{
	printk("[TOUCH]	%s	touch	pins\n",	enable?"enable":"disable");
	if	(enable)	{
		omap34xx_pad_set_config_lcd(PAD_OFFS_I2C3_SCL,	OMAP34XX_MUX_MODE0	|	OMAP34XX_PIN_INPUT_PULLUP	|	OMAP34XX_PIN_OFF_OUTPUT_LOW);
		omap34xx_pad_set_config_lcd(PAD_OFFS_I2C3_SDA,	OMAP34XX_MUX_MODE0	|	OMAP34XX_PIN_INPUT_PULLUP	|	OMAP34XX_PIN_OFF_OUTPUT_LOW);
	}
	else	{
		omap34xx_pad_set_config_lcd(PAD_OFFS_I2C3_SCL,	OMAP34XX_MUX_MODE4	|	OMAP34XX_PIN_OUTPUT	|	OMAP34XX_PIN_OFF_OUTPUT_LOW);
		omap34xx_pad_set_config_lcd(PAD_OFFS_I2C3_SDA,	OMAP34XX_MUX_MODE4	|	OMAP34XX_PIN_OUTPUT	|	OMAP34XX_PIN_OFF_OUTPUT_LOW);
	}
	mdelay(50);
	return	0;
}
EXPORT_SYMBOL(nowplus_enable_touch_pins);

int	nowplus_enable_uart_pins(int	enable)	{
	printk("[UART]	%s	uart	pins\n",	enable?"enable":"disable");
	if	(enable)	{
		omap34xx_pad_set_config_lcd(PAD_OFFS_UART3_RX,	OMAP34XX_MUX_MODE0	|	OMAP34XX_PIN_INPUT_PULLUP	|	OMAP34XX_PIN_OFF_INPUT);
		omap34xx_pad_set_config_lcd(PAD_OFFS_UART3_TX,	OMAP34XX_MUX_MODE0	|	OMAP34XX_PIN_OUTPUT	|	OMAP34XX_PIN_OFF_INPUT);
	}
	else	{
		omap34xx_pad_set_config_lcd(PAD_OFFS_UART3_RX,	OMAP34XX_MUX_MODE4	|	OMAP34XX_PIN_INPUT	|	OMAP34XX_PIN_OFF_INPUT	);
		omap34xx_pad_set_config_lcd(PAD_OFFS_UART3_TX,	OMAP34XX_MUX_MODE4	|	OMAP34XX_PIN_INPUT	|	OMAP34XX_PIN_OFF_INPUT);
	}
	mdelay(50);
	return	0;
}

EXPORT_SYMBOL(nowplus_enable_uart_pins);

#ifdef	CONFIG_WL127X_RFKILL

int	nowplus_bt_hw_enable(void)	{
//	return	0;
	printk("[BT]	%s	bt	pins\n",	1?"enable":"disable");
	//cts
	omap34xx_pad_set_config_lcd(0x144	+	0x30,	OMAP34XX_MUX_MODE0	|	OMAP34XX_PIN_INPUT_PULLUP	|	OMAP34XX_PIN_OFF_INPUT_PULLUP);
	//rts
	omap34xx_pad_set_config_lcd(0x146	+	0x30,	OMAP34XX_MUX_MODE0	|	OMAP34XX_PIN_OUTPUT	|	OMAP34XX_PIN_OFF_OUTPUT_LOW);
	//tx
	omap34xx_pad_set_config_lcd(0x148	+	0x30,	OMAP34XX_MUX_MODE0	|	OMAP34XX_PIN_OUTPUT	|	OMAP34XX_PIN_OFF_INPUT_PULLUP);
	//rx
	omap34xx_pad_set_config_lcd(0x14a	+	0x30,	OMAP34XX_MUX_MODE0	|	OMAP34XX_PIN_INPUT_PULLUP	|	OMAP34XX_PIN_OFF_INPUT_PULLUP);
	mdelay(50);
	return	0;
};

int	nowplus_bt_hw_disable(void){
	//	return	0;
	printk("[BT]	%s	bt	pins",	0?"enable":"disable");
	omap34xx_pad_set_config_lcd(0x144	+	0x30,	OMAP34XX_MUX_MODE4	|	OMAP34XX_PIN_INPUT_PULLDOWN	|	OMAP34XX_PIN_OFF_INPUT_PULLDOWN);
	omap34xx_pad_set_config_lcd(0x146	+	0x30,	OMAP34XX_MUX_MODE4	|	OMAP34XX_PIN_INPUT_PULLDOWN	|	OMAP34XX_PIN_OFF_INPUT_PULLDOWN);
	omap34xx_pad_set_config_lcd(0x148	+	0x30,	OMAP34XX_MUX_MODE4	|	OMAP34XX_PIN_INPUT_PULLDOWN	|	OMAP34XX_PIN_OFF_INPUT_PULLDOWN);
	omap34xx_pad_set_config_lcd(0x14a	+	0x30,	OMAP34XX_MUX_MODE4	|	OMAP34XX_PIN_INPUT_PULLDOWN	|	OMAP34XX_PIN_OFF_INPUT_PULLDOWN);
	mdelay(50);
	return	0;
};

static	struct	wl127x_rfkill_platform_data	wl127x_plat_data	=	{
	.bt_nshutdown_gpio	=	OMAP_GPIO_BT_NSHUTDOWN	,		/*	Bluetooth	Enable	GPIO	*/
	.fm_enable_gpio	=	-1,		/*	FM	Enable	GPIO	*/
	.bt_hw_enable	=	nowplus_bt_hw_enable,
	.bt_hw_disable	=	nowplus_bt_hw_disable,
};

static	struct	platform_device	nowplus_wl127x_device	=	{
	.name			=	"wl127x-rfkill",
	.id				=	-1,
	.dev.platform_data	=	&wl127x_plat_data,
};
#endif


static struct omap_sdrc_params nowplus_sdrc_params[] = {
	[0] = {
		.rate	     	= 166000000,
		.actim_ctrla 	= 0x629db485,
		.actim_ctrlb 	= 0x00022014,
		.rfr_ctrl    	= 0x0004de01,
		.mr	     		= 0x00000032,
	},
	[1] = {
		.rate	    	 = 165941176,
		.actim_ctrla 	= 0x629db485,
		.actim_ctrlb 	= 0x00022014,
		.rfr_ctrl    	= 0x0004de01,
		.mr	     		= 0x00000032,
	},
	[2] = {
		.rate	     	= 83000000,
		.actim_ctrla 	= 0x39512243,
		.actim_ctrlb 	= 0x00022010,
		.rfr_ctrl    	= 0x00025401,
		.mr	     		= 0x00000032,
	},
	[3] = {
		.rate	     	= 82970588,
		.actim_ctrla 	= 0x39512243,
		.actim_ctrlb 	= 0x00022010,
		.rfr_ctrl    	= 0x00025401,
		.mr	     		= 0x00000032,
	},
	[4] = {
		.rate	     = 0
	},
};

static	int	nowplus_twl4030_keymap[]	=	{
	KEY(0,	0,	KEY_FRONT),
	KEY(1,	0,	KEY_SEARCH),
	KEY(2,	0,	KEY_CAMERA_FOCUS),
	KEY(0,	1,	KEY_PHONE),
	KEY(2,	1,	KEY_CAMERA),
	KEY(0,	2,	KEY_EXIT),
	KEY(1,	2,	KEY_VOLUMEUP),
	KEY(2,	2,	KEY_VOLUMEDOWN),
};

static	struct	matrix_keymap_data	nowplus_keymap_data	=	{
	.keymap			=	nowplus_twl4030_keymap,
	.keymap_size		=	ARRAY_SIZE(nowplus_twl4030_keymap),
};

static	struct	twl4030_keypad_data	nowplus_kp_data	=	{
	.keymap_data	=	&nowplus_keymap_data,
	.rows		=	3,
	.cols		=	3,
	.rep		=	1,
};

static	struct	resource	nowplus_power_key_resource	=	{
	.start	=	OMAP_GPIO_IRQ(OMAP_GPIO_KEY_PWRON),
	.end	=	OMAP_GPIO_IRQ(OMAP_GPIO_KEY_PWRON),
	.flags	=	IORESOURCE_IRQ	|	IORESOURCE_IRQ_HIGHLEVEL,
};


static	struct	platform_device	nowplus_power_key_device	=	{
	.name			=	"power_key_device",
	.id				=	-1,
	.num_resources	=	1,
	.resource		=	&nowplus_power_key_resource,
};

static	struct	resource	nowplus_ear_key_resource	=	{
				.start	=	OMAP_GPIO_IRQ(OMAP_GPIO_EAR_KEY),
				.end	=	0,
				.flags	=	IORESOURCE_IRQ	|	IORESOURCE_IRQ_HIGHLEVEL,
};

static	struct	platform_device	nowplus_ear_key_device	=	{
		.name			=	"ear_key_device",
		.id				=	-1,
		.num_resources	=	1,
		.resource		=	&nowplus_ear_key_resource,
};


static	struct	resource	samsung_charger_resources[]	=	{
	[0]	=	{
		//	USB	IRQ
		.start	=	0,
		.end	=	0,
		.flags	=	IORESOURCE_IRQ	|	IORESOURCE_IRQ_SHAREABLE,
	},
	[1]	=	{
		//	TA	IRQ
		.start	=	0,
		.end	=	0,
		.flags	=	IORESOURCE_IRQ	|	IORESOURCE_IRQ_HIGHEDGE	|	IORESOURCE_IRQ_LOWEDGE,
	},
	[2]	=	{
		//	CHG_ING_N
		.start	=	0,
		.end	=	0,
		.flags	=	IORESOURCE_IRQ	|	IORESOURCE_IRQ_HIGHEDGE	|	IORESOURCE_IRQ_LOWEDGE,
	},
	[3]	=	{
		//	CHG_EN
		.start	=	0,
		.end	=	0,
		.flags	=	IORESOURCE_IRQ,
	},
};

#ifdef	CONFIG_USB_SWITCH_FSA9480
struct	sec_battery_callbacks	{
	void	(*set_cable)(struct	sec_battery_callbacks	*ptr,
		enum	cable_type_t	status);
};

struct	sec_battery_callbacks	*callbacks;

static	void	sec_battery_register_callbacks(
		struct	sec_battery_callbacks	*ptr)
{
	callbacks	=	ptr;
	/*	if	there	was	a	cable	status	change	before	the	charger	was
	ready,	send	this	now	*/
	if	((set_cable_status	!=	0)	&&	callbacks	&&	callbacks->set_cable)
		callbacks->set_cable(callbacks,	set_cable_status);
}
#endif

struct charger_device_config 
//	THIS	CONFIG	IS	SET	IN	BOARD_FILE.(platform_data)
{
	/*	CHECK	BATTERY	VF	USING	ADC	*/
	int	VF_CHECK_USING_ADC;	//	true	or	false
	int	VF_ADC_PORT;

	/*	SUPPORT	CHG_ING	IRQ	FOR	CHECKING	FULL	*/
	int	SUPPORT_CHG_ING_IRQ;

#ifdef	CONFIG_USB_SWITCH_FSA9480
	void	(*register_callbacks)(struct	sec_battery_callbacks	*ptr);
#endif
};

static struct charger_device_config samsung_charger_config_data = {
	//	[	CHECK	VF	USING	ADC	]
	/*	1.	ENABLE	(true,	flase)	*/
	.VF_CHECK_USING_ADC	=	false,	//	n.c.	no	adc	connected	to	battery	on	nowplus

	/*	2.	ADCPORT	(ADCPORT	NUM)	*/
	.VF_ADC_PORT	=	0,


	//	[	SUPPORT	CHG_ING	IRQ	FOR	CHECKING	FULL	]
	/*	1.	ENABLE	(true,	flase)	*/
	.SUPPORT_CHG_ING_IRQ	=	true,	//use	/CHRG	as	IRQ

#ifdef	CONFIG_USB_SWITCH_FSA9480
	.register_callbacks	=	&sec_battery_register_callbacks,
#endif
};

static	int	samsung_battery_config_data[]	=	{
	//	[	SUPPORT	MONITORING	CHARGE	CURRENT	FOR	CHECKING	FULL	]
	/*	1.	ENABLE	(true,	flase)	*/
	false,	//nowplus	doesnt	supprot	charging	current	measurement
	/*	2.	ADCPORT	(ADCPORT	NUM)	*/
	0,


	//	[	SUPPORT	MONITORING	TEMPERATURE	OF	THE	SYSTEM	FOR	BLOCKING	CHARGE	]
	/*	1.	ENABLE	(true,	flase)	*/
	true,	//	nowplus	has	thermistor

	/*	2.	ADCPORT	(ADCPORT	NUM)	*/
	5,	//	NCP15WB473	thermistor	@twl5030	adcin5
};

static	struct	platform_device	samsung_charger_device	=	{
	.name			=	"secChargerDev",
	.id				=	-1,
	.num_resources	=	ARRAY_SIZE(samsung_charger_resources),
	.resource		=	samsung_charger_resources,

	.dev	=	{
		.platform_data	=	&samsung_charger_config_data,
	}
};

static	struct	platform_device	samsung_battery_device	=	{
	.name			=	"secBattMonitor",
	.id				=	-1,
	.num_resources	=	0,
	.dev	=	{
		.platform_data	=	&samsung_battery_config_data,
	}
};


static	struct	platform_device	samsung_virtual_rtc_device	=	{
	.name			=	"virtual_rtc",
	.id				=	-1,
};

static	struct	platform_device	samsung_vibrator_device	=	{
	.name			=	"secVibrator",
	.id				=	-1,
	.num_resources	=	0,
};

static	struct	platform_device	samsung_pl_sensor_power_device	=	{
	.name			=	"secPLSensorPower",
	.id				=	-1,
	.num_resources	=	0,
};

static	struct	bd2802_led_platform_data	nowplus_led_data	=	{
	.reset_gpio	=	OMAP_GPIO_RGB_RST,
	.rgb_time	=	(3<<6)|(3<<4)|(4),	/*	refer	to	application	note	*/
};

static	struct	omap2_mcspi_device_config	nowplus_mcspi_config	=	{
	.turbo_mode	=	0,
	.single_channel	=	1,	/*	0:	slave,	1:	master	*/
};

static	struct	spi_board_info	nowplus_spi_board_info[]	__initdata	=	{
	{
		.modalias		=	"tl2796_disp_spi",
		.bus_num		=	1,
		.chip_select		=	0,
		.max_speed_hz		=	375000,
		.controller_data	=	&nowplus_mcspi_config,
		.irq			=	0,
	}
};


static	struct	omap_dss_device	nowplus_lcd_device	=	{
	.name			=	"lcd",
	.driver_name		=	"tl2796_panel",
	.type			=	OMAP_DISPLAY_TYPE_DPI,
	.phy.dpi.data_lines	=	24,
	.platform_enable	=	NULL,
	.platform_disable	=	NULL,
};

static	struct	omap_dss_device	*nowplus_dss_devices[]	=	{
	&nowplus_lcd_device,
};

static	struct	omap_dss_board_info	nowplus_dss_data	=	{
	.get_last_off_on_transaction_id	=	(void	*)	get_last_off_on_transaction_id,
	.num_devices	=	ARRAY_SIZE(nowplus_dss_devices),
	.devices	=	nowplus_dss_devices,
	.default_device	=	&nowplus_lcd_device,
};

// static	struct	platform_device	nowplus_dss_device	=	{
	// .name	=	"omapdss",
	// .id		=	-1,
	// .dev	=	{
		// .platform_data	=	&nowplus_dss_data,
	// },
// };


#ifdef	CONFIG_PM
static	struct	omap_volt_vc_data	vc_config	=	{
#if 1
	/* MPU / IVA / ARM */
	.vdd0_on	= 1200000, /* 1.2v */
	.vdd0_onlp	= 975000, /* 1.0v */
	.vdd0_ret	=  975000, /* 0.975v */
	.vdd0_off	=  600000, /* 0.6v */
	/* CORE */
	.vdd1_on	= 1150000, /* 1.15v */
	.vdd1_onlp	= 975000, /* 1.0v */
	.vdd1_ret	=  975000, /* 0.975v */
	.vdd1_off	=  600000, /* 0.6v */
#else //Samsung
	/* MPU */
	.vdd0_on	= 1200000, /* 1.2v */
	.vdd0_onlp	=  975000, //1000000, /* 1.0v */
	.vdd0_ret	=  975000, /* 0.975v */
	.vdd0_off	= 1200000,//600000, /* 0.6v */
	/* CORE */
	.vdd1_on	= 1150000, /* 1.15v */
	.vdd1_onlp	=  975000, //1000000, /* 1.0v */
	.vdd1_ret	=  975000, /* 0.975v */
	.vdd1_off	= 1150000,//600000, /* 0.6v */
#endif
	.clksetup	=	0xff,
	.voltoffset	=	0xff,
	.voltsetup2	=	0xff,
	.voltsetup_time1	=	0xfff,
	.voltsetup_time2	=	0xfff,
};

#ifdef	CONFIG_TWL4030_CORE
static	struct	omap_volt_pmic_info	omap_pmic_mpu	=	{	/*	and	iva	*/
	.name	=	"twl",
	.slew_rate	=	4000,
	.step_size	=	12500,
	.i2c_addr	=	0x12,
	.i2c_vreg	=	0x0,	/*	(vdd0)	VDD1	->	VDD1_CORE	->	VDD_MPU (MPU / ARM / IVA)	*/
	.vsel_to_uv	=	omap_twl_vsel_to_uv,
	.uv_to_vsel	=	omap_twl_uv_to_vsel,
	.onforce_cmd	=	omap_twl_onforce_cmd,
	.on_cmd	=	omap_twl_on_cmd,
	.sleepforce_cmd	=	omap_twl_sleepforce_cmd,
	.sleep_cmd	=	omap_twl_sleep_cmd,
	.vp_config_erroroffset	=	0,
	.vp_vstepmin_vstepmin	=	0x01,
	.vp_vstepmax_vstepmax	=	0x04,
	.vp_vlimitto_timeout_us	=	0xFFFF,//values	taken	from	froyo
	.vp_vlimitto_vddmin	=	0x00,//values	taken	from	froyo
	.vp_vlimitto_vddmax	=	0x3C,//values	taken	from	froyo
};

static	struct	omap_volt_pmic_info	omap_pmic_core	=	{
	.name	=	"twl",
	.slew_rate	=	4000,
	.step_size	=	12500,
	.i2c_addr	=	0x12,
	.i2c_vreg	=	0x1,	/*	(vdd1)	VDD2	->	VDD2_CORE	->	VDD_CORE (L3)	*/
	.vsel_to_uv	=	omap_twl_vsel_to_uv,
	.uv_to_vsel	=	omap_twl_uv_to_vsel,
	.onforce_cmd	=	omap_twl_onforce_cmd,
	.on_cmd	=	omap_twl_on_cmd,
	.sleepforce_cmd	=	omap_twl_sleepforce_cmd,
	.sleep_cmd	=	omap_twl_sleep_cmd,
	.vp_config_erroroffset	=	0,
	.vp_vstepmin_vstepmin	=	0x01,
	.vp_vstepmax_vstepmax	=	0x04,
	.vp_vlimitto_timeout_us	=	0xFFFF,//values	taken	from	froyo
	.vp_vlimitto_vddmin	=	0x00,//values	taken	from	froyo
	.vp_vlimitto_vddmax	=	0x2C,//values	taken	from	froyo
};
#endif	/*	CONFIG_TWL4030_CORE	*/
#endif	/*	CONFIG_PM	*/


//	for	storing	/proc/last_kmsg
#ifdef	CONFIG_ANDROID_RAM_CONSOLE
#define	RAM_CONSOLE_START	0x8E000000
#define	RAM_CONSOLE_SIZE	0x20000
static	struct	resource	ram_console_resource	=	{
		.start	=	RAM_CONSOLE_START,
		.end	=	(RAM_CONSOLE_START	+	RAM_CONSOLE_SIZE	-	1),
		.flags	=	IORESOURCE_MEM,
};

static	struct	platform_device	ram_console_device	=	{
		.name	=	"ram_console",
		.id	=	0,
		.num_resources	=	1,
		.resource		=	&ram_console_resource,
};

static	inline	void	nowplus_ramconsole_init(void)
{
	platform_device_register(&ram_console_device);
}

static	inline	void	omap2_ramconsole_reserve_sdram(void)
{
	reserve_bootmem(RAM_CONSOLE_START,	RAM_CONSOLE_SIZE,	0);
}
#else
static	inline	void	nowplus_ramconsole_init(void)	{}

static	inline	void	omap2_ramconsole_reserve_sdram(void)	{}
#endif

static	struct	gpio_switch_platform_data	headset_switch_data	=	{
		.name	=	"h2w",
		.gpio	=	OMAP_GPIO_DET_3_5,	/*	Omap3430	GPIO_27	For	samsung	zeus	*/
};

static	struct	platform_device	headset_switch_device	=	{
		.name	=	"switch-gpio",
		.dev	=	{
			.platform_data	=	&headset_switch_data,
		}
};

static	struct	platform_device	sec_device_dpram	=	{
	.name	=	"dpram-device",
	.id		=	-1,
};

//>>>	switch	>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
int	sec_switch_get_cable_status(void)
{
	return	set_cable_status;	
}
// used in twl_power_off
EXPORT_SYMBOL(sec_switch_get_cable_status);

void	sec_switch_set_switch_status(int	val)
{
	printk("%s	(switch_status	:	%d)\n",	__func__,	val);

#if	defined(CONFIG_USB_SWITCH_FSA9480)
	if(!sec_switch_inited)	{
		fsa9480_enable_irq();
		/*
			*	TI	CSR	OMAPS00222879:	"MP3	playback	current	consumption	is	very	high"
			*	This	a	workaround	to	reduce	the	mp3	power	consumption	when	the	system
			*	is	booted	without	USB	cable.
			*/
		twl4030_phy_enable();

		if(set_cable_status	!=	CABLE_TYPE_USB)
			twl4030_phy_disable();
	}
#endif

	if(!sec_switch_inited)
		sec_switch_inited	=	1;

	sec_switch_status	=	val;
}

static	struct	sec_switch_platform_data	sec_switch_pdata	=	{
	.get_cable_status	=	sec_switch_get_cable_status,
	.set_switch_status	=	sec_switch_set_switch_status,
};

struct	platform_device	sec_device_switch	=	{
	.name	=	"sec_switch",
	.id	=	1,
	.dev	=	{
		.platform_data	=	&sec_switch_pdata,
	}
};
//<<<	switch	<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

#define	OMAP_GPIO_SYS_DRM_MSECURE	22
static	int	__init	msecure_init(void)
{
		int	ret	=	0;

#ifdef	CONFIG_RTC_DRV_TWL4030
		/*	3430ES2.0	doesn't	have	msecure/gpio-22	line	connected	to	T2	*/
	if	(omap_type()	==	OMAP2_DEVICE_TYPE_GP){
				void	__iomem	*msecure_pad_config_reg	=	omap_ctrl_base_get()	+	0xA3C;
				int	mux_mask	=	0x04;
				u16	tmp;
#if	1
				ret	=	gpio_request(OMAP_GPIO_SYS_DRM_MSECURE,	"msecure");
				if	(ret	<	0)	{
						printk(KERN_ERR	"msecure_init:	can't"
								"reserve	GPIO:%d	!\n",	OMAP_GPIO_SYS_DRM_MSECURE);
						goto	out;
				}
				/*
	*	*					*	TWL4030	will	be	in	secure	mode	if	msecure	line	from	OMAP
	*	*									*	is	low.	Make	msecure	line	high	in	order	to	change	the
	*	*													*	TWL4030	RTC	time	and	calender	registers.
	*		*																		*/
				tmp	=	__raw_readw(msecure_pad_config_reg);
				tmp	&=	0xF8;	/*	To	enable	mux	mode	03/04	=	GPIO_RTC	*/
				tmp	|=	mux_mask;/*	To	enable	mux	mode	03/04	=	GPIO_RTC	*/
				__raw_writew(tmp,	msecure_pad_config_reg);

				gpio_direction_output(OMAP_GPIO_SYS_DRM_MSECURE,	1);
#endif
		}
out:
#endif
		return	ret;
}

static	struct	platform_device	*nowplus_devices[]	__initdata	=	{
	//&nowplus_dss_device,	//move to omap_...
	&headset_switch_device,
#ifdef	CONFIG_WL127X_RFKILL
	&nowplus_wl127x_device,
#endif
	&nowplus_power_key_device,
	&nowplus_ear_key_device,
	&samsung_battery_device,
	&samsung_charger_device,
	&samsung_vibrator_device,	//	cdy_100111	add	vibrator	device
	&sec_device_dpram,
	&samsung_pl_sensor_power_device,
	&sec_device_switch,
#ifdef	CONFIG_RTC_DRV_VIRTUAL
	&samsung_virtual_rtc_device,
#endif
#ifdef CONFIG_WIFI_CONTROL_FUNC
    &nowplus_wifi_device,
#endif
};

extern unsigned int wakeup_gpio[];
#ifdef	CONFIG_OMAP_MUX
extern	struct	omap_board_mux	board_mux[];
#else
#define	board_mux		NULL
#endif

static	inline	void	__init	nowplus_init_power_key(void)
{
	nowplus_power_key_resource.start	=	OMAP_GPIO_IRQ(OMAP_GPIO_KEY_PWRON);

	if	(gpio_request(OMAP_GPIO_KEY_PWRON,	"power_key_irq")	<	0	)	{
		printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	for	POWER	KEY	IRQ	\n",OMAP_GPIO_KEY_PWRON);
		return;
	}
	gpio_direction_input(OMAP_GPIO_KEY_PWRON);

}

static	inline	void	__init	nowplus_init_battery(void)
{
	if	(gpio_request(OMAP_GPIO_USBSW_NINT,	"OMAP_GPIO_USBSW_NINT")	<	0)	{
		printk(KERN_ERR	"	request	OMAP_GPIO_USBSW_NINT	failed\n");
		return;
	}
	gpio_direction_input(OMAP_GPIO_USBSW_NINT);

	if(gpio_request(OMAP_GPIO_CHG_ING_N,	"OMAP_GPIO_CHG_ING_N")	<	0	){
			printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_CHG_ING_N);
			return;
		}
	gpio_direction_input(OMAP_GPIO_CHG_ING_N);

	if(gpio_request(OMAP_GPIO_CHG_EN,	"OMAP_GPIO_CHG_EN")	<	0	){
			printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_CHG_EN);
			return;
		}

	/*CHG_EN	low	or	high	depend	on	bootloader	configuration*/
	if(gpio_get_value(OMAP_GPIO_CHG_EN))
	{
		gpio_direction_output(OMAP_GPIO_CHG_EN,	1);
	}
	else
	{
		gpio_direction_output(OMAP_GPIO_CHG_EN,	0);
	}
    
	//	Line	903:		KUSB_CONN_IRQ	=	platform_get_irq(	pdev,	0	);
	//	Line	923:		KTA_NCONN_IRQ	=	platform_get_irq(	pdev,	1	);
	//	Line	938:		KCHG_ING_IRQ	=	platform_get_irq(	pdev,	2	);
	//	Line	957:		KCHG_EN_GPIO	=	irq_to_gpio(	platform_get_irq(	pdev,	3	)	);

	//	samsung_charger_resources[0].start	=	IH_USBIC_BASE;			//	is	usb	cable	connected	?
	//	samsung_charger_resources[1].start	=	IH_USBIC_BASE	+	1;		//	is	charger	connected	?

    
#if 0
	if (gpio_request(OMAP_GPIO_USBSW_NINT, "usb switch irq") < 0) {
		printk(KERN_ERR "Failed to request GPIO%d for usb switch IRQ\n",
		       OMAP_GPIO_USBSW_NINT);
		samsung_charger_resources[0].start = -1;
	}
	else {
		samsung_charger_resources[0].start = gpio_to_irq(OMAP_GPIO_USBSW_NINT);
		//omap_set_gpio_debounce( OMAP_GPIO_TA_NCONNECTED, 3 );
		gpio_set_debounce( OMAP_GPIO_USBSW_NINT, 3 );
	}

	samsung_charger_resources[1].start	=	-1;	//	NC,	set	dummy	value	//	is	charger	connected	?

    
    if (gpio_request(OMAP_GPIO_CHG_ING_N, "charge full irq") < 0) {
        printk(KERN_ERR "Failed to request GPIO%d for charge full IRQ\n",
               OMAP_GPIO_CHG_ING_N);
        samsung_charger_resources[2].start = -1;
	}
	else {
		samsung_charger_resources[2].start = gpio_to_irq(OMAP_GPIO_CHG_ING_N);
		//omap_set_gpio_debounce( OMAP_GPIO_CHG_ING_N, 3 );
		gpio_set_debounce( OMAP_GPIO_CHG_ING_N, 3 );
	}

	if (gpio_request(OMAP_GPIO_CHG_EN, "Charge enable gpio") < 0) {
		printk(KERN_ERR "Failed to request GPIO%d for charge enable gpio\n",
		       OMAP_GPIO_CHG_EN);
		samsung_charger_resources[3].start = -1;
 	}
	else {
		samsung_charger_resources[3].start = gpio_to_irq(OMAP_GPIO_CHG_EN);
	}
#endif
}


#if 0 // nowplus doesnt use TV-OUT DAC
static	struct	regulator_consumer_supply	nowplus_vdda_dac_supply	=	{
	.supply		=	"vdda_dac",
	.dev		=	&nowplus_dss_device.dev,
};
#endif
static	struct	regulator_consumer_supply	nowplus_vmmc1_supply	=	{
	.supply			=	"vmmc",
};

static	struct	regulator_consumer_supply	nowplus_vmmc2_supply	=	{
	.supply			=	"vmmc",
};

static	struct	regulator_consumer_supply	nowplus_vaux1_supply	=	{
	.supply			=	"vaux1",
};

static	struct	regulator_consumer_supply	nowplus_vaux2_supply	=	{
	.supply			=	"vaux2",
};

static	struct	regulator_consumer_supply	nowplus_vaux3_supply	=	{
	.supply			=	"vaux3",
};

static	struct	regulator_consumer_supply	nowplus_vaux4_supply	=	{
	.supply			=	"vaux4",
};

static	struct	regulator_consumer_supply	nowplus_vpll2_supply	=	{
	.supply			=	"vpll2",
};

static	struct	regulator_consumer_supply	nowplus_vsim_supply	=	{
	.supply			=	"vmmc_aux",
};
#if 0 // nowplus doesnt use TV-OUT DAC
/* VDAC for DSS driving S-Video (8 mA unloaded, max 65 mA) */
static	struct	regulator_init_data	nowplus_vdac	=	{
	.constraints	=	{
		.min_uV					=	1800000,
		.max_uV					=	1800000,
        .boot_on		        =	true,       //enabled by u-boot
		.valid_modes_mask		=	REGULATOR_MODE_NORMAL
					|	REGULATOR_MODE_STANDBY,
		.valid_ops_mask			=	REGULATOR_CHANGE_MODE
					|	REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	1,
	.consumer_supplies		=	&nowplus_vdda_dac_supply,
};
#endif
/*	VMMC1	for	MMC1	card	*/
static	struct	regulator_init_data	nowplus_vmmc1	=	{
	.constraints	=	{
		.min_uV			=	1850000,
		.max_uV			=	3150000,
		.apply_uV		=	true,
		.valid_modes_mask	=	REGULATOR_MODE_NORMAL
					|	REGULATOR_MODE_STANDBY,
		.valid_ops_mask		=	REGULATOR_CHANGE_MODE
					|	REGULATOR_CHANGE_STATUS
					|	REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	=	1,
	.consumer_supplies	=	&nowplus_vmmc1_supply,
};

/*	VMMC2	for	MoviNAND	*/
static	struct	regulator_init_data	nowplus_vmmc2	=	{
	.constraints	=	{
		.min_uV			=	1800000,	//	!!!	->	also	have	to	change	TLW	VIO	voltage	to	1.85V
		.max_uV			=	1800000,
		.apply_uV		=	true,
		.valid_modes_mask	=	REGULATOR_MODE_NORMAL
					|	REGULATOR_MODE_STANDBY,
		.valid_ops_mask		=	REGULATOR_CHANGE_MODE
					|	REGULATOR_CHANGE_STATUS	,
	},
	.num_consumer_supplies	=	1,
	.consumer_supplies	=	&nowplus_vmmc2_supply,
};


/*	VAUX1	for	PL_SENSOR	*/
static	struct	regulator_init_data	nowplus_vaux1	=	{
	.constraints	=	{
		.min_uV			=	3000000,
		.max_uV			=	3000000,
		.valid_modes_mask	=	REGULATOR_MODE_NORMAL
					|	REGULATOR_MODE_STANDBY,
		.valid_ops_mask		=	REGULATOR_CHANGE_MODE
					|	REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	1,
	.consumer_supplies		=	&nowplus_vaux1_supply,
};

/*	VAUX2	for	touch	screen	*/
static	struct	regulator_init_data	nowplus_vaux2	=	{
	.constraints	=	{
		.min_uV			=	2800000,
		.max_uV			=	2800000,
		.apply_uV		=	true,
		.always_on		=	true,
		.valid_modes_mask	=	REGULATOR_MODE_NORMAL
					|	REGULATOR_MODE_STANDBY,
		.valid_ops_mask		=	REGULATOR_CHANGE_MODE
					|	REGULATOR_CHANGE_STATUS
					|	REGULATOR_CHANGE_VOLTAGE,
	},
	.num_consumer_supplies	=	1,
	.consumer_supplies	=	&nowplus_vaux2_supply,
};

/*	VAUX3	for	display	*/
// TL2796 IOVCC - IO Supply Voltage
static	struct	regulator_init_data	nowplus_vaux3	=	{
	.constraints	=	{
		.min_uV			=	1800000,
		.max_uV			=	1800000,
		.boot_on		=	true,
		.valid_modes_mask	=	REGULATOR_MODE_NORMAL
					|	REGULATOR_MODE_STANDBY,
		.valid_ops_mask		=	REGULATOR_CHANGE_MODE
					|	REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	1,
	.consumer_supplies	=	&nowplus_vaux3_supply,
};

/*	VAUX4	for	display	*/
// TL2796 VCI - Internal power for RAM (Analog)
static	struct	regulator_init_data	nowplus_vaux4	=	{
	.constraints	=	{
		.min_uV			=	2800000,
		.max_uV			=	2800000,
		.boot_on		=	true,
		.valid_modes_mask	=	REGULATOR_MODE_NORMAL
					|	REGULATOR_MODE_STANDBY,
		.valid_ops_mask		=	REGULATOR_CHANGE_MODE
					|	REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	1,
	.consumer_supplies	=	&nowplus_vaux4_supply,
};

/*	VPLL2	for	digital	video	outputs	*/
static	struct	regulator_init_data	nowplus_vpll2	=	{
	.constraints	=	{
		.min_uV			=	1800000,
		.max_uV			=	1800000,
		.boot_on		=	true,
		.valid_modes_mask	=	REGULATOR_MODE_NORMAL
					|	REGULATOR_MODE_STANDBY,
		.valid_ops_mask		=	REGULATOR_CHANGE_MODE
					|	REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	1,
	.consumer_supplies	=	&nowplus_vpll2_supply,
};

/*	VSIM	for	WiFi	SDIO	*/
static	struct	regulator_init_data	nowplus_vsim	=	{
	.constraints	=	{
		.min_uV			=	1800000,
		.max_uV			=	1800000,
		.apply_uV				=	true,
		.boot_on		=	true,
        .always_on		=	true,   // always enable until mmc sdio driver is fixed
		.valid_modes_mask	=	REGULATOR_MODE_NORMAL
					|	REGULATOR_MODE_STANDBY,
		.valid_ops_mask		=	REGULATOR_CHANGE_VOLTAGE
					|	REGULATOR_CHANGE_MODE
					|	REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	=	1,
	.consumer_supplies	=	&nowplus_vsim_supply,
};

//ZEUS_LCD
static	struct	omap_lcd_config	nowplus_lcd_config	__initdata	=	{
		.ctrl_name		=	"internal",
};
//ZEUS_LCD
static	struct	omap_uart_config	nowplus_uart_config	__initdata	=	{
#ifdef	CONFIG_SERIAL_OMAP_CONSOLE
	.enabled_uarts	=	(	(1	<<	0)	|	(1	<<	1)	|	(1	<<	2)	),
#else
	.enabled_uarts	=	(	(1	<<	0)	|	(1	<<	1)	),
#endif
};

static	struct	omap_board_config_kernel	nowplus_config[]	__initdata	=	{
		{	OMAP_TAG_UART,	&nowplus_uart_config	},
		{	OMAP_TAG_LCD,	&nowplus_lcd_config	},	//ZEUS_LCD
};

static	int	nowplus_batt_table[]	=	{
/*	0	C*/
30800,	29500,	28300,	27100,
26000,	24900,	23900,	22900,	22000,	21100,	20300,	19400,	18700,	17900,
17200,	16500,	15900,	15300,	14700,	14100,	13600,	13100,	12600,	12100,
11600,	11200,	10800,	10400,	10000,	9630,	9280,	8950,	8620,	8310,
8020,	7730,	7460,	7200,	6950,	6710,	6470,	6250,	6040,	5830,
5640,	5450,	5260,	5090,	4920,	4760,	4600,	4450,	4310,	4170,
4040,	3910,	3790,	3670,	3550
};

static	struct	twl4030_bci_platform_data	nowplus_bci_data	=	{
	.battery_tmp_tbl	=	nowplus_batt_table,
	.tblsize		=	ARRAY_SIZE(nowplus_batt_table),
};

static	struct	omap2_hsmmc_info	nowplus_mmc[]	=	{
	{
		.name			=	"external",
		.mmc			=	1,
		.caps			=	MMC_CAP_4_BIT_DATA,
		.gpio_wp		=	-EINVAL,
		.power_saving	=	true,
	},
#ifdef	ENABLE_EMMC
	{
		.name			=	"internal",
		.mmc			=	2,
		.caps			=	MMC_CAP_4_BIT_DATA	|	MMC_CAP_8_BIT_DATA,
		.gpio_cd		=	-EINVAL,
		.gpio_wp		=	-EINVAL,
		.nonremovable	=	true,
		.power_saving	=	true,
	},
#endif
	{
		.mmc			=	3,
		.caps			=	MMC_CAP_4_BIT_DATA,
		.gpio_cd		=	-EINVAL,
		.gpio_wp		=	-EINVAL,
	},
	{}	/*	Terminator	*/
};


static	int	__init	nowplus_twl_gpio_setup(struct	device	*dev,
		unsigned	gpio,	unsigned	ngpio)
{
	nowplus_mmc[0].gpio_cd	=	OMAP_GPIO_TF_DETECT;
	omap2_hsmmc_init(nowplus_mmc);

	/*	link	regulators	to	MMC	adapters	*/
	nowplus_vmmc1_supply.dev	=	nowplus_mmc[0].dev;
#ifdef	ENABLE_EMMC
	nowplus_vmmc2_supply.dev	=	nowplus_mmc[1].dev;
#endif
	nowplus_vsim_supply.dev	=	nowplus_mmc[2].dev;

	return	0;
}

static	struct	twl4030_gpio_platform_data	__initdata	nowplus_gpio_data	=	{
	.gpio_base	=	OMAP_MAX_GPIO_LINES,
	.irq_base	=	TWL4030_GPIO_IRQ_BASE,
	.irq_end	=	TWL4030_GPIO_IRQ_END,
	.setup		=	nowplus_twl_gpio_setup,
};

static	struct	twl4030_usb_data	nowplus_usb_data	=	{
	.usb_mode	=	T2_USB_MODE_ULPI,
};

static	struct	twl4030_madc_platform_data	nowplus_madc_data	=	{
	.irq_line	=	1,
};

/*
 TWL5030 power management sequences
    DEV_GRP_P1= sleep when OMAP SYS_OFF_MODE=L  (only used in real OFF_MODE, not in RETENTION!)
    DEV_GRP_P3= sleep when OMAP SYS_CLKREQ=L
    
#define MSG_BROADCAST(devgrp, grp, type, type2, state) \
        ( (devgrp) << 13 | 1 << 12 | (grp) << 9 | (type2) << 7 \
        | (type) << 4 | (state))

#define MSG_SINGULAR(devgrp, id, state) \
        ((devgrp) << 13 | 0 << 12 | (id) << 4 | (state))
*/
/* 'Active to Sleep' sequence */
static	struct	twl4030_ins	__initdata	sleep_on_seq[]	=	{
	{MSG_BROADCAST(DEV_GRP_NULL,	RES_GRP_RC,	    RES_TYPE_ALL,0x0,RES_STATE_SLEEP),	4}, //set RC group to sleep
	{MSG_BROADCAST(DEV_GRP_NULL,	RES_GRP_ALL,	RES_TYPE_ALL,0x0,RES_STATE_SLEEP),	2}, // set all resource to sleep
};

/* 'Sleep to Active (P12)' sequence */
static	struct	twl4030_ins	wakeup_p12_seq[]	__initdata	=	{
	//{MSG_BROADCAST(DEV_GRP_NULL,	RES_GRP_RES,	RES_TYPE_ALL,0x2,RES_STATE_ACTIVE),	2},
	{MSG_SINGULAR(DEV_GRP_P1,	RES_CLKEN,	RES_STATE_ACTIVE),	2},                        //set CLKEN to active
	{MSG_BROADCAST(DEV_GRP_NULL,	RES_GRP_ALL,	RES_TYPE_ALL,0x0,RES_STATE_ACTIVE),	2},   //set all resourcec to active  

};

/* 'Sleep to Active' (P3) sequence */
static	struct	twl4030_ins	wakeup_p3_seq[]	__initdata	=	{
	{MSG_BROADCAST(DEV_GRP_NULL,	RES_GRP_RC,	RES_TYPE_ALL,0x0,RES_STATE_ACTIVE),	0x37},  // set RC group to active (nRESPWRON, CLKEN, SYSEN, HFCLKOUT, 32KCLKOUT, TRITON_RESET)
};

/* warm reset sequence  */
static	struct	twl4030_ins	wrst_seq[]	__initdata	=	{
	{MSG_SINGULAR(DEV_GRP_NULL,	RES_RESET,	    RES_STATE_OFF),	    2},         //	Reset	twl4030.
	{MSG_SINGULAR(DEV_GRP_P1,	RES_VDD1,	    RES_STATE_WRST),	0xE},		//	Reset	VDD1	regulator.
	{MSG_SINGULAR(DEV_GRP_P1,	RES_VDD2,	    RES_STATE_WRST),	0xE},		//	Reset	VDD2	regulator.
	{MSG_SINGULAR(DEV_GRP_P1,	RES_VPLL1,	    RES_STATE_WRST),	0x60},		//	Reset	VPLL1	regulator.
	{MSG_SINGULAR(DEV_GRP_P1,	RES_HFCLKOUT,	RES_STATE_ACTIVE),	2},         //	Enable	sysclk	output.
	{MSG_SINGULAR(DEV_GRP_NULL,	RES_RESET,	    RES_STATE_ACTIVE),	2},         //	Reenable	twl4030.
};

static struct twl4030_resconfig twl4030_rconfig[] = {
//                    .resource     .devgroup  .type .type2 .remap_sleep
    TWL4030_RESCONFIG(RES_HFCLKOUT, DEV_GRP_P3,     -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VDD1,     DEV_GRP_P1,     -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VDD2,     DEV_GRP_P1,     -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_CLKEN,    DEV_GRP_P3,     -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VIO,      DEV_GRP_ALL,    -1, -1, RES_STATE_ACTIVE ),
    TWL4030_RESCONFIG(RES_REGEN,    DEV_GRP_NULL,   -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_SYSEN,    DEV_GRP_ALL,    -1, -1, RES_STATE_ACTIVE),
//	[	-	In	order	to	prevent	damage	to	the	PMIC(TWL4030	or	5030),
//		VINTANA1	should	maintain	active	state	even	though	the	system	is	in	offmode.
    TWL4030_RESCONFIG(RES_VINTANA1, DEV_GRP_P1,     -1, -1, RES_STATE_ACTIVE),
    TWL4030_RESCONFIG(RES_VINTDIG,  DEV_GRP_P1,     -1, -1, RES_STATE_SLEEP ),
    TWL4030_RESCONFIG(RES_VAUX1,    -1,             -1, -1, RES_STATE_OFF   ),
// touch has pullups to VIO, only disable VAUX2 if VIO is off
    TWL4030_RESCONFIG(RES_VAUX2,    DEV_GRP_P1,     -1, -1, RES_STATE_SLEEP ),
    TWL4030_RESCONFIG(RES_VAUX3,    -1,             -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VAUX4,    -1,             -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VMMC1,    -1,             -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VMMC2,    -1,             -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VPLL1,    DEV_GRP_P1,     -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VPLL2,    -1,             -1, -1, RES_STATE_OFF   ),
//enable WIFI_vio, regulator framwork not used yet
    TWL4030_RESCONFIG(RES_VSIM,     DEV_GRP_P1,     -1, -1, RES_STATE_SLEEP ),
    TWL4030_RESCONFIG(RES_VDAC,     -1,             -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VINTANA2, DEV_GRP_ALL,    -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VUSB_1V5, -1,             -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VUSB_1V8, -1,             -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VUSB_3V1, DEV_GRP_ALL,    -1, -1, RES_STATE_OFF   ),
    TWL4030_RESCONFIG(RES_VUSB_3V1, -1,             -1, -1, RES_STATE_OFF   ),
    
    TWL4030_RESCONFIG(0, 0, 0, 0, 0),
};

static	struct	twl4030_script	sleep_on_script	__initdata	=	{
	.script	=	sleep_on_seq,
	.size	=	ARRAY_SIZE(sleep_on_seq),
	.flags	=	TWL4030_SLEEP_SCRIPT,
};
static	struct	twl4030_script	wakeup_p12_script	__initdata	=	{
	.script	=	wakeup_p12_seq,
	.size	=	ARRAY_SIZE(wakeup_p12_seq),
	.flags	=	TWL4030_WAKEUP12_SCRIPT,
};
static	struct	twl4030_script	wakeup_p3_script	__initdata	=	{
	.script	=	wakeup_p3_seq,
	.size	=	ARRAY_SIZE(wakeup_p3_seq),
	.flags	=	TWL4030_WAKEUP3_SCRIPT,
};
static	struct	twl4030_script	wrst_script	__initdata	=	{
	.script	=	wrst_seq,
	.size	=	ARRAY_SIZE(wrst_seq),
	.flags	=	TWL4030_WRST_SCRIPT,
};
static	struct	twl4030_script	*twl4030_scripts[]	__initdata	=	{
	&sleep_on_script,
	&wakeup_p12_script,
	&wakeup_p3_script,
	&wrst_script,
};
static	struct	twl4030_power_data	nowplus_t2scripts_data	__initdata	=	{
	.scripts	=	twl4030_scripts,
	.num		=	ARRAY_SIZE(twl4030_scripts),
	.resource_config	=	twl4030_rconfig,
};

static	struct	twl4030_codec_audio_data	nowplus_audio_data	=	{
	.audio_mclk	=	26000000,
};

static	struct	twl4030_codec_data	nowplus_codec_data	=	{
	.audio_mclk	=	26000000,
	.audio	=	&nowplus_audio_data,
};

static	struct	twl4030_platform_data	__initdata	nowplus_twl_data	__initdata	=	{
	.irq_base	=	TWL4030_IRQ_BASE,
	.irq_end	=	TWL4030_IRQ_END,

	.bci		=	&nowplus_bci_data,
	.madc		=	&nowplus_madc_data,
	.usb		=	&nowplus_usb_data,
	.gpio		=	&nowplus_gpio_data,
	.keypad		=	&nowplus_kp_data,
	.codec		=	&nowplus_codec_data,
	.power		=	&nowplus_t2scripts_data,

	.vmmc1		=	&nowplus_vmmc1,
#ifdef	ENABLE_EMMC
	.vmmc2		=	&nowplus_vmmc2,
#endif
	.vsim		=	&nowplus_vsim,
//	.vdac		=	&nowplus_vdac,
	.vaux1		=	&nowplus_vaux1,
	.vaux2		=	&nowplus_vaux2,
	.vaux3		=	&nowplus_vaux3,
	.vaux4		=	&nowplus_vaux4,
	.vpll2		=	&nowplus_vpll2,
};

#ifdef	CONFIG_BATTERY_MAX17040	//use	MAX17040	driver	for	charging

static	int	nowplus_battery_online(void)	{
	return !gpio_get_value(OMAP_GPIO_VF);
	//return 1;
};

static	int	nowplus_charger_online(void)	{
	// CABLE_TYPE_NONE = 0,
	// CABLE_TYPE_USB,
	// CABLE_TYPE_AC,
	// CABLE_TYPE_IMPROPER_AC,
	return sec_switch_get_cable_status();
};

static	int	nowplus_charger_enable(void)	{

	if(	sec_switch_get_cable_status()	)
	{
		gpio_set_value(OMAP_GPIO_CHG_EN,	0);
		set_irq_type	(OMAP_GPIO_IRQ(OMAP_GPIO_CHG_ING_N),	IRQ_TYPE_EDGE_RISING);
		return	1;
	}

	return	0;
};

static	void	nowplus_charger_disable(void)
{
	set_irq_type(OMAP_GPIO_IRQ(OMAP_GPIO_CHG_ING_N),	IRQ_TYPE_NONE);
	gpio_set_value(OMAP_GPIO_CHG_EN,	1);
}

static	int	nowplus_charger_done(void)
{
	return	0;
}

static	struct	max17040_platform_data	nowplus_max17040_data	=	{
	.battery_online	=	&nowplus_battery_online,
	.charger_online	=	&nowplus_charger_online,
	.charger_enable	=	&nowplus_charger_enable,
	.charger_done	=	&nowplus_charger_done,
	.charger_disable	=	&nowplus_charger_disable,
};
#endif


#ifdef	CONFIG_USB_SWITCH_FSA9480
static	void	fsa9480_usb_cb(bool	attached)
{
	if(sec_switch_inited) {
		if	(attached)
				twl4030_phy_enable();
			else
				twl4030_phy_disable();
	}
	
	set_cable_status	=	attached	?	CABLE_TYPE_USB	:	CABLE_TYPE_NONE;

	if	(callbacks	&&	callbacks->set_cable)
		callbacks->set_cable(callbacks,	set_cable_status);
}

static	void	fsa9480_charger_cb(bool	attached)
{
	set_cable_status	=	attached	?	CABLE_TYPE_AC	:	CABLE_TYPE_NONE;

	if	(callbacks	&&	callbacks->set_cable)
		callbacks->set_cable(callbacks,	set_cable_status);
}

static	void	fsa9480_jig_cb(bool	attached)
{
	printk("%s	:	attached	(%d)\n",	__func__,	(int)attached);

	fsa9480_jig_status	=	attached;
}

static	void	fsa9480_deskdock_cb(bool	attached)
{
	//	TODO
}

static	void	fsa9480_cardock_cb(bool	attached)
{
	//	TODO
}

static	void	fsa9480_reset_cb(void)
{
	//	TODO
}

static	struct	fsa9480_platform_data	fsa9480_pdata	=	{
	.usb_cb	=	fsa9480_usb_cb,
	.charger_cb	=	fsa9480_charger_cb,
	.jig_cb	=	fsa9480_jig_cb,
	.deskdock_cb	=	fsa9480_deskdock_cb,
	.cardock_cb	=	fsa9480_cardock_cb,
	.reset_cb	=	fsa9480_reset_cb,
};
#endif



static	struct	i2c_board_info	__initdata	nowplus_i2c1_boardinfo[]	=	{
	{
		I2C_BOARD_INFO("twl5030",	0x48),
		.flags	=	I2C_CLIENT_WAKE,
		.irq	=	INT_34XX_SYS_NIRQ,
		.platform_data	=	&nowplus_twl_data,
	},
};

static	struct	i2c_board_info	__initdata	nowplus_i2c2_boardinfo[]	=	{
	{
		I2C_BOARD_INFO(S5KA3DFX_DRIVER_NAME,	S5KA3DFX_I2C_ADDR),
//		.platform_data	=	&nowplus1_s5ka3dfx_platform_data,
	},
	{
		I2C_BOARD_INFO(M4MO_DRIVER_NAME,	M4MO_I2C_ADDR),
//		.platform_data	=	&nowplus_m4mo_platform_data,
	},
	{
		I2C_BOARD_INFO(CAM_PMIC_DRIVER_NAME,	CAM_PMIC_I2C_ADDR),
	},
	{
		I2C_BOARD_INFO("kxsd9",	0x18),
	},
	{
		I2C_BOARD_INFO("Si4709_driver",	0x10),
	},
	{
		I2C_BOARD_INFO("BD2802",	0x1A),
		.platform_data	=	&nowplus_led_data,
	},
	{
		I2C_BOARD_INFO("fsa9480",	0x4A	>>	1),
		.platform_data	=	&fsa9480_pdata,
		.irq	=	OMAP_GPIO_IRQ(OMAP_GPIO_USBSW_NINT),
	},
	{
#ifdef	CONFIG_BATTERY_MAX17040
			I2C_BOARD_INFO("max17040",	0x36),
		.platform_data	=	&nowplus_max17040_data,
#else   //	use	Samsung	driver
		I2C_BOARD_INFO("secFuelgaugeDev",	0x36),
#if	1	//(CONFIG_ARCHER_REV	>=	ARCHER_REV11)
		//.flags	=	I2C_CLIENT_WAKE,
		//.irq	=	OMAP_GPIO_IRQ(OMAP_GPIO_FUEL_INT_N),
#endif
#endif
	},
	{
		I2C_BOARD_INFO("PL_driver",	0x44),
	},
	{
		I2C_BOARD_INFO("max9877",	0x4d),
	},
};

static	struct	synaptics_rmi4_platform_data	synaptics_platform_data[]	=	{
	{
		.name	=	"synaptics",
		.irq_number	=	OMAP_GPIO_IRQ(OMAP_GPIO_TOUCH_IRQ),
		.irq_type	=	IRQF_TRIGGER_FALLING,
		.x_flip	=	false,
		.y_flip	=	false	,
		.regulator_en	=	true,
	}
};

static	struct	i2c_board_info	__initdata	nowplus_i2c3_boardinfo[]	=	{
	{
		I2C_BOARD_INFO("synaptics_rmi4_i2c",	0x2C),
		.irq	=	0,
		.platform_data	=	&synaptics_platform_data,
	},
};

static	int	__init	nowplus_i2c_init(void)
{
	/*	CSR	ID:-	OMAPS00222372	Changed	the	order	of	I2C	Bus	Registration
		*	Previously	I2C1	channel	1	was	being	registered	followed	by	I2C2	but	since
		*	TWL4030-USB	module	had	a	dependency	on	FSA9480	USB	Switch	device	which	is
		*	connected	to	I2C2	channel,	changed	the	order	such	that	I2C	channel	2	will	get
		*	registered	first	and	then	followed	by	I2C1	channel.	*/
	omap_register_i2c_bus(2, 400, NULL, nowplus_i2c2_boardinfo,	ARRAY_SIZE(nowplus_i2c2_boardinfo));
	omap_register_i2c_bus(1, 400, NULL, nowplus_i2c1_boardinfo,	ARRAY_SIZE(nowplus_i2c1_boardinfo));
	omap_register_i2c_bus(3, 100, NULL, nowplus_i2c3_boardinfo,	ARRAY_SIZE(nowplus_i2c3_boardinfo));
	return	0;
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
*/
//  max size = 0x1f280000-0x012c0000 = 0x1dfc0000

/*
start at limo initrd partition
leave blocks bevore untouched so we use odin to flash
kernel to BML partiton

new layout:
   ____________
  | 0x00000000 |    BML AREA, flash with odin
  |    ...     |
  | ---------- | -----------------------------
  | 0x012c0000 |    MTD AREA, flash from linux
  |    ...     |
  | ---------- | -----------------------------
  | 0x1f280000 |    reserved area
  |    ...     |
  | 0x1f480000 |
  |____________|

*/
#define MTD_SPLASH_SIZE     3*(64*2048)
#define MTD_START_OFFSET    (0x012c0000+MTD_SPLASH_SIZE)   
#define MTD_MAX_SIZE        (0x1dfc0000-MTD_SPLASH_SIZE)  

#ifdef CONFIG_MTD_PARTITIONS
//for safety
int check_mtd(struct mtd_partition *partitions , int cnt)
{
    int i;
    int size=0;

    //check start
    if (partitions[0].offset != MTD_START_OFFSET)
    {
        printk("check_mtd failed: wrong offset of MTD start: 0x%08x!\n", (unsigned int)partitions[0].offset);
        return 0;
    }

    for (i=0; i<cnt; i++)
        size+=partitions[i].size;
    if(size > MTD_MAX_SIZE)
    {
        printk("check_mtd failed: wrong size of MTD area: 0x%08x!\n", size);
        return 0;
    }

    return 1;
}

static struct mtd_partition onenand_partitions[] = {
    {
		.name           = "misc",
        .offset         = MTD_START_OFFSET,
		.size           = 2*(64*2048),
	},
    {
		.name           = "recovery",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 40*(64*2048),
	},
	{
		.name           = "boot",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 40*(64*2048),
	},
	{
		.name           = "system",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 1312*(64*2048),
	},
    {
		.name           = "cache",
		.offset         = MTDPART_OFS_APPEND,
		.size           = 40*(64*2048),
	},
    {
		.name           = "userdata",
		.offset         = MTDPART_OFS_APPEND,
        .size           = 2401*(64*2048),
    },

};
#endif
static struct omap_onenand_platform_data board_onenand_data = {
	.cs		= 0,
	//.gpio_irq	= OMAP_GPIO_AP_NAND_INT,
    .dma_channel = -1,  /* disable DMA in OMAP OneNAND driver */
#ifdef CONFIG_MTD_PARTITIONS
	.parts		= onenand_partitions,
	.nr_parts	= ARRAY_SIZE(onenand_partitions),
#endif
	.flags		= ONENAND_SYNC_READWRITE,
};

static void __init board_onenand_init(void)
{
#ifdef CONFIG_MTD_PARTITIONS
// only register if size and offset is correct
    if(check_mtd(onenand_partitions, ARRAY_SIZE(onenand_partitions)))
        gpmc_onenand_init(&board_onenand_data);
#else
    gpmc_onenand_init(&board_onenand_data);
#endif
}

#else

static inline void board_onenand_init(void)
{
}

#endif

static	struct	omap_uart_port_info	omap_serial_platform_data[]	=	{
	{
#if	defined(CONFIG_SERIAL_OMAP_UART1_DMA)
		.use_dma	=	CONFIG_SERIAL_OMAP_UART1_DMA,
		.dma_rx_buf_size	=	CONFIG_SERIAL_OMAP_UART1_RXDMA_BUFSIZE,
		.dma_rx_timeout	=	CONFIG_SERIAL_OMAP_UART1_RXDMA_TIMEOUT,
#else
		.use_dma	=	0,
		.dma_rx_buf_size	=	0,
		.dma_rx_timeout	=	0,
#endif	/*	CONFIG_SERIAL_OMAP_UART1_DMA	*/
		.idle_timeout	=	CONFIG_SERIAL_OMAP_IDLE_TIMEOUT,
		.flags		=	1,
	},
	{
#if	defined(CONFIG_SERIAL_OMAP_UART2_DMA)
		.use_dma	=	CONFIG_SERIAL_OMAP_UART2_DMA,
		.dma_rx_buf_size	=	CONFIG_SERIAL_OMAP_UART2_RXDMA_BUFSIZE,
		.dma_rx_timeout	=	CONFIG_SERIAL_OMAP_UART2_RXDMA_TIMEOUT,
#else
		.use_dma	=	0,
		.dma_rx_buf_size	=	0,
		.dma_rx_timeout	=	0,
#endif	/*	CONFIG_SERIAL_OMAP_UART2_DMA	*/
		.idle_timeout	=	CONFIG_SERIAL_OMAP_IDLE_TIMEOUT,
		.flags		=	1,
	},
	{
#if	defined(CONFIG_SERIAL_OMAP_UART3_DMA)
		.use_dma	=	CONFIG_SERIAL_OMAP_UART3_DMA,
		.dma_rx_buf_size	=	CONFIG_SERIAL_OMAP_UART3_RXDMA_BUFSIZE,
		.dma_rx_timeout	=	CONFIG_SERIAL_OMAP_UART3_RXDMA_TIMEOUT,
#else
		.use_dma	=	0,
		.dma_rx_buf_size	=	0,
		.dma_rx_timeout	=	0,
#endif	/*	CONFIG_SERIAL_OMAP_UART3_DMA	*/
		.idle_timeout	=	CONFIG_SERIAL_OMAP_IDLE_TIMEOUT,
		.flags		=	1,
	},
	{
#if	defined(CONFIG_SERIAL_OMAP_UART4_DMA)
		.use_dma	=	CONFIG_SERIAL_OMAP_UART4_DMA,
		.dma_rx_buf_size	=	CONFIG_SERIAL_OMAP_UART4_RXDMA_BUFSIZE,
		.dma_rx_timeout	=	CONFIG_SERIAL_OMAP_UART4_RXDMA_TIMEOUT,
#else
		.use_dma	=	0,
		.dma_rx_buf_size	=	0,
		.dma_rx_timeout	=	0,
#endif	/*	CONFIG_SERIAL_OMAP_UART3_DMA	*/
		.idle_timeout	=	CONFIG_SERIAL_OMAP_IDLE_TIMEOUT,
		.flags		=	1,
	},
	{
		.flags		=	0
	}
};


static	void	nowplus_init_camera(void)
{
	if	(gpio_request(OMAP_GPIO_CAMERA_LEVEL_CTRL,"OMAP_GPIO_CAMERA_LEVEL_CTRL")	!=	0)
	{
		printk("Could	not	request	GPIO	%d\n",	OMAP_GPIO_CAMERA_LEVEL_CTRL);
		return;
	}

	if	(gpio_request(OMAP_GPIO_CAM_EN,"OMAP_GPIO_CAM_EN")	!=	0)
	{
		printk("Could	not	request	GPIO	%d\n",	OMAP_GPIO_CAM_EN);
		return;
	}

	if	(gpio_request(OMAP_GPIO_ISP_INT,"OMAP_GPIO_ISP_INT")	!=	0)
	{
		printk("Could	not	request	GPIO	%d\n",	OMAP_GPIO_ISP_INT);
		return;
	}

	if	(gpio_request(OMAP_GPIO_CAM_RST,"OMAP_GPIO_CAM_RST")	!=	0)
	{
		printk("Could	not	request	GPIO	%d\n",	OMAP_GPIO_CAM_RST);
		return;
	}

	if	(gpio_request(OMAP_GPIO_VGA_STBY,"OMAP_GPIO_VGA_STBY")	!=	0)
	{
		printk("Could	not	request	GPIO	%d\n",	OMAP_GPIO_VGA_STBY);
		return;
	}

	if	(gpio_request(OMAP_GPIO_VGA_RST,"OMAP_GPIO_VGA_RST")	!=	0)
	{
		printk("Could	not	request	GPIO	%d",	OMAP_GPIO_VGA_RST);
		return;
	}

	gpio_direction_output(OMAP_GPIO_CAMERA_LEVEL_CTRL,	0);
	gpio_direction_output(OMAP_GPIO_CAM_EN,	0);
	gpio_direction_input(OMAP_GPIO_ISP_INT);
	gpio_direction_output(OMAP_GPIO_CAM_RST,	0);
	gpio_direction_output(OMAP_GPIO_VGA_STBY,	0);
	gpio_direction_output(OMAP_GPIO_VGA_RST,	0);


	gpio_free(OMAP_GPIO_CAMERA_LEVEL_CTRL);
	gpio_free(OMAP_GPIO_CAM_EN);
	gpio_free(OMAP_GPIO_ISP_INT);
	gpio_free(OMAP_GPIO_CAM_RST);
	gpio_free(OMAP_GPIO_VGA_STBY);
	gpio_free(OMAP_GPIO_VGA_RST);

}


static	void	mod_clock_correction(void)
{
	cm_write_mod_reg(0x00,	OMAP3430ES2_SGX_MOD,	CM_CLKSEL); // SGX_FCLK is CORE_CLK divided by 3
	cm_write_mod_reg(0x04,	OMAP3430_CAM_MOD,	    CM_CLKSEL); // CAM_MCLK is DPLL4 clock divided by 4
}

int	config_twl4030_resource_remap(	void	)
{
	int	ret	=	0;
	printk("Start	config_twl4030_resource_remap\n");

#if	1//TI	HS.Yoon	20101018	for	increasing	VIO	level	to	1.85V
	ret	=	twl_i2c_write_u8(	TWL4030_MODULE_PM_RECEIVER,	0x1,	TWL4030_VIO_VSEL	);
	if	(ret)
		printk("	board-file:	fail	to	set	TWL4030_VIO_VSEL\n");
#endif
	return	ret;
}

static	struct	omap_musb_board_data	musb_board_data	=	{
	.interface_type		=	MUSB_INTERFACE_ULPI,
	.mode			=	MUSB_OTG,
	.power			=	100,
};
#define	POWERUP_REASON	"androidboot.mode"
int	get_powerup_reason(char	*str)
{
	extern	char	*saved_command_line;
	char	*next,	*start	=	NULL;
	int	i;

	i	=	strlen(POWERUP_REASON);
	next	=	saved_command_line;

	while	((next	=	strchr(next,	'a'))	!=	NULL)	{
		if	(!strncmp(next,	POWERUP_REASON,	i))	{
			start	=	next;
			break;
		}	else	{
			next++;
		}
	}
	if	(!start)
		return	-EPERM;
	i	=	0;
	start	=	strchr(start,	'=')	+	1;
	while	(*start	!=	'	')	{
		str[i++]	=	*start++;
		if	(i	>	14)	{
			printk(KERN_INFO	"Invalid	Powerup	reason\n");
			return	-EPERM;
		}
	}
	str[i]	=	'\0';
	return	0;
}


static	inline	void	__init	nowplus_init_lcd(void)
{
	printk(KERN_ERR	"\n%s\n",__FUNCTION__);
	if	(gpio_request(OMAP_GPIO_MLCD_RST,	"OMAP_GPIO_MLCD_RST")	<	0	)	{
		printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d\n",OMAP_GPIO_MLCD_RST);
		return;
	}
	gpio_direction_output(OMAP_GPIO_MLCD_RST,1);

	if	(gpio_request(OMAP_GPIO_LCD_REG_RST,	"OMAP_GPIO_LCD_REG_RST")	<	0	)	{
		printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d\n",OMAP_GPIO_LCD_REG_RST);
		return;
	}
	gpio_direction_input(OMAP_GPIO_LCD_REG_RST);

	if	(gpio_request(OMAP_GPIO_LCD_ID,	"OMAP_GPIO_LCD_ID")	<	0	)	{
		printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d\n",OMAP_GPIO_LCD_ID);
		return;
	}
	gpio_direction_output(OMAP_GPIO_LCD_ID,0);

}


static	inline	void	__init	nowplus_init_fmradio(void)
{

	if(gpio_request(OMAP_GPIO_FM_nRST,	"OMAP_GPIO_FM_nRST")	<	0	){
	printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d\n",OMAP_GPIO_FM_nRST);
	return;
	}
	gpio_direction_output(OMAP_GPIO_FM_nRST,	0);
	//	mdelay(1);
	//	gpio_direction_output(OMAP_GPIO_FM_nRST,	1);	//temp	disable


	if(gpio_request(OMAP_GPIO_FM_INT,	"OMAP_GPIO_FM_INT")	<	0	){
	printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d\n",OMAP_GPIO_FM_INT);
	return;
	}
	gpio_direction_input(OMAP_GPIO_FM_INT);
}

// static	inline	void	__init	nowplus_init_mmc1(void)
// {
	// if(gpio_request(OMAP_GPIO_TF_DETECT,	"OMAP_GPIO_TF_DETECT")	<	0	){
		// printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d\n",OMAP_GPIO_TF_DETECT);
		// return;
		// }
		// gpio_direction_input(OMAP_GPIO_TF_DETECT);
		// set_irq_type(OMAP_GPIO_IRQ(OMAP_GPIO_TF_DETECT),	IRQ_TYPE_EDGE_BOTH);

	// return;
// }

// static	inline	void	__init	nowplus_init_movi_nand(void)
// {
	// if(gpio_request(OMAP_GPIO_MOVI_EN,	"OMAP_GPIO_MOVI_EN")	<	0	){
	// printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_MOVI_EN);
	// return;
	// }
	// gpio_direction_output(OMAP_GPIO_MOVI_EN,0);
// }

static	inline	void	__init	nowplus_init_sound(void)
{
	if(gpio_request(OMAP_GPIO_PCM_SEL,	"OMAP_GPIO_PCM_SEL")	<	0	){
	printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_PCM_SEL);
	return;
	}
	gpio_direction_output(OMAP_GPIO_PCM_SEL,0);

	if(gpio_request(OMAP_GPIO_TVOUT_SEL,	"OMAP_GPIO_TVOUT_SEL")	<	0	){
	printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_TVOUT_SEL);
	return;
	}
	gpio_direction_output(OMAP_GPIO_TVOUT_SEL,0);
}

static	inline	void	__init	nowplus_init_PL(void)
{
	if(gpio_request(OMAP_GPIO_PS_OUT,	"OMAP_GPIO_PS_OUT")	<	0	){
	printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_PS_OUT);
	return;
	}
	gpio_direction_input(OMAP_GPIO_PS_OUT);
}

static	inline	void	__init	nowplus_init_earphone(void)
{
    
    if(gpio_request(OMAP_GPIO_EAR_KEY, "OMAP_GPIO_EAR_KEY") < 0){
        printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d for OMAP_GPIO_EAR_KEY \n", OMAP_GPIO_EAR_KEY);
        return;
    }
    gpio_direction_input(OMAP_GPIO_EAR_KEY);
    
    if(gpio_request(OMAP_GPIO_DET_3_5,	"OMAP_GPIO_DET_3_5")	<	0	){
        printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_DET_3_5);
        return;
    }
    gpio_direction_input(OMAP_GPIO_DET_3_5);

    if(gpio_request(OMAP_GPIO_EAR_ADC_SEL, "OMAP_GPIO_EAR_ADC_SEL") < 0){
        printk(KERN_ERR "\n FAILED TO REQUEST GPIO %d for OMAP_GPIO_EAR_ADC_SEL \n", OMAP_GPIO_EAR_ADC_SEL);
        return;
    }
    gpio_direction_output(OMAP_GPIO_EAR_ADC_SEL, EAR_ADC_ON);
    
    if(gpio_request(OMAP_GPIO_EAR_MIC_LDO_EN,	"OMAP_GPIO_EAR_MIC_LDO_EN")	<	0	){
        printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_EAR_MIC_LDO_EN);
        return;
    }
    gpio_direction_output(OMAP_GPIO_EAR_MIC_LDO_EN,0);

    gpio_free(	OMAP_GPIO_DET_3_5	);
    gpio_free(	OMAP_GPIO_EAR_MIC_LDO_EN	);
}

static	inline	void	__init	nowplus_init_platform(void)
{
	/*PS HOLD*/
	if(gpio_request(OMAP_GPIO_PS_HOLD_PU,	"OMAP_GPIO_PS_HOLD_PU")	<	0	){
			printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_PS_HOLD_PU);
			return;
		}
#ifdef PS_HOLD_USE_PULL
	gpio_direction_input(OMAP_GPIO_PS_HOLD_PU);
#else
	gpio_direction_output(OMAP_GPIO_PS_HOLD_PU, 1);
#endif

	/*PDA ACTIVE*/
	if(gpio_request(OMAP_GPIO_PDA_ACTIVE,	"OMAP_GPIO_PDA_ACTIVE")	<	0	){
			printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_PDA_ACTIVE);
			return;
		}
	gpio_direction_output(OMAP_GPIO_PDA_ACTIVE, 1);
    
	/*NAND	INT_GPIO*/
	if(gpio_request(OMAP_GPIO_AP_NAND_INT,	"OMAP_GPIO_AP_NAND_INT")	<	0	){
			printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_AP_NAND_INT);
			return;
		}
	gpio_direction_input(OMAP_GPIO_AP_NAND_INT);

	if(gpio_request(OMAP_GPIO_IF_CON_SENSE,	"OMAP_GPIO_IF_CON_SENSE")	<	0	){
			printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_IF_CON_SENSE);
			return;
		}
	gpio_direction_input(OMAP_GPIO_IF_CON_SENSE);

	if(gpio_request(OMAP_GPIO_VF,	"OMAP_GPIO_VF")	<	0	){
			printk(KERN_ERR	"\n	FAILED	TO	REQUEST	GPIO	%d	\n",OMAP_GPIO_VF);
			return;
		}
	gpio_direction_input(OMAP_GPIO_VF);

}


static	void	__init	get_board_hw_rev(void)
{
	int	ret;

	ret	=	gpio_request(	OMAP_GPIO_HW_REV0,	"HW_REV0");
	if	(ret	<	0)
	{
		printk(	"fail	to	get	gpio	:	%d,	res	:	%d\n",	OMAP_GPIO_HW_REV0,	ret	);
		return;
	}

	ret	=	gpio_request(	OMAP_GPIO_HW_REV1,	"HW_REV1");
	if	(ret	<	0)
	{
		printk(	"fail	to	get	gpio	:	%d,	res	:	%d\n",	OMAP_GPIO_HW_REV1,	ret	);
		return;
	}

	ret	=	gpio_request(	OMAP_GPIO_HW_REV2,	"HW_REV2");
	if	(ret	<	0)
	{
		printk(	"fail	to	get	gpio	:	%d,	res	:	%d\n",	OMAP_GPIO_HW_REV2,	ret	);
		return;
	}

	gpio_direction_input(	OMAP_GPIO_HW_REV0	);
	gpio_direction_input(	OMAP_GPIO_HW_REV1	);
	gpio_direction_input(	OMAP_GPIO_HW_REV2	);

#if	1	//should	be	rev1.0
	hw_revision	=	100;	//
#else
	hw_revision	=	gpio_get_value(OMAP_GPIO_HW_REV0);
	hw_revision	|=	(gpio_get_value(OMAP_GPIO_HW_REV1)	<<	1);
	hw_revision	|=	(gpio_get_value(OMAP_GPIO_HW_REV2)	<<	2);
#endif


	gpio_free(	OMAP_GPIO_HW_REV0	);
	gpio_free(	OMAP_GPIO_HW_REV1	);
	gpio_free(	OMAP_GPIO_HW_REV2	);

	//	safe	mode	reconfiguration
	//omap_ctrl_writew(OMAP34XX_MUX_MODE7,	0x018C);
	//omap_ctrl_writew(OMAP34XX_MUX_MODE7,	0x0190);
	//omap_ctrl_writew(OMAP34XX_MUX_MODE7,	0x0192);

	//	REV03	:	000b,	REV04	:	001b,	REV05	:	010b,	REV06	:	011b
	printk("BOARD	HW	REV	:	%d\n",	hw_revision);
}

/*	integer	parameter	get/set	functions	*/
static	ssize_t	set_integer_param(param_idx	idx,	const	char	*buf,	size_t	size)
{
	int	ret	=	0;
	char	*after;
	unsigned	long	value	=	simple_strtoul(buf,	&after,	10);
	unsigned	int	count	=	(after	-	buf);
	if	(*after	&&	isspace(*after))
		count++;
	if	(count	==	size)	{
		ret	=	count;

		if	(sec_set_param_value)	sec_set_param_value(idx,	&value);
	}

	return	ret;
}
static	ssize_t	get_integer_param(param_idx	idx,	char	*buf)
{
	int	value;

	if	(sec_get_param_value)	sec_get_param_value(idx,	&value);

	return	sprintf(buf,	"%d\n",	value);
}
REGISTER_PARAM(__DEBUG_LEVEL,	debug_level);
REGISTER_PARAM(__DEBUG_BLOCKPOPUP,	block_popup);
static	void	*dev_attr[]	=	{
	&dev_attr_debug_level,
	&dev_attr_block_popup,
};
static	int	nowplus_param_sysfs_init(void)
{
	int	ret,	i	=	0;

	param_dev	=	device_create(sec_class,	NULL,	MKDEV(0,0),	NULL,	"param");

	if	(IS_ERR(param_dev))	{
		pr_err("Failed	to	create	device(param)!\n");
		return	PTR_ERR(param_dev);
	}

	for	(;	i	<	ARRAY_SIZE(dev_attr);	i++)	{
		ret	=	device_create_file(param_dev,	dev_attr[i]);
		if	(ret	<	0)	{
			pr_err("Failed	to	create	device	file(%s)!\n",
				((struct	device_attribute	*)dev_attr[i])->attr.name);
			goto	fail;
		}
	}

	return	0;

fail:
	for	(--i;	i	>=	0;	i--)
		device_remove_file(param_dev,	dev_attr[i]);

	return	-1;
}

#if	defined(CONFIG_SAMSUNG_SETPRIO)
/*	begins	-	andre.b.kim	:	added	sec_setprio	as	module	{	*/
static	ssize_t	sec_setprio_show(struct	device	*dev,	struct	device_attribute	*attr,	char	*buf)
{
	if	(sec_setprio_get_value)	{
		sec_setprio_get_value();
	}

	return	0;
}

static	ssize_t	sec_setprio_store(struct	device	*dev,	struct	device_attribute	*attr,	const	char	*buf,	size_t	size)
{
	if	(sec_setprio_set_value)	{
		sec_setprio_set_value(buf);
	}

	return	size;
}

static	DEVICE_ATTR(sec_setprio,	S_IRUGO	|	S_IWUSR	|	S_IWOTH	|	S_IXOTH,	sec_setprio_show,	sec_setprio_store);

static	int	nowplus_sec_setprio_sysfs_init(void)
{
	/*	file	creation	at	'/sys/class/sec_setprio/sec_setprio	*/
	sec_setprio_class	=	class_create(THIS_MODULE,	"sec_setprio");
	if	(IS_ERR(sec_setprio_class))
		pr_err("Failed	to	create	class	(sec_setprio)\n");

	sec_set_prio	=	device_create(sec_setprio_class,	NULL,	0,	NULL,	"sec_setprio");
	if	(IS_ERR(sec_set_prio))
		pr_err("Failed	to	create	device(sec_set_prio)!\n");
	if	(device_create_file(sec_set_prio,	&dev_attr_sec_setprio)	<	0)
		pr_err("Failed	to	create	device	file(%s)!\n",	dev_attr_sec_setprio.attr.name);

	return	0;
}
/*	}	ends	-	andre.b.kim	:	added	sec_setprio	as	module	*/
#endif

static	void	synaptics_dev_init(void)
{
		/*	Set	the	ts_gpio	pin	mux	*/
		if	(gpio_request(OMAP_GPIO_TOUCH_IRQ,	"touch_synaptics")	<	0)	{
				printk(KERN_ERR	"can't	get	synaptics	pen	down	GPIO\n");
				return;
		}
		gpio_direction_input(OMAP_GPIO_TOUCH_IRQ);
}

static	void	enable_board_wakeup_source(void)
{
	/*	T2	interrupt	line	(keypad)	*/
	omap_mux_init_signal("sys_nirq",	OMAP_WAKEUP_EN	|	OMAP_PIN_INPUT_PULLUP);
}
int set_wakeup_gpio( unsigned int gpio[], int cnt )
{
	int ret = 0;
	int i;

	for( i = 0 ; i < cnt ; i++ ) 
		enable_irq_wake( OMAP_GPIO_IRQ( gpio[i] ) );

	return ret;
}

unsigned int wakeup_gpio[] = {
	OMAP_GPIO_USBSW_NINT,
	OMAP_GPIO_INT_ONEDRAM_AP,
	OMAP_GPIO_TF_DETECT,
	OMAP_GPIO_KEY_PWRON,
	OMAP_GPIO_EAR_KEY,
	OMAP_GPIO_DET_3_5,
};
static	const	struct	usbhs_omap_platform_data	usbhs_pdata	__initconst	=	{
	.port_mode[0]		=	OMAP_USBHS_PORT_MODE_UNUSED,
	.port_mode[1]		=	OMAP_EHCI_PORT_MODE_PHY,
	.port_mode[2]		=	OMAP_USBHS_PORT_MODE_UNUSED,
	.phy_reset			=	true,
	.reset_gpio_port[0]	=	-EINVAL,
	.reset_gpio_port[1]	=	64,
	.reset_gpio_port[2]	=	-EINVAL,
};

void	__init	nowplus_peripherals_init(void)
{
    nowplus_i2c_init();

	board_onenand_init();
	synaptics_dev_init();
	omap_serial_init(omap_serial_platform_data);
	usb_musb_init(&musb_board_data);
#if defined(CONFIG_VIDEO_OMAP3) || \
	defined(CONFIG_VIDEO_OMAP3_MODULE)
	mod_clock_correction();
#endif
	enable_board_wakeup_source();

    nowplus_ramconsole_init();
	nowplus_init_power_key();
	nowplus_init_earphone();
	nowplus_init_battery();
	nowplus_init_platform();
	nowplus_init_lcd();
	nowplus_init_PL();
	nowplus_init_fmradio();
	nowplus_init_camera();
	nowplus_wifi_init();
	
    /*	For	Display	*/
    spi_register_board_info(nowplus_spi_board_info,	ARRAY_SIZE(nowplus_spi_board_info));
	omap_display_init(&nowplus_dss_data);
}


static	void	__init	nowplus_init_irq(void)
{
	omap_board_config	=	nowplus_config;
	omap_board_config_size	=	ARRAY_SIZE(nowplus_config);
    omap3_pm_init_cpuidle(omap3_cpuidle_params_table);
	omap2_init_common_hw(nowplus_sdrc_params,	NULL);
	omap2_gp_clockevent_set_gptimer(1);
	omap_init_irq();
}

static	void	__init	nowplus_init(void)
{
	char	str_powerup_reason[15];
	u32	regval;

	printk("\n.....[Nowplus]	Initializing...\n");

	omap3_mux_init(board_mux,	OMAP_PACKAGE_CBB);

	set_wakeup_gpio(wakeup_gpio, ARRAY_SIZE(wakeup_gpio));
	msecure_init();

	//	change	reset	duration	(PRM_RSTTIME	register)
	regval	=	omap_readl(0x48307254);
	regval	|=	0xFF;
	omap_writew(regval,	0x48307254);

	get_powerup_reason(str_powerup_reason);
	printk(	"\n\n	<Powerup	Reason	:	%s>\n\n",	str_powerup_reason);

// debug: get video ram addres from SBL
// 0x4805 0480+ j * 0x04    printk(    "DISPC_GFX_BA0: 0x%08x\n", omap_readl(0x48050480));    printk(    "DISPC_GFX_BA1: 0x%08x\n", omap_readl(0x48050484));    printk(    "DISPC_GFX_POSITION: %dx%d\n", omap_readl(0x48050488)&0xff, (omap_readl(0x48050488)>>16)&0xf-//debug 0x4805 0480+ j * 0x04
    // printk(    " --- SBL dump -->\n"); 
    // printk(    "    DISPC_GFX_BA0: 0x%08x\n", omap_readl(0x48050480));
    // printk(    "    GFXFORMAT: 0x%01x\n", omap_readl(0x480504A0)>>1&0xf);
    // printk(    "    DISPC_GFX_POSITION: %dx%d\n", omap_readl(0x48050488)&0xff, (omap_readl(0x48050488)>>16)&0xff );
    // printk(    "    DISPC_GFX_SIZE: %dx%d\n", omap_readl(0x4805048C)&0xff, (omap_readl(0x4805048C)>>16)&0xff );
    // printk(    "    ---\n");   
    // printk(    "    CM_CLKSEL1_PLL: %dx%d\n", omap_readl(0x48004D40));
    regval = omap_readl(OMAP343X_SCRATCHPAD + 4);
    printk(    "BOOT CMD: %c%c%c\n", (regval>>24)&0xff, (regval>>16)&0xff, regval&0xff);

    // printk(    " <-- SBL dump ---\n"); 


	platform_add_devices(nowplus_devices,	ARRAY_SIZE(nowplus_devices));
//	For	Regulator	Framework	:
	nowplus_vaux3_supply.dev	=	&nowplus_lcd_device.dev;
	nowplus_vaux4_supply.dev	=	&nowplus_lcd_device.dev;
	nowplus_vpll2_supply.dev	=	&nowplus_lcd_device.dev;

	get_board_hw_rev();

	sec_class	=	class_create(THIS_MODULE,	"sec");
	if	(IS_ERR(sec_class))
		pr_err("Failed	to	create	class(sec)!\n");
	
	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");
		
	nowplus_param_sysfs_init();

#if	defined(CONFIG_SAMSUNG_SETPRIO)
	nowplus_sec_setprio_sysfs_init();
#endif

	nowplus_peripherals_init();
	usb_uhhtll_init(&usbhs_pdata);
		
#ifdef	CONFIG_OMAP_SMARTREFLEX_CLASS3
	sr_class3_init();
#endif
		
#ifdef	CONFIG_PM
#ifdef	CONFIG_TWL4030_CORE
	omap_voltage_register_pmic(&omap_pmic_core,	"core");
	omap_voltage_register_pmic(&omap_pmic_mpu,	"mpu");
#endif
	omap_voltage_init_vc(&vc_config);
#endif
}

#if 0
static	void	__init	bootloader_reserve_sdram(void)
{
	u32	paddr;
	u32	size	=	0x80000;

	paddr	=	0x8FB80000; //0x8fc00000; //fb addr set by Samsung SBL

	paddr	-=	size;

	if	(reserve_bootmem(paddr,	size,	BOOTMEM_EXCLUSIVE)	<	0)	{
		pr_err("FB:	failed	to	reserve	VRAM\n");
	}
}
#endif

static	void	__init	nowplus_map_io(void)
{
	omap2_set_globals_343x();
	omap34xx_map_common_io();
	omap2_ramconsole_reserve_sdram();
	//bootloader_reserve_sdram();
}

static	void	__init	nowplus_fixup(struct	machine_desc	*desc,
					struct	tag	*tags,	char	**cmdline,
					struct	meminfo	*mi)
{
	mi->bank[0].start	=	0x80000000;
	mi->bank[0].size	=	256	*	SZ_1M;	//	DDR	256MB
	mi->bank[0].node	=	0;

	mi->nr_banks	=	1;

}

MACHINE_START(NOWPLUS,	"Samsung	NOWPLUS	board")
	.phys_io		=	0x48000000,
	.io_pg_offst	=	((0xfa000000)	>>	18)	&	0xfffc,
	.boot_params	=	0x80000100,
	.fixup			=	nowplus_fixup,
	.map_io			=	nowplus_map_io,
	.init_irq		=	nowplus_init_irq,
	.init_machine	=	nowplus_init,
	.timer			=	&omap_timer,
MACHINE_END
