/*
 * input/touchscreen/omap/omap_ts.c
 *
 * touchscreen input device driver for various TI OMAP boards
 * Copyright (c) 2002 MontaVista Software Inc.
 * Copyright (c) 2004 Texas Instruments, Inc.
 * Cleanup and modularization 2004 by Dirk Behme <dirk.behme@de.bosch.com>
 *
 * Assembled using driver code copyright the companies above.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * History:
 * 12/12/2004    Srinath Modified and intergrated code for H2 and H3
 *
 */
#ifdef CONFIG_SAMSUNG_ARCHER_TARGET_SK 
// #define FEATURE_MOVE_LIMIT 
// #define TOUCH_DEVICE 
#define TOUCH_PROC 
//#define FEATURE_2_MORE_MULTI_TOUCH
#endif
#ifdef CONFIG_MACH_HALO
 #define FEATURE_MOVE_LIMIT 
// #define TOUCH_DEVICE 
#endif

 #define RESERVED_T0                               0u
#define RESERVED_T1                               1u
#define DEBUG_DELTAS_T2                           2u
#define DEBUG_REFERENCES_T3                       3u
#define DEBUG_SIGNALS_T4                          4u
#define GEN_MESSAGEPROCESSOR_T5                   5u
#define GEN_COMMANDPROCESSOR_T6                   6u
#define GEN_POWERCONFIG_T7                        7u
#define GEN_ACQUISITIONCONFIG_T8                  8u
#define TOUCH_MULTITOUCHSCREEN_T9                 9u
#define TOUCH_SINGLETOUCHSCREEN_T10               10u
#define TOUCH_XSLIDER_T11                         11u
#define TOUCH_YSLIDER_T12                         12u
#define TOUCH_XWHEEL_T13                          13u
#define TOUCH_YWHEEL_T14                          14u
#define TOUCH_KEYARRAY_T15                        15u
#define PROCG_SIGNALFILTER_T16                    16u
#define PROCI_LINEARIZATIONTABLE_T17              17u
#define SPT_COMCONFIG_T18                         18u
#define SPT_GPIOPWM_T19                           19u
#define PROCI_GRIPFACESUPPRESSION_T20             20u
#define RESERVED_T21                              21u
#define PROCG_NOISESUPPRESSION_T22                22u
#define TOUCH_PROXIMITY_T23	                      23u
#define PROCI_ONETOUCHGESTUREPROCESSOR_T24        24u
#define SPT_SELFTEST_T25                          25u
#define DEBUG_CTERANGE_T26                        26u
#define PROCI_TWOTOUCHGESTUREPROCESSOR_T27        27u
#define SPT_CTECONFIG_T28                         28u
#define SPT_GPI_T29                               29u
#define SPT_GATE_T30                              30u
#define TOUCH_KEYSET_T31                          31u
#define TOUCH_XSLIDERSET_T32                      32u

#define  OMAP_GPIO_TSP_INT 186
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/platform_device.h>
// #include <asm/semaphore.h>
#include <linux/semaphore.h>	// ryun
#include <asm/mach-types.h>

//#include <asm/arch/gpio.h>
//#include <asm/arch/mux.h>
#include <plat/gpio.h>	//ryun
#include <plat/mux.h>	//ryun 
#include <linux/delay.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <linux/firmware.h>
//#include "dprintk.h"
//#include "message.h"
#include <linux/time.h>

#include <linux/i2c/twl.h>	// ryun 20091125 
#include <linux/earlysuspend.h>	// ryun 20200107 for early suspend
#include <plat/omap-pm.h>	// ryun 20200107 for touch boost

#ifdef TOUCH_PROC
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#endif

#include <linux/wakelock.h>
#include "atmel_touch.h"

extern unsigned char g_version;
extern Atmel_model_type g_model;
extern uint8_t cal_check_flag;//20100208
#include <mach/archer.h>
#include <plat/mux_archer_rev_r03.h>

extern void disable_irq_nosync(unsigned int);
extern void enable_irq(unsigned int);

//#define TEST_TOUCH_KEY_IN_ATMEL


#define PRINT_FUNCTION_ENTER  //printk("%s", __FUNCTION__)

// #define TOUCH_BOOST   1
#define VDD1_OPP6_FREQ         720000000
#define VDD1_OPP5_FREQ         600000000
#define VDD1_OPP4_FREQ         550000000
#define VDD1_OPP3_FREQ         500000000
#define VDD1_OPP1_FREQ         125000000
//modified for samsung customisation -touchscreen 
unsigned short enable_touch_boost;
static ssize_t ts_show(struct kobject *, struct kobj_attribute *, char *);
static ssize_t ts_store(struct kobject *k, struct kobj_attribute *,
			  const char *buf, size_t n);
static ssize_t firmware_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t firmware_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size);
//modified for samsung customisation

#define __CONFIG_ATMEL__

struct	semaphore	sem_touch_onoff;
struct	semaphore	sem_touch_handler;

#define TOUCHSCREEN_NAME			"touchscreen"
#define DEFAULT_PRESSURE_UP		0
#define DEFAULT_PRESSURE_DOWN		256

static struct touchscreen_t tsp;
//static struct work_struct tsp_work;	//	 ryun
static struct workqueue_struct *tsp_wq;
//static int g_touch_onoff_status = 0;
static int g_enable_touchscreen_handler = 0;	// fixed for i2c timeout error.
//static unsigned int g_version_read_addr = 0;
//static unsigned short g_position_read_addr = 0;

#define IS_ATMEL	1	// ryun

#define TOUCH_MENU	  KEY_MENU
#define TOUCH_SEARCH  KEY_SEARCH 
#define TOUCH_HOME  KEY_HOME
#define TOUCH_BACK	  KEY_BACK


#if defined(CONFIG_SAMSUNG_ARCHER_TARGET_SK) 
#define MAX_TOUCH_X_RESOLUTION	480 // 480
#define MAX_TOUCH_Y_RESOLUTION	800 // 800
int atmel_ts_tk_keycode[] =
{ TOUCH_MENU, TOUCH_BACK};
#elif CONFIG_MACH_HALO 
#define MAX_TOUCH_X_RESOLUTION	480 // 480
#define MAX_TOUCH_Y_RESOLUTION	800 // 800
int atmel_ts_tk_keycode[] =
{ TOUCH_MENU, TOUCH_BACK};
#elif defined(CONFIG_MACH_TICKERTAPE)
//#if (CONFIG_TICKER_REV <= CONFIG_TICKER_EMU01)
//#define MAX_TOUCH_X_RESOLUTION	1024
//#define MAX_TOUCH_Y_RESOLUTION	600 
//#else
#define MAX_TOUCH_X_RESOLUTION	480
#define MAX_TOUCH_Y_RESOLUTION	992 
//#endif
#define TICKERTAPE_SFN  //jihyon82.kim for Tickertape SFN


#define KEY_START_X 0
#define KEY_START_Y 800
#define KEY_END_X 480
#define KEY_END_Y (KEY_START_Y + 92)
#define MENU_START_X (KEY_START_X)
#define SEARCH_START_X (KEY_START_X + 120)
#define HOME_START_X  (KEY_START_X + 240)
#define BACK_START_X (KEY_START_X + 360)

int atmel_ts_tk_keycode[] =
{ TOUCH_MENU, TOUCH_SEARCH, TOUCH_HOME, TOUCH_BACK};

#elif defined(CONFIG_MACH_OSCAR)
#define MAX_TOUCH_X_RESOLUTION	480 // 480
#define MAX_TOUCH_Y_RESOLUTION	800 // 800
int atmel_ts_tk_keycode[] = {}; // dummy for build
#endif

struct touchscreen_t;

struct touchscreen_t {
	struct input_dev * inputdevice;
	struct timer_list ts_timer;
	int touched;
	int irq;
	int irq_type;
	int irq_enabled;
	struct device *dev;
	spinlock_t lock;
	struct early_suspend	early_suspend;// ryun 20200107 for early suspend
	struct work_struct  tsp_work;	// ryun 20100107 
	struct timer_list opp_set_timer;	// ryun 20100107 for touch boost
	struct work_struct constraint_wq;
	int opp_high;	// ryun 20100107 for touch boost	
};

#ifdef CONFIG_HAS_EARLYSUSPEND
void atmel_ts_early_suspend(struct early_suspend *h);
void atmel_ts_late_resume(struct early_suspend *h);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

#ifdef FEATURE_MOVE_LIMIT
int pre_x, pre_y, pre_size;
#endif

#ifdef TOUCH_PROC
int touch_proc_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
int touch_proc_read(char *page, char **start, off_t off,int count, int *eof, void *data);
int touch_proc_write(struct file *file, const char __user *buffer, unsigned long count, void *data);

#define IOCTL_TOUCH_PROC 'T'
enum 
{
        TOUCH_GET_VERSION = _IOR(IOCTL_TOUCH_PROC, 0, char*),
        TOUCH_GET_T_KEY_STATE = _IOR(IOCTL_TOUCH_PROC, 1, int),
        TOUCH_GET_SW_VERSION = _IOR(IOCTL_TOUCH_PROC, 2, char*),
};

const char fw_version[10]="0X16";
struct proc_dir_entry *touch_proc;
struct file_operations touch_proc_fops = 
{
        .ioctl=touch_proc_ioctl,
};
#endif

#ifdef FEATURE_2_MORE_MULTI_TOUCH
// function definition and declaration of variables
void handle_2_more_touch(uint8_t *atmel_msg);

typedef struct
{
	uint8_t id;			/*!< id */
	int8_t status;		/*!< dn=1, up=0, none=-1 */
	int16_t x;			/*!< X */
	int16_t y;			/*!< Y */
	unsigned int size ;		/*!< size */
} report_finger_info_t;

#define MAX_NUM_FINGER	10

static report_finger_info_t fingerInfo[MAX_NUM_FINGER];
#endif

struct wake_lock tsp_firmware_wake_lock;

// [[ ryun 20100113 
typedef struct
{
	int x;
	int y;
	int press;
} dec_input;

static dec_input id2 = {0};
static dec_input id3 = {0};
static dec_input tmp2 = {0};
static dec_input tmp3 = {0};

#if defined(CONFIG_SAMSUNG_ARCHER_TARGET_SK) && defined(FEATURE_MOVE_LIMIT)  
#define REPORT( touch, width, x, y) \
{	input_report_abs(tsp.inputdevice, ABS_MT_TOUCH_MAJOR, touch ); \
	input_report_abs(tsp.inputdevice, ABS_MT_WIDTH_MAJOR, width ); \
	input_report_abs(tsp.inputdevice, ABS_MT_POSITION_X, x); \
	input_report_abs(tsp.inputdevice, ABS_MT_POSITION_Y, y); \
	input_mt_sync(tsp.inputdevice);\
        pre_x = x; pre_y = y; pre_size = width; \
	}
#elif defined(CONFIG_MACH_HALO)
#define REPORT( touch, width, x, y) \
{	input_report_abs(tsp.inputdevice, ABS_MT_TOUCH_MAJOR, touch ); \
	input_report_abs(tsp.inputdevice, ABS_MT_WIDTH_MAJOR, width ); \
	input_report_abs(tsp.inputdevice, ABS_MT_POSITION_X, x); \
	input_report_abs(tsp.inputdevice, ABS_MT_POSITION_Y, y); \
	input_mt_sync(tsp.inputdevice);\
        pre_x = x; pre_y = y; pre_size = width; \
	}
#else
#define REPORT( touch, width, x, y) \
{	input_report_abs(tsp.inputdevice, ABS_MT_TOUCH_MAJOR, touch ); \
	input_report_abs(tsp.inputdevice, ABS_MT_WIDTH_MAJOR, width ); \
	input_report_abs(tsp.inputdevice, ABS_MT_POSITION_X, x); \
	input_report_abs(tsp.inputdevice, ABS_MT_POSITION_Y, y); \
	input_mt_sync(tsp.inputdevice); }
#endif	

// ]] ryun 20100113 


void read_func_for_only_single_touch(struct work_struct *work);
void read_func_for_multi_touch(struct work_struct *work);
void handle_keyarray(uint8_t * atmel_msg);
void handle_multi_touch(uint8_t *atmel_msg);
void handle_keyarray_of_archer(uint8_t * atmel_msg);
void initialize_multi_touch(void);



void (*atmel_handler_functions[MODEL_TYPE_MAX])(struct work_struct *work) =
{
    read_func_for_only_single_touch, // default handler
    read_func_for_multi_touch,        // ARCHER_KOR_SK
    read_func_for_only_single_touch, // TICKER_TAPE
    read_func_for_only_single_touch //
};

static irqreturn_t touchscreen_handler(int irq, void *dev_id);
static int touchscreen_poweronoff(int power_state);
void set_touch_irq_gpio_init(void);
void set_touch_irq_gpio_disable(void);	// ryun 20091203

//samsung customisation
static struct kobj_attribute touch_boost_attr =
	__ATTR(touch_boost, 0644, ts_show, ts_store);
static struct kobj_attribute firmware_attr =
	__ATTR(firmware, 0664, firmware_show, firmware_store);

/*------------------------------ for tunning ATmel - start ----------------------------*/
extern  ssize_t set_power_show(struct device *dev, struct device_attribute *attr, char *buf);
extern  ssize_t set_power_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
extern  ssize_t set_acquisition_show(struct device *dev, struct device_attribute *attr, char *buf);
extern  ssize_t set_acquisition_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
extern  ssize_t set_touchscreen_show(struct device *dev, struct device_attribute *attr, char *buf);
extern  ssize_t set_touchscreen_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
extern  ssize_t set_keyarray_show(struct device *dev, struct device_attribute *attr, char *buf);
extern  ssize_t set_keyarray_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
extern  ssize_t set_grip_show(struct device *dev, struct device_attribute *attr, char *buf);
extern  ssize_t set_grip_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
extern  ssize_t set_noise_show(struct device *dev, struct device_attribute *attr, char *buf);
extern  ssize_t set_noise_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
extern  ssize_t set_total_show(struct device *dev, struct device_attribute *attr, char *buf);
extern  ssize_t set_total_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
extern  ssize_t set_write_show(struct device *dev, struct device_attribute *attr, char *buf);
extern  ssize_t set_write_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);

#if 1 
extern ssize_t set_tsp_for_extended_indicator_show(struct device *dev, struct device_attribute *attr, char *buf);
extern ssize_t set_tsp_for_extended_indicator_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
#endif
// 20100325 / ryun / add for jump limit control 
extern ssize_t set_tsp_for_inputmethod_show(struct device *dev, struct device_attribute *attr, char *buf);
extern ssize_t set_tsp_for_inputmethod_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);


#ifdef FEATURE_TSP_FOR_TA
extern ssize_t set_tsp_for_TA_show(struct device *dev, struct device_attribute *attr, char *buf);
extern ssize_t set_tsp_for_TA_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
#endif

#ifdef FEATURE_DYNAMIC_SLEEP
extern ssize_t set_sleep_way_of_TSP_show(struct device *dev, struct device_attribute *attr, char *buf);
extern ssize_t set_sleep_way_of_TSP_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
#endif


extern ssize_t set_TSP_debug_on_show(struct device *dev, struct device_attribute *attr, char *buf);
extern ssize_t set_TSP_debug_on_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);
// kmj_DE31
extern ssize_t set_test_value_show(struct device *dev, struct device_attribute *attr, char *buf);
extern ssize_t set_test_value_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size);

// kmj_df08
ssize_t TSP_filter_status_show(struct device *dev, struct device_attribute *attr, char *buf);

static DEVICE_ATTR(set_power, S_IRWXU|S_IRWXG, set_power_show, set_power_store);
static DEVICE_ATTR(set_acquisition, S_IRWXU|S_IRWXG, set_acquisition_show, set_acquisition_store);
static DEVICE_ATTR(set_touchscreen, S_IRWXU|S_IRWXG, set_touchscreen_show, set_touchscreen_store);
static DEVICE_ATTR(set_keyarray, S_IRWXU|S_IRWXG, set_keyarray_show, set_keyarray_store);
static DEVICE_ATTR(set_grip , S_IRWXU|S_IRWXG, set_grip_show, set_grip_store);
static DEVICE_ATTR(set_noise, S_IRWXU|S_IRWXG, set_noise_show, set_noise_store);
static DEVICE_ATTR(set_total, S_IRWXU|S_IRWXG, set_total_show, set_total_store);
static DEVICE_ATTR(set_write, S_IRWXU|S_IRWXG, set_write_show, set_write_store);
#if 1 
static DEVICE_ATTR(set_tsp_for_extended_indicator, S_IRWXU|S_IRWXG, set_tsp_for_extended_indicator_show, set_tsp_for_extended_indicator_store);
#endif
// 20100325 / ryun / add for jump limit control 
static DEVICE_ATTR(set_tsp_for_inputmethod, S_IRWXU|S_IRWXG, set_tsp_for_inputmethod_show, set_tsp_for_inputmethod_store);


#ifdef FEATURE_TSP_FOR_TA
static DEVICE_ATTR(set_tsp_for_TA, S_IRWXU|S_IRWXG, set_tsp_for_TA_show, set_tsp_for_TA_store);
#endif

#ifdef FEATURE_DYNAMIC_SLEEP
static DEVICE_ATTR(set_sleep_way_of_TSP, S_IRWXU|S_IRWXG, set_sleep_way_of_TSP_show, set_sleep_way_of_TSP_store);
#endif


static DEVICE_ATTR(set_TSP_debug_on, S_IRWXU|S_IRWXG, set_TSP_debug_on_show, set_TSP_debug_on_store);
// kmj_De31
static DEVICE_ATTR(set_test_value, S_IRWXU|S_IRWXG, set_test_value_show, set_test_value_store);

// kmj_DF08
static DEVICE_ATTR(TSP_filter_status, S_IRUSR | S_IRGRP , TSP_filter_status_show, NULL);
/*------------------------------ for tunning ATmel - end ----------------------------*/

extern void restore_acquisition_config(void);
extern void restore_power_config(void);
extern uint8_t calibrate_chip(void);

#ifdef FEATURE_CALIBRATION_BY_KEY
extern unsigned char first_key_after_probe_or_wake_up;
#endif

static unsigned char menu_button = 0;
static unsigned char back_button = 0;


#ifdef FEATURE_VERIFY_INCORRECT_PRESS
static unsigned char figure_x_count(int id, int x);
static init_acc_count(int id);
int incorrect_repeat_count=4;
//definition
#endif

// kmj_DE03
unsigned char debug_print_on = 1; 

extern uint8_t report_id_to_type(uint8_t report_id, uint8_t *instance);

static ssize_t ts_show(struct kobject *kobj, struct kobj_attribute *attr,
			 char *buf)
{
	if (attr == &touch_boost_attr)
		return sprintf(buf, "%hu\n", enable_touch_boost);
	
	else
		return -EINVAL;

}
static ssize_t ts_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t n)
{
	unsigned short value;

	if (sscanf(buf, "%hu", &value) != 1) {
	        // kmj_DE03
	        if(debug_print_on)
        		printk(KERN_ERR "ts_store: Invalid value\n");
		return -EINVAL;
	}

	if (attr == &touch_boost_attr) {
		enable_touch_boost = value;
	} 

	 else {
		return -EINVAL;
	}

	return n;
}
//samsung customisation

//[[ ryun 20100107 for touch boost
static void tsc_timer_out (unsigned long v)
	{
		schedule_work(&(tsp.constraint_wq));
		return;
	}

void tsc_remove_constraint_handler(struct work_struct *work)
{
		omap_pm_set_min_mpu_freq(tsp.dev, VDD1_OPP1_FREQ);
                tsp.opp_high = 0;
}

//]] ryun 20200107 for touch boost

/* ryun 20091125 
struct omap3430_pin_config touch_i2c_gpio_init[] = {
	
	OMAP3430_PAD_CFG("TOUCH I2C SCL", OMAP3430_PAD_I2C_TOUCH_SCL, PAD_MODE0, PAD_INPUT_PU, PAD_OFF_PU, PAD_WAKEUP_NA),
	OMAP3430_PAD_CFG("TOUCH I2C SDA", OMAP3430_PAD_I2C_TOUCH_SDA, PAD_MODE0, PAD_INPUT_PU, PAD_OFF_PU, PAD_WAKEUP_NA),
};

struct omap3430_pin_config touch_irq_gpio_init[] = {
	OMAP3430_PAD_CFG("TOUCH INT",		OMAP3430_PAD_TOUCH_INT, PAD_MODE4, PAD_INPUT_PU, PAD_OFF_PU, PAD_WAKEUP_NA),
};
*/
void set_touch_i2c_gpio_init(void)
{
        // kmj_DE03
        if(debug_print_on)
        	printk("[TSP] %s() \n", __FUNCTION__);
/*
	omap3430_pad_set_configs(touch_i2c_gpio_init, ARRAY_SIZE(touch_i2c_gpio_init));

	omap_set_gpio_direction(OMAP_GPIO_TSP_SCL, GPIO_DIR_INPUT);
	omap_set_gpio_direction(OMAP_GPIO_TSP_SDA, GPIO_DIR_INPUT);

//	omap_set_gpio_direction(OMAP_GPIO_TSP_INT, GPIO_DIR_OUTPUT);
//	omap_set_gpio_dataout(OMAP_GPIO_TSP_INT, GPIO_LEVEL_HIGH);
*/
    return;
}

void set_touch_irq_gpio_init(void)
{
        // kmj_DE03
        if(debug_print_on)
        	printk("[TSP] %s() \n", __FUNCTION__);

#if 0 // SASKEN - gpio_request is already made in touchscreen_init(), this one bound to fail
	if (gpio_request(OMAP_GPIO_TSP_INT, "atmel_touch") < 0) {
	        if(debug_print_on)
        		printk(KERN_ERR "can't get atmel pen down GPIO\n");
		return;
	}
#endif
	gpio_direction_input(OMAP_GPIO_TSP_INT);
//	omap_set_gpio_debounce(OMAP_SYNAPTICS_GPIO, 0);  //  added this to remove the debouncing feature
   // ]] ryun 20091125
	
//	omap3430_pad_set_configs(touch_irq_gpio_init, ARRAY_SIZE(touch_irq_gpio_init));	// ryun
//	omap_set_gpio_direction(OMAP_GPIO_TSP_INT, GPIO_DIR_INPUT);
    return;
}

// [[ ryun 20091203
void set_touch_irq_gpio_disable(void)
{
        if(debug_print_on)
        	printk("[TSP] %s() \n", __FUNCTION__);
	if(g_enable_touchscreen_handler == 1)
	{
		free_irq(tsp.irq, &tsp);
	gpio_free(OMAP_GPIO_TSP_INT);
		g_enable_touchscreen_handler = 0;
	}	
    return;
}
// ]] ryun 20091203

unsigned char get_touch_irq_gpio_value(void)			
{
	//return gpio_get_value(GPIO_TOUCH_IRQ);
	return gpio_get_value(OMAP_GPIO_TSP_INT);
// ***********************************************************************
// NOTE: HAL function: User adds/ writes to the body of this function
// ***********************************************************************
}

void atmel_touchscreen_poweron(void)
{
        if(debug_print_on)
        	printk("[TSP] %s() \n", __FUNCTION__);
	// ryun 20091125 power up 
	if(touchscreen_poweronoff(1)<0)
	        if(debug_print_on)
        		printk("[TSP] %s() - power on error!!! \n", __FUNCTION__ );	// ryun 20091125 
/*
	if(tsp_rhandle != NULL)
		resource_request(tsp_rhandle, T2_VAUX2_2V80);
	else
	{
		printk("[TS][ERR] %s() enable_tsp_pins() - tsp_rhandle is NULL\n", __FUNCTION__);
		return;
	}
	mdelay(200);

	printk("[TS] atmel touchscreen power on\n");
*/
	return;
}

#define U8	__u8
#define  U16 	unsigned short int
#define READ_MEM_OK                 1u


extern unsigned int g_i2c_debugging_enable;
extern U8 read_mem(U16 start, U8 size, U8 *mem);
extern uint16_t message_processor_address;
extern uint8_t max_message_length;
extern uint8_t *atmel_msg;
extern uint8_t QT_Boot(uint8_t qt_force_update);
extern unsigned long simple_strtoul(const char *,char **,unsigned int);
extern unsigned char g_version, g_build, qt60224_notfound_flag;

extern void check_chip_calibration(unsigned char one_touch_input_flag);

/* firmware 2009.09.24 CHJ - end 1/2 */


void disable_tsp_irq(void)
{
        if(debug_print_on)
        	printk("[ryun] %s :: tsp.irq=%d",__func__,  tsp.irq);
	//disable_irq(tsp.irq);
	disable_irq_nosync(tsp.irq);
}

void enable_tsp_irq(void)
{
        if(debug_print_on)
        	printk("[ryun] %s :: tsp.irq=%d",__func__,  tsp.irq);
	//enable_irq(tsp.irq);	
	enable_irq(tsp.irq);
}

static ssize_t firmware_show(struct device *dev, struct device_attribute *attr, char *buf)
{	// v1.2 = 18 , v1.4 = 20 , v1.5 = 21
        if(debug_print_on)
        {
        	printk("[TSP] QT602240 Firmware Ver.\n");
        	printk("[TSP] version = %x\n", g_version);
        	printk("[TSP] Build = %x\n", g_build);
        }	
	//	printk("[TSP] version = %d\n", info_block->info_id.version);
	//	sprintf(buf, "QT602240 Firmware Ver. %x\n", info_block->info_id.version);
//	sprintf(buf, "QT602240 Firmware Ver. %x \nQT602240 Firmware Build. %x\n", g_version, g_build );

    return sprintf(buf, "0X%x", g_version );
}

static ssize_t firmware_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	char *after;
	int firmware_ret_val = -1;

	unsigned long value = simple_strtoul(buf, &after, 10);	
	if(debug_print_on)
        	printk(KERN_INFO "[TSP] %s\n", __FUNCTION__);

	if ( value == 1 )	// auto update.
	{
	        if(debug_print_on)
	        {
        		printk("[TSP] Firmware update start!!\n" );
        		printk("[TSP] version = 0x%x\n", g_version );
                }
//		if( g_version <= 22 ) 
		if(((g_version != 0x14)&&(g_version <0x16))||((g_version==0x16)&&(g_build==0xaa)))
		{			
			wake_lock(&tsp_firmware_wake_lock);
			firmware_ret_val = QT_Boot(qt60224_notfound_flag);
                        if(firmware_ret_val == 0)
			        qt60224_notfound_flag = 1;
                        else if(firmware_ret_val == 1)
			        qt60224_notfound_flag = 0;
			wake_unlock(&tsp_firmware_wake_lock);			
		}	
		else
		{
			firmware_ret_val = 1; 
		}
		if(debug_print_on)
        		printk("[TSP] Firmware result = %d\n", firmware_ret_val );

	}
  
  return size;  
}

#ifdef 	CONFIG_MACH_TICKERTAPE
u16 read_touch_keycode(u16 x, u16 y)
{
  if((y > KEY_START_Y) && (y <= KEY_END_Y))
  {
    if(x <= SEARCH_START_X)
      return TOUCH_MENU;
    else if(x <= HOME_START_X)
      return TOUCH_SEARCH;
    else if(x <= BACK_START_X)
      return TOUCH_HOME;
    else
      return TOUCH_BACK;
  }
  else
    return 0;
}
#endif //jihyon82.kim add touch key

void initialize_multi_touch(void)
{
        id2.x = 0;
        id2.y = 0;
        id2.press = 0;

        id3.x = 0;
        id3.y = 0;
        id3.press = 0;

        tmp2.x = 0;
        tmp2.y = 0;
        tmp2.press = 0;

        tmp3.x = 0;
        tmp3.y = 0;
        tmp3.press = 0;
        
}

void handle_keyarray_of_archer(uint8_t * atmel_msg)
{
        if(debug_print_on)
        	printk("atmel_msg[2] = 0x%x   ", atmel_msg[2]);
	if( (atmel_msg[2] & 0x1) && (menu_button==0) ) // menu press
	{
		menu_button = 1;
                if(debug_print_on)		
		        printk("menu_button is pressed\n");
		input_report_key(tsp.inputdevice, 139, DEFAULT_PRESSURE_DOWN);
        	input_sync(tsp.inputdevice);    		
	}
	else if( (atmel_msg[2] & 0x2) && (back_button==0) ) // back press
	{
		back_button = 1;      
		if(debug_print_on)
        		printk("back_button is pressed\n");                
		input_report_key(tsp.inputdevice, 158, DEFAULT_PRESSURE_DOWN);                
        	input_sync(tsp.inputdevice);    				
	}
	else if( (~atmel_msg[2] & (0x1)) && menu_button==1 ) // menu_release
	{
#ifdef FEATURE_CALIBRATION_BY_KEY
                if( first_key_after_probe_or_wake_up )
                {
                        if(debug_print_on)
                                printk("[TSP] calibratiton by first key after waking-up or probing\n");
                        first_key_after_probe_or_wake_up = 0;
                        calibrate_chip();
                }
#endif		
		menu_button = 0;  
		if(debug_print_on)
        		printk("menu_button is released\n");                                
		input_report_key(tsp.inputdevice, 139, DEFAULT_PRESSURE_UP);     
        	input_sync(tsp.inputdevice);    				
	}
	else if( (~atmel_msg[2] & (0x2)) && back_button==1 ) // menu_release
	{
#ifdef FEATURE_CALIBRATION_BY_KEY
                if( first_key_after_probe_or_wake_up )
                {
                        if(debug_print_on)
                                printk("[TSP] calibratiton by first key after waking-up or probing\n");
                        first_key_after_probe_or_wake_up = 0;
                        calibrate_chip();
                }
#endif		
		back_button = 0;    
		if(debug_print_on)
        		printk("back_button is released\n");                                
		input_report_key(tsp.inputdevice, 158, DEFAULT_PRESSURE_UP); 
        	input_sync(tsp.inputdevice);    				
	}
	else
	{
		menu_button=0; 
		back_button=0;
		if(debug_print_on)
        		printk("Unknow state of touch key\n");
	}

}

void handle_keyarray(uint8_t * atmel_msg)
{

    switch(g_model)
    {
        case ARCHER_KOR_SK:
        {
            handle_keyarray_of_archer(atmel_msg);
        }
        break;
        default:
        {
                if(debug_print_on)
                        printk("[TSP][ERROR] atmel message of key_array was not handled normally\n");
        }
    }
}
#ifdef FEATURE_MOVE_LIMIT
#define MOVE_LIMIT_SQUARE (150*150) // 100*100
#define DISTANCE_SQUARE(X1, Y1, X2, Y2)    (((X2-X1)*(X2-X1))+((Y2-Y1)*(Y2-Y1)))

int pre_x, pre_y, pre_size;

#endif

#ifdef FEATURE_VERIFY_INCORRECT_PRESS 
static u16 new_detect_id=0;
static u16 new_detect_x=0;

static u16 fixed_id2_x = 0;
static u16 fixed_id3_x = 0;

static u16 fixed_id2_count = 0;
static u16 fixed_id3_count = 0;
#endif

////ryun 20100208 add
void handle_multi_touch(uint8_t *atmel_msg)
{
	u16 x=0, y=0;
	unsigned int size ;	// ryun 20100113 
	static int check_flag=0; // ryun 20100113 	
	uint8_t touch_message_flag = 0;// ryun 20100208
        unsigned char one_touch_input_flag=0;
        
    
    x = atmel_msg[2];
    x = x << 2;
    x = x | (atmel_msg[4] >> 6);

    y = atmel_msg[3];
    y = y << 2;
    y = y | ((atmel_msg[4] & 0x6)  >> 2);

    size = atmel_msg[5];

//        printk("[TSP] msg[0] = %d, msg[1] = 0x%x  \n ", atmel_msg[0], atmel_msg[1] );	
	/* for ID=2 & 3, these are valid inputs. */
	if ((atmel_msg[0]==2)||(atmel_msg[0]==3))
	{
        if( atmel_msg[1] & 0x60) 
        {
            if(debug_print_on)	        
                printk("[TSP] msg0:0x%x", atmel_msg[1]);
        }
		/* for Single Touch input */
		if((id2.press+id3.press)!= 2)
		{
			/* case.1 - 11000000 -> DETECT & PRESS */
			if( ( atmel_msg[1] & 0xC0 ) == 0xC0  ) 
			{
				touch_message_flag = 1;// ryun 20100208
				
				if (atmel_msg[0] % 2)
					id3.press = 1;   // for ID=3
				else
					id2.press = 1;	 // for ID=2

				if ( (id2.press + id3.press) == 2 )
				{
					if ( atmel_msg[0] % 2)
					{
						REPORT( 1, size, tmp2.x, tmp2.y );
					}  // for ID=3
					else 
					{
						REPORT( 1, size, tmp3.x, tmp3.y );
					}  // for ID=2
					REPORT( 1, size, x, y );
                                        #ifdef FEATURE_VERIFY_INCORRECT_PRESS 
                                        new_detect_id = atmel_msg[0];
                                        new_detect_x = x;
                                        #endif
//                                        if(debug_print_on)
// don't print according to google recommendation        					printk("[TSP] detect one with new detect id=%d, x=%d, y=%d\n", atmel_msg[0] , x, y );
					
					if (atmel_msg[0] % 2)
					{
						id2.x = tmp2.x;	// for ID=3
						id2.y = tmp2.y;
						id3.x = x;
						id3.y = y;
					}
					else
					{
						id2.x = x; 		// for ID=2
						id2.y = y;
						id3.x = tmp3.x;
						id3.y = tmp3.y;
					}
				}
				else
				{
				        one_touch_input_flag = 1;
					REPORT( 1, size, x, y );
//					if(debug_print_on)
// don't print according to google recommendation        					printk("[TSP] one detect - id=%d, x=%d, y=%d\n", atmel_msg[0] , x, y );
				}
			}
			/* case.2 - case 10010000 -> DETECT & MOVE */
#if 0 //def	FEATURE_MOVE_LIMIT
                        // approve touch event as a move
			if( ( atmel_msg[1] & 0x90 ) == 0x90)
			{
			        if(  DISTANCE_SQUARE(pre_x, pre_y, x, y) < MOVE_LIMIT_SQUARE )
			        {
        				REPORT( 1, size , x, y);			        
			        }
			        else
			        {
        			        printk("filter incorrect move event px:%d, py:%d, x:%d, y:%d\n", pre_x, pre_y, x, y);
        			        REPORT( 0, pre_size, pre_x, pre_y);
        			        input_sync(tsp.inputdevice);
        			        REPORT( 1, size , x, y);
			        }

			}
#else			
			if( ( atmel_msg[1] & 0x90 ) == 0x90 )
			{
				REPORT( 1, size , x, y);
			}
#endif
			/* case.3 - case 00100000 -> RELEASE */
			else if( ((atmel_msg[1] & 0x20 ) == 0x20))   
			{
				REPORT( 0, 1, x, y );
//				if(debug_print_on)
// don't print according to google recommendation        				printk("[TSP] id=%d, x=%d, y=%d - release\n", atmel_msg[0] , x, y );

				if (atmel_msg[0] % 2)
					id3.press = 0; 	// for ID=3
				else
					id2.press = 0;	// for ID=2


				if((id3.press == 0) && (id2.press == 0))
				{
				}
                                #ifdef FEATURE_VERIFY_INCORRECT_PRESS 
                                init_acc_count(atmel_msg[0]);				
		                #endif		
			}

			input_sync(tsp.inputdevice);

			if (atmel_msg[0] % 2)
			{
				tmp3.x = x; 	// for ID=3
				tmp3.y = y;
			}
			else
			{
				tmp2.x = x;		// for ID=2
				tmp2.y = y;
			}
		}
		/* for Two Touch input */
		else if( id2.press && id3.press )
		{
			if ( atmel_msg[0] % 2 )
			{
				id3.x = x; // for x, ID=3
				id3.y = y; // for y
			}
			else
			{
				id2.x = x; // for x, ID=2
				id2.y = y; // for y
			}

			if( ((atmel_msg[1] & 0x20 ) == 0x20))   // release
			{
			
				if (atmel_msg[0] % 2)
					id3.press = 0;	// for ID=3
				else
					id2.press = 0;	// for ID=2

//				if(debug_print_on)	
// don't print according to google recommendation        				printk("[TSP] two input & one release\tid=2, x=%d, y=%d, id=3, x=%d, y=%d release id=%d\n", id2.x, id2.y ,id3.x, id3.y,  atmel_msg[0] );
				

                                #ifdef FEATURE_VERIFY_INCORRECT_PRESS
                                // check error and calibrate chip if there is a error.
                                {
                                        int count_id, count_x;

                                        // when a ID release event is reported
                                        // another ID is checked if it is fixed by noise.
        				if (atmel_msg[0] % 2)
        				{
        					count_id = 2;	
        					count_x = id2.x;
        				}	
        				else
        				{
        					count_id = 3;	
        					count_x = id3.x;
        				}	

//                                        printk("[TSP] id to be figureed is %d, x:%d\n", count_id, count_x);

                                        if( new_detect_id == atmel_msg[0] && new_detect_x != x)
                                        {
                                                if( figure_x_count(count_id, count_x) )
                                                {
                                                        extern uint8_t calibrate_chip(void);
                                                        calibrate_chip();
                                                        initialize_multi_touch();
                                                }
                                        }
                                        else // initialize accumulated count
                                        {
                                              init_acc_count(count_id);
                                        }
                                        init_acc_count(atmel_msg[0]);
                                }
                                #endif
				
			}	

			if ( id2.y && id3.y ) // report
			{
				REPORT( id2.press, size, id2.x, id2.y );
				REPORT( id3.press, size, id3.x, id3.y );

				input_sync(tsp.inputdevice);
				//				printk("[TSP] report two real value report ID=%d\n\tid=2, x=%d, y=%d , p=%d, id=3, x=%d, y=%d, p=%d\n", atmel_msg[0] , id2.x, id2.y, id2.press ,id3.x, id3.y, id3.press );

				id2.y = 0;  // clear
				id3.y = 0;
			}
			else if ((atmel_msg[0]==3)  // report one real value and one dummy value
					|| (atmel_msg[0]==2 && check_flag) // id2 is reported twice
					|| ((atmel_msg[1] & 0x20) == 0x20 )) // any one is released
			{
				if( id3.y == 0 )
				{
					REPORT( id2.press , size , id2.x, id2.y);
					REPORT( id3.press , size , tmp3.x, tmp3.y);
					//					printk("[TSP] report one and dummy %d report ID=%d\n\tid=2, x=%d, y=%d, p=%d, id=3, x=%d, y=%d, p=%d\n",__LINE__,atmel_msg[0], id2.x, id2.y, id2.press, tmp3.x, tmp3.y, id3.press );
					//					printk("\tid=2, tmp.x=%d, tmp.y=%d, id=3, tmp.x=%d, tmp.y=%d\n", tmp2.x, tmp2.y, tmp3.x, tmp3.y );
					input_sync(tsp.inputdevice);
					id2.y = 0;  // clear
				}
				else if( id2.y == 0 )
				{
					REPORT( id2.press , size , tmp2.x, tmp2.y);
					REPORT( id3.press , size , id3.x, id3.y);
					//					printk("[TSP] report one and dummy %d report ID=%d\n\tid=2, x=%d, y=%d, p=%d, id=3, x=%d, y=%d, p=%d\n",__LINE__,atmel_msg[0], id2.x, id2.y, id2.press, tmp3.x, tmp3.y, id3.press );
        				input_sync(tsp.inputdevice);
					id3.y = 0; // clear
        			}
			}
			else if ( atmel_msg[0] == 2 ) // for ID=2
			{
				check_flag=1;
			}

			if ( atmel_msg[0] % 2 ) 
			{
				tmp3.x = x;	 	// for ID=3
				tmp3.y = y;	
				check_flag=0; 
			}
			else
			{
				tmp2.x = x;	   	// for ID=2
				tmp2.y = y;
			}
		}
//		touch_flag = 1;	// ryun 20100113 
	}
	/* case.4 - Palm Touch & Unknow sate */
	else if ( atmel_msg[0] == 14 )
	{
		if((atmel_msg[1]&0x01) == 0x00)   
		{
		        if(debug_print_on)
        			printk("[TSP] Palm Touch! - %d <released from palm touch>\n", atmel_msg[1]);	
//			printk("[TSP] Firmware Ver. %x\n", info_block->info_id.version );	

			id2.press=0;  // touch history clear
			id3.press=0;

		}
		else
		{
		        if(debug_print_on)
        			printk("[TSP] test Palm Touch! - %d <face suppresstion status bit>\n", atmel_msg[1] );	
			touch_message_flag = 1;// ryun 20100208
		}
	}	
	else if ( atmel_msg[0] == 1 )
	{

                if(debug_print_on)
        		printk("[TSP] msg[0] = %d, msg[1] = 0x%x\n", atmel_msg[0], atmel_msg[1] );
#if 0	
                if((atmel_msg[1]&0x10) == 0x10) //hugh 0311
                {
                        cal_check_flag = 1;
                }
                else if(((atmel_msg[1]&0x10) == 0x00)&&(cal_check_flag == 1))
                {
                        cal_check_flag = 2;
                }
#endif                
 	}
	else if ( atmel_msg[0] == 0 )
	{
	        if(debug_print_on)
                	printk("[TSP] Error : %d - What happen to TSP chip?\n", __LINE__ );

//		touch_hw_rst( );  // TOUCH HW RESET
//		TSP_set_for_ta_charging( config_set_mode );   // 0 for battery, 1 for TA, 2 for USB
	}

	if(touch_message_flag && (cal_check_flag))//hugh 0311
	{
		check_chip_calibration(one_touch_input_flag);
	}
}    


#ifdef FEATURE_2_MORE_MULTI_TOUCH
// function definition
void handle_2_more_touch(uint8_t *atmel_msg)
{
	unsigned long x, y;
	unsigned int press = 3;
	int i;	
	int btn_report=0;	
	static int nPrevID= -1;
	uint8_t id;
	int bChangeUpDn= 0;

	int touchCount=0;
	uint8_t touch_message_flag = 0;// ryun 20100208
	unsigned int size=0;	// ryun 20100113 
	
	size = atmel_msg[5];

	id= atmel_msg[0] - 2;

	x = atmel_msg[2];
	x = x << 2;
	x = x | atmel_msg[4] >> 6;

	y = atmel_msg[3];
	y = y << 2;
	y = y | ((atmel_msg[4] & 0x6)  >> 2);
/* for ID=2 ~ 11, these are valid inputs. */
	if (atmel_msg[0] >=2 && atmel_msg[0]<=11)
	{

	if ( (atmel_msg[1] & 0x80) && (atmel_msg[1] & 0x40) )	// Detect & Press
	{		
		fingerInfo[id].id= id;
		fingerInfo[id].status= 1;
		fingerInfo[id].x= (int16_t)x;
		fingerInfo[id].y= (int16_t)y;
			fingerInfo[id].size= (unsigned int)size;
		bChangeUpDn= 1;
			touch_message_flag = 1;
			printk("[TSP] Finger[%d] Down (%d,%d)!\n", id, fingerInfo[id].x, fingerInfo[id].y );
	}
	else if ( (atmel_msg[1] & 0x80) && (atmel_msg[1] & 0x10) )	// Detect & Move
	{
		fingerInfo[id].x= (int16_t)x;
		fingerInfo[id].y= (int16_t)y;
			fingerInfo[id].size= (unsigned int)size;
		//printk("##### Finger[%d] Move (%d,%d)!\n", id, fingerInfo[id].x, fingerInfo[id].y );
	}
	else if ( (atmel_msg[1] & 0x20 ) )	// Release
	{
		fingerInfo[id].status= 0;
		bChangeUpDn= 1;
			printk("[TSP] Finger[%d] Up (%d,%d)!\n", id, fingerInfo[id].x, fingerInfo[id].y );
			for ( i= 0; i<MAX_NUM_FINGER; ++i )
			{
				if (fingerInfo[id].status == 1)
					touchCount++;
				else 
					break;
			}
			if(touchCount==0)
			{
			}
			
	}
	else
	{
		press = 3;
		printk("Unknown state! " );
//		print_msg();
		printk("\n");
	}			
	}
	/* case. - Palm Touch & Unknow sate */
	else if ( atmel_msg[0] == 14 )
	{
		if((atmel_msg[1]&0x01) == 0x00)   
		{ 
			printk("[TSP] Palm Touch! - %d <released from palm touch>\n", atmel_msg[1]);	
//			printk("[TSP] Firmware Ver. %x\n", info_block->info_id.version );	

			// touch history clear
			for ( i= 0; i<MAX_NUM_FINGER; ++i )
			{
				fingerInfo[i].status = -1;
			}

		}
		else
		{
			printk("[TSP] test Palm Touch! - %d <face suppresstion status bit>\n", atmel_msg[1] );	
			touch_message_flag = 1;// ryun 20100208
		}
	}	
	else if ( atmel_msg[0] == 1 )
	{
		printk("[TSP] msg[0] = %d, msg[1] = %d\n", atmel_msg[0], atmel_msg[1] );	

	}
	else if ( atmel_msg[0] == 0 )
	{
	printk("[TSP] Error : %d - What happen to TSP chip?\n", __LINE__ );

//		touch_hw_rst( );  // TOUCH HW RESET
//		TSP_set_for_ta_charging( config_set_mode );   // 0 for battery, 1 for TA, 2 for USB
	}

	if(touch_message_flag && cal_check_flag)
	{
		check_chip_calibration();
	}

	if ( nPrevID >= id || bChangeUpDn )
	{
//		amplitude= 0;
		for ( i= 0; i<MAX_NUM_FINGER; ++i )
		{
			if ( fingerInfo[i].status == -1 ) continue;

			// fingerInfo[i].status  : 0 Release,  Press (Down or Move)
			REPORT(fingerInfo[i].status, fingerInfo[id].size, fingerInfo[i].x, fingerInfo[i].y);
/*
			input_report_abs(tsp.inputdevice, ABS_MT_POSITION_X, fingerInfo[i].x);
			input_report_abs(tsp.inputdevice, ABS_MT_POSITION_Y, fingerInfo[i].y);
			input_report_abs(tsp.inputdevice, ABS_MT_TOUCH_MAJOR, fingerInfo[i].status);//pressure[i]); // 0 Release,  Press (Down or Move)
			input_report_abs(tsp.inputdevice, ABS_MT_WIDTH_MAJOR, fingerInfo[i].id);		// Size  ID 
			input_mt_sync(tsp.inputdevice);
*/
//			amplitude++;
			//printk("[TSP]%s x=%3d, y=%3d\n",__FUNCTION__, fingerInfo[i].x, fingerInfo[i].y);
			//printk("ID[%d]= %s", fingerInfo[i].id, (fingerInfo[i].status == 0)?"Up ":"" );

			if ( fingerInfo[i].status == 0 ) fingerInfo[i].status= -1;
		}
		input_sync(tsp.inputdevice);
//		printk("\n");
//		printk("##### Multi-Touch Event[%d] Done!\n", amplitude );
	}
	nPrevID= id;
	
}
#endif
void read_func_for_only_single_touch(struct work_struct *work)
{
//	uint8_t ret_val = MESSAGE_READ_FAILED;
	u16 x=0, y=0;
	u16 x480, y800, press;
//	int status;
//	u8 family_id;
//	unsigned int count=0;
#ifdef 	CONFIG_MACH_TICKERTAPE
    u16 keycode = 0;
#endif //jihyon82.kim add touch key  
	//PRINT_FUNCTION_ENTER;
	struct touchscreen_t *ts = container_of(work,
					struct touchscreen_t, tsp_work);
	if(enable_touch_boost) //added for touchscreen boost,samsung customisation,enabled in init.rc
	{	
		if (timer_pending(&ts->opp_set_timer))
			del_timer(&ts->opp_set_timer);
		omap_pm_set_min_mpu_freq(ts->dev, VDD1_OPP5_FREQ);
		mod_timer(&ts->opp_set_timer, jiffies + (1000 * HZ) / 1000);
	}

//	while(get_touch_irq_gpio_value() == 0 && count < 1000)
	{
//		count++;
		/*
		status = read_mem(0, 1, (void *) &family_id);
	    if (status != READ_MEM_OK)
	    {
	        return(status);
	    }else
		{
			printk("[TSP] family_id = 0x%x\n\n", family_id);
		}
		*/

		g_i2c_debugging_enable = 0;
		if(read_mem(message_processor_address, max_message_length, atmel_msg) == READ_MEM_OK)
		{
			g_i2c_debugging_enable = 0;
			
			if(atmel_msg[0]<2 || atmel_msg[0]>11)
			{
				g_i2c_debugging_enable = 0;
				if(debug_print_on)
        				printk("[TSP][ERROR] %s() - read fail \n", __FUNCTION__);
				//enable_irq(tsp.irq);
				enable_irq(tsp.irq);
				return ; 
			}
			
            //printk(DCM_INP, "[TSP][REAL]x: 0x%02x, 0x%02x \n", atmel_msg[2], atmel_msg[3]);
			x = atmel_msg[2];
			x = x << 2;
			x = x | (atmel_msg[4] >> 6);

			y = atmel_msg[3];
			y = y << 2;
			y = y | ((atmel_msg[4] & 0x6)  >> 2);
            //printk(DCM_INP, "[TSP][BIT]x: %d, %d \n", x, y);

//			printk(DCM_INP, "[TSP][RAWDATA]  x=%d, y=%d \n", x, y);		// ryun 20091126 
/* ryun 20091126 
			x480 = (y * 480)/1024;
			y800 = (x * 800)/1024;

			x480 = 480-x480;
			y800 = 800-y800;
*/
#if 1		// ryun 20091126 RESOLUTION : 1024 * 600
//			x480 = (x*MAX_TOUCH_X_RESOLUTION)/1024;
//			y800 = MAX_TOUCH_Y_RESOLUTION - (y*MAX_TOUCH_Y_RESOLUTION)/1024;
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV == CONFIG_OSCAR_REV00_EMUL)
			x480 = MAX_TOUCH_X_RESOLUTION - x;
#else
			x480 = x;
#endif
#ifdef 	CONFIG_MACH_TICKERTAPE
#ifdef TICKERTAPE_SFN
            if(y <= KEY_START_Y)
              y800 = (MAX_TOUCH_Y_RESOLUTION*y)/KEY_START_Y;
            else if(y <= KEY_END_Y )
              y800 = y;
            else
              y800 = 1024; //temp value

              keycode = read_touch_keycode(x, y); //jihyon82.kim for position error fixed
#else /*TICKERTAPE_SFN*/
              y800 = y;

              keycode = read_touch_keycode(x480, y800); //jihyon82.kim for position error fixed
#endif
#else /*CONFIG_MACH_TICKERTAPE*///jihyon82.kim add touch key
			y800 = y;
#endif
#else	// ryun 20091126 RESOLUTION : 480*800
			x480 = (y*480)/1024;
			y800 = (x*800)/1024;
		
#endif
			if( ((atmel_msg[1] & 0x40) == 0x40  || (atmel_msg[1] & 0x10) == 0x10) && (atmel_msg[1] & 0x20) == 0)
				press = 1;
			else if( (atmel_msg[1] & 0x40) == 0 && (atmel_msg[1] & 0x20) == 0x20)
				press = 0;
			else
			{
				//press = 3;
				//printk("[TSP][WAR] unknow state : 0x%x\n", msg[1]);
				//enable_irq(tsp.irq);
				enable_irq(tsp.irq);
				return;
			}
			
			if(press == 1)
			{
#ifdef 	CONFIG_MACH_TICKERTAPE
                if(keycode)
                {
                  input_report_key(tsp.inputdevice, keycode, DEFAULT_PRESSURE_DOWN);
                  printk("[TSP][KEY][DOWN] \n");
                 }
#ifdef  TICKERTAPE_SFN
                else if(y800 == 1024)
                  printk("[TSP][KEY][TICKER] \n");
#endif                
                else
                {
                  input_report_abs(tsp.inputdevice, ABS_X, x480);
				  input_report_abs(tsp.inputdevice, ABS_Y, y800);

				  input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_DOWN);
				  input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_DOWN);
                }
#else //jihyon82.kim add touch key			    
				input_report_abs(tsp.inputdevice, ABS_X, x480);
				input_report_abs(tsp.inputdevice, ABS_Y, y800);
				input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_DOWN);
				input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_DOWN);
#endif				
				input_sync(tsp.inputdevice);
				if(en_touch_log)
					{
				printk("[TSP][DOWN] id=%d, x=%d, y=%d, press=%d \n",(int)atmel_msg[0], x480, y800, press);
						en_touch_log = 0;
					}
			}else if(press == 0)
			{
#ifdef 	CONFIG_MACH_TICKERTAPE
                if(keycode)
                  {
                  input_report_key(tsp.inputdevice, keycode, DEFAULT_PRESSURE_UP	);
                    printk("[TSP][KEY][TICKER] \n");
                  }
#ifdef  TICKERTAPE_SFN
                else if(y800 == 1024)
                  printk("[TSP][BLACK][TICKER] \n");
#endif                
                else
                {
                  input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP	);
				  input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
                }
#else //jihyon82.kim add touch key			
				input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP	);
				input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
#endif				
				input_sync(tsp.inputdevice);
				printk("[TSP][UP] id=%d, x=%d, y=%d, press=%d \n",(int)atmel_msg[0], x480, y800, press);
				en_touch_log = 1;
			}
	//		ret_val = MESSAGE_READ_OK;
		}else
		{
			g_i2c_debugging_enable = 0;
			printk("[TSP][ERROR] %s() - read fail \n", __FUNCTION__);
		}
	}
//	PRINT_FUNCTION_EXIT;
	//enable_irq(tsp.irq);
	enable_irq(tsp.irq);
	return;
}
void read_func_for_multi_touch(struct work_struct *work)
{
//	uint8_t ret_val = MESSAGE_READ_FAILED;
//	u16 x=0, y=0;
//	u16 x480, y800, press;
//	unsigned int size ;	// ryun 20100113 
//	static int check_flag=0; // ryun 20100113 
//	int status;
//	u8 family_id;
//	unsigned int count=0;

	uint8_t object_type, instance;
	
	


	//PRINT_FUNCTION_ENTER;
	struct touchscreen_t *ts = container_of(work,
					struct touchscreen_t, tsp_work);

	if(enable_touch_boost) //added for touchscreen boost,samsung customisation,enabled in init.rc
	{
		if (timer_pending(&ts->opp_set_timer))
			del_timer(&ts->opp_set_timer);
		omap_pm_set_min_mpu_freq(ts->dev, VDD1_OPP5_FREQ);
		mod_timer(&ts->opp_set_timer, jiffies + (1000 * HZ) / 1000);
	}


	g_i2c_debugging_enable = 0;
	if(read_mem(message_processor_address, max_message_length, atmel_msg) != READ_MEM_OK)
	{
		g_i2c_debugging_enable = 0;
		if(debug_print_on)
        		printk("[TSP][ERROR] %s() - read fail \n", __FUNCTION__);
	//	enable_irq(tsp.irq);
	enable_irq(tsp.irq);
		return ;
	}

	object_type = report_id_to_type(atmel_msg[0], &instance);


	switch(object_type)
	{
		case GEN_COMMANDPROCESSOR_T6:	
		case TOUCH_MULTITOUCHSCREEN_T9:
		case PROCI_GRIPFACESUPPRESSION_T20: 
#ifdef FEATURE_2_MORE_MULTI_TOUCH
                        handle_2_more_touch(atmel_msg);        
#else
			handle_multi_touch(atmel_msg);
#endif			
			break;
		case TOUCH_KEYARRAY_T15:
        handle_keyarray(atmel_msg);
			break;
		// kmj_de31
		case PROCG_NOISESUPPRESSION_T22:
		{
                        extern void noise_detected(void);
		        extern unsigned char median_on;
		        extern unsigned char is_ta_on(void);

                        printk("[TSP]   PROCG_NOISESUPPRESSION_T22 detected\n");
                        if( is_ta_on() && (atmel_msg[1] & 0x1) )
                        {
                                printk("[TSP] FREQ CHANGE detected\n");
                                noise_detected();
                        }
//		        break;
		}        
		case SPT_GPIOPWM_T19:
		case PROCI_ONETOUCHGESTUREPROCESSOR_T24:
		case PROCI_TWOTOUCHGESTUREPROCESSOR_T27:
		case SPT_SELFTEST_T25:
		case SPT_CTECONFIG_T28:
		default:
		        if(debug_print_on)
        			printk("[TSP] 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x\n", atmel_msg[0], atmel_msg[1],atmel_msg[2], 
				    atmel_msg[3], atmel_msg[4],atmel_msg[5], atmel_msg[6], atmel_msg[7], atmel_msg[8]);
			break;
	};

	//if( atmel_msg[0] == 12 || atmel_msg[0] == 13)
	//{
       // handle_keyarray(atmel_msg);
	//}

	g_i2c_debugging_enable = 0;

      //if( atmel_msg[0] >=2 && atmel_msg[0] <=11 )
      //{
      //    handle_multi_touch(atmel_msg);
      //}    

	//enable_irq(tsp.irq);
	enable_irq(tsp.irq);

	return;
}



void atmel_touchscreen_read(struct work_struct *work)
{
    atmel_handler_functions[g_model](work);
}

void clear_touch_int()
{
		if(read_mem(message_processor_address, max_message_length, atmel_msg) == READ_MEM_OK)
		{
			{
#if 0
        			printk("[TSP][ERROR] %s() - read fail \n", __FUNCTION__);
				return ; 
#endif
			}
			
            printk( "[TSP][REAL]x: 0x%02x, 0x%02x \n", atmel_msg[2], atmel_msg[3]);
}
}

static irqreturn_t touchscreen_handler(int irq, void *dev_id)
{
//	down_interruptible(&sem_touch_handler);
//	if(g_enable_touchscreen_handler == 0)
//		return IRQ_HANDLED;
//	up(&sem_touch_handler);

//	cancel_delayed_work(&tsp_up_work);	 
	//schedule_work(&tsp_work);
//	printk("kranti:disable_irq\n");
//	disable_irq(tsp.irq);
	disable_irq_nosync(irq);
//	clear_touch_int();
//	printk("kranti:disable_irq2\n");
	queue_work(tsp_wq, &tsp.tsp_work);
//	printk("kranti:after queue work\n");
//	schedule_delayed_work(&tsp_up_work, msecs_to_jiffies(30));


//	printk("[TSP] %s() - end\n", __FUNCTION__);
	return IRQ_HANDLED;
}

extern int atmel_touch_probe(void); 
//extern void atmel_touchscreen_read(struct work_struct *work); 

int  enable_irq_handler(void)
{
	if (tsp.irq != -1)
	{
		tsp.irq = OMAP_GPIO_IRQ(OMAP_GPIO_TSP_INT);	// ryun .. move to board-xxx.c
		tsp.irq_type = IRQF_TRIGGER_LOW; 

		if (request_irq(tsp.irq, touchscreen_handler, tsp.irq_type, TOUCHSCREEN_NAME, &tsp))	
		{
		        if(debug_print_on)
        			printk("[TS][ERR] Could not allocate touchscreen IRQ!\n");
			tsp.irq = -1;
			input_free_device(tsp.inputdevice);
			return -EINVAL;
		}
		else
		{
                        if(debug_print_on)		
        			printk("[TS] registor touchscreen IRQ!\n");
		}

//		down_interruptible(&sem_touch_handler);
		if(g_enable_touchscreen_handler == 0)
			g_enable_touchscreen_handler = 1;
//		up(&sem_touch_handler);

		tsp.irq_enabled = 1;
	}
	return 0;
}

static int touchscreen_probe(struct platform_device *pdev)
{
	int ret;
	int error = -1;
//	u8 data[2] = {0,};
	struct kobject *ts_kobj;

        if(debug_print_on)
        	printk("[TSP] touchscreen_probe !! \n");
	set_touch_irq_gpio_disable();	// ryun 20091203


	if(IS_ATMEL) {
	        if(debug_print_on)
        		printk("[TS] atmel touch driver!! \n");
	} else {
	        if(debug_print_on)
        		printk("[TS][ERROR] unknown touch driver!! \n");
	}
	
	memset(&tsp, 0, sizeof(tsp));

	tsp.dev = &pdev->dev;
	
	tsp.inputdevice = input_allocate_device();

	if (!tsp.inputdevice)
	{
	        if(debug_print_on)
        		printk("[TS][ERR] input_allocate_device fail \n");
		return -ENOMEM;
	}

	spin_lock_init(&tsp.lock);

	/* request irq */
	if (tsp.irq != -1)
	{
		tsp.irq = OMAP_GPIO_IRQ(OMAP_GPIO_TSP_INT);
		tsp.irq_type = IRQF_TRIGGER_LOW; 
		tsp.irq_enabled = 1;
	}

    // default and common settings
	tsp.inputdevice->name = TOUCHSCREEN_NAME;
	tsp.inputdevice->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) | BIT_MASK(EV_SYN);	// ryun
	tsp.inputdevice->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);		// ryun 20091127 
	tsp.inputdevice->id.bustype = BUS_I2C;
	tsp.inputdevice->id.vendor  = 0;
	tsp.inputdevice->id.product =0;
 	tsp.inputdevice->id.version =0;
	
    // model specific settings
    switch(g_model) 
    {
        case ARCHER_KOR_SK:
        {
          tsp.inputdevice->keybit[BIT_WORD(TOUCH_MENU)] |= BIT_MASK(TOUCH_MENU);
          tsp.inputdevice->keybit[BIT_WORD(TOUCH_BACK)] |= BIT_MASK(TOUCH_BACK);
          tsp.inputdevice->keycode = atmel_ts_tk_keycode;       
        	input_set_abs_params(tsp.inputdevice, ABS_MT_POSITION_X, 0, MAX_TOUCH_X_RESOLUTION, 0, 0);
        	input_set_abs_params(tsp.inputdevice, ABS_MT_POSITION_Y, 0, MAX_TOUCH_Y_RESOLUTION, 0, 0);
        	input_set_abs_params(tsp.inputdevice, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
        	input_set_abs_params(tsp.inputdevice, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);            
        }
        break;
        case TICKER_TAPE:
        {
          tsp.inputdevice->keybit[BIT_WORD(TOUCH_MENU)] |= BIT_MASK(TOUCH_MENU);
          tsp.inputdevice->keybit[BIT_WORD(TOUCH_SEARCH)] |= BIT_MASK(TOUCH_SEARCH);
          tsp.inputdevice->keybit[BIT_WORD(TOUCH_HOME)] |= BIT_MASK(TOUCH_HOME);
          tsp.inputdevice->keybit[BIT_WORD(TOUCH_BACK)] |= BIT_MASK(TOUCH_BACK);
          tsp.inputdevice->keycode = atmel_ts_tk_keycode;
        	input_set_abs_params(tsp.inputdevice, ABS_X, 0, MAX_TOUCH_X_RESOLUTION, 0, 0);
        	input_set_abs_params(tsp.inputdevice, ABS_Y, 0, MAX_TOUCH_Y_RESOLUTION, 0, 0);
        	input_set_abs_params(tsp.inputdevice, ABS_PRESSURE, 0, 1, 0, 0);
            
        }
        break;
        case OSCAR:
        {
        	input_set_abs_params(tsp.inputdevice, ABS_X, 0, MAX_TOUCH_X_RESOLUTION, 0, 0);
        	input_set_abs_params(tsp.inputdevice, ABS_Y, 0, MAX_TOUCH_Y_RESOLUTION, 0, 0);
        	input_set_abs_params(tsp.inputdevice, ABS_PRESSURE, 0, 1, 0, 0);
        }
        break;

        default:
          break;
    }        

	ret = input_register_device(tsp.inputdevice);
	if (ret) {
	        if(debug_print_on)
                	printk(KERN_ERR "atmel_ts_probe: Unable to register %s \
			input device\n", tsp.inputdevice->name);
	}

	tsp_wq = create_singlethread_workqueue("tsp_wq");
#ifdef __CONFIG_ATMEL__
	INIT_WORK(&tsp.tsp_work, atmel_touchscreen_read);
#endif

#ifdef TOUCH_PROC
	touch_proc = create_proc_entry("touch_proc", S_IFREG | S_IRUGO | S_IWUGO, 0);
	if (touch_proc)
	{
		touch_proc->proc_fops = &touch_proc_fops;
		if(debug_print_on)
        		printk(" succeeded in initializing touch proc file\n");
	}
	else
	{
	        if(debug_print_on)
        	        printk(" error occured in initializing touch proc file\n");
	}
#endif

#ifdef __CONFIG_ATMEL__	
        if( atmel_touch_probe() < 0 )
        {
                return ENXIO;
        }
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	tsp.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	tsp.early_suspend.suspend = atmel_ts_early_suspend;
	tsp.early_suspend.resume = atmel_ts_late_resume;
	register_early_suspend(&tsp.early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

// [[ This will create the touchscreen sysfs entry under the /sys directory
	ts_kobj = kobject_create_and_add("touchscreen", NULL);
		if (!ts_kobj)
				return -ENOMEM;

	error = sysfs_create_file(ts_kobj,
				  &touch_boost_attr.attr);

	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}

	error = sysfs_create_file(ts_kobj,
				  &firmware_attr.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}

	/*------------------------------ for tunning ATmel - start ----------------------------*/
	error = sysfs_create_file(ts_kobj, &dev_attr_set_power.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}
	error = sysfs_create_file(ts_kobj, &dev_attr_set_acquisition.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}
	error = sysfs_create_file(ts_kobj, &dev_attr_set_touchscreen.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}
	error = sysfs_create_file(ts_kobj, &dev_attr_set_keyarray.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}
	error = sysfs_create_file(ts_kobj, &dev_attr_set_grip.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}
	error = sysfs_create_file(ts_kobj, &dev_attr_set_noise.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}
	error = sysfs_create_file(ts_kobj, &dev_attr_set_total.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}
	error = sysfs_create_file(ts_kobj, &dev_attr_set_write.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}

        #if 1 
	error = sysfs_create_file(ts_kobj, &dev_attr_set_tsp_for_extended_indicator.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}
        #endif	
	// 20100325 / ryun / add for jump limit control 	
	error = sysfs_create_file(ts_kobj, &dev_attr_set_tsp_for_inputmethod.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}	

#ifdef FEATURE_TSP_FOR_TA
	error = sysfs_create_file(ts_kobj, &dev_attr_set_tsp_for_TA.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}	
#endif	

#ifdef FEATURE_DYNAMIC_SLEEP
	error = sysfs_create_file(ts_kobj, &dev_attr_set_sleep_way_of_TSP.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
		return error;
	}	
#endif

        // kmj_DE10
	error = sysfs_create_file(ts_kobj, &dev_attr_set_TSP_debug_on.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
	}	

	// kmj_DE31
	error = sysfs_create_file(ts_kobj, &dev_attr_set_test_value.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
	}	
        // kmj_df08
	error = sysfs_create_file(ts_kobj, &dev_attr_TSP_filter_status.attr);
	if (error) {
	        if(debug_print_on)
        		printk(KERN_ERR "sysfs_create_file failed: %d\n", error);
	}	


	/*------------------------------ for tunning ATmel - end ----------------------------*/
	

#if 0 
#ifdef TOUCH_PROC
	touch_proc = create_proc_entry("touch_proc", S_IFREG | S_IRUGO | S_IWUGO, 0);
	if (touch_proc)
	{
		touch_proc->proc_fops = &touch_proc_fops;
		printk(" succeeded in initializing touch proc file\n");
	}
	else
	{
	        printk(" error occured in initializing touch proc file\n");
	}
#endif
#endif


// ]] This will create the touchscreen sysfs entry under the /sys directory
  	init_timer(&tsp.opp_set_timer);
  	tsp.opp_set_timer.data = (unsigned long)&tsp; 
  	tsp.opp_set_timer.function = tsc_timer_out;

	INIT_WORK(&(tsp.constraint_wq), tsc_remove_constraint_handler);

        if(debug_print_on)
        	printk("[TSP] success probe() !\n");

	return 0;
}

#ifdef TOUCH_PROC
int touch_proc_ioctl(struct inode *p_node, struct file *p_file, unsigned int cmd, unsigned long arg)
{
        switch(cmd)
        {
                case TOUCH_GET_VERSION :
                {
                        char fv[10];

                        sprintf(fv,"0X%x", g_version);
                        if(copy_to_user((void*)arg, (const void*)fv, sizeof(fv)))
                          return -EFAULT;
                }
                break;

                case TOUCH_GET_T_KEY_STATE :
                {
                        int key_short = 0;

                        key_short = menu_button || back_button;
                        if(copy_to_user((void*)arg, (const void*)&key_short, sizeof(int)))
                          return -EFAULT;
                
                }
                break;
                case TOUCH_GET_SW_VERSION :
                {
                        if(copy_to_user((void*)arg, (const void*)fw_version, sizeof(fw_version)))
                          return -EFAULT;
                }
                break;                
        }
        return 0;
}
#endif
static int touchscreen_remove(struct platform_device *pdev)
{

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&tsp.early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	input_unregister_device(tsp.inputdevice);

	if (tsp.irq != -1)
	{
//		down_interruptible(&sem_touch_handler);
		if(g_enable_touchscreen_handler == 1)
		{
			free_irq(tsp.irq, &tsp);
			g_enable_touchscreen_handler = 0;
		}
//		up(&sem_touch_handler);
	}

	touchscreen_poweronoff(0);

	return 0;
}
/* ryun 20091125
void touchscreen_poweronoff(int onoff)
{
	if (onoff)
	{
#ifndef __CONFIG_ATMEL__
		touchscreen_poweron();
#endif
	}
	else
	{
#ifndef __CONFIG_ATMEL__
		touchscreen_poweroff();
#endif
	}
	
	return;
}
*/
static int touchscreen_poweronoff(int power_state)
{
	int ret  = 0;
#ifdef CONFIG_MACH_TICKERTAPE
        if(power_state)
        {
                if (ret != twl4030_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,0x09, TWL4030_VAUX2_DEDICATED))
                        return -EIO;
                if (ret != twl4030_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,0x20, TWL4030_VAUX2_DEV_GRP))
                        return -EIO;
        }
        else
        {
                if (ret != twl4030_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,0x00, TWL4030_VAUX2_DEDICATED))
                        return -EIO;
     }
#endif
	return ret;
}
//EXPORT_SYMBOL(touchscreen_poweronoff);	// ryun 20091125 !!! ???

extern int atmel_suspend(void);
extern int atmel_resume(void);

static int touchscreen_suspend(struct platform_device *pdev, pm_message_t state)
{
        if(debug_print_on)
        	printk("[TSP] touchscreen_suspend : touch power off\n");
	//disable_irq(tsp.irq);
	disable_irq_nosync(tsp.irq);
//	flush_workqueue(tsp_wq);
	atmel_suspend();
        initialize_multi_touch();	
	return 0;
}

static int touchscreen_resume(struct platform_device *pdev)
{
        if(debug_print_on)
        	printk("[TSP] touchscreen_resume start : touch power on\n");
        #ifdef FEATURE_VERIFY_INCORRECT_PRESS	
        new_detect_id=0;
        new_detect_x=0;
        fixed_id2_x = 0;
        fixed_id3_x = 0;
        fixed_id2_count = 0;
        fixed_id3_count = 0;
        
        init_acc_count(2);
        init_acc_count(3);
        #endif
	atmel_resume();
	initialize_multi_touch(); 
	//enable_irq(tsp.irq);
	enable_irq(tsp.irq);
        if(debug_print_on)	
        	printk("[TSP] touchscreen_resume end\n");
	return 0;
}

static void touchscreen_device_release(struct device *dev)
{
	/* Nothing */
}

static struct platform_driver touchscreen_driver = {
	.probe 		= touchscreen_probe,
	.remove 	= touchscreen_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND		// 20100113 ryun 
	.suspend 	= &touchscreen_suspend,
	.resume 	= &touchscreen_resume,
#endif	
	.driver = {
		.name	= TOUCHSCREEN_NAME,
	},
};

static struct platform_device touchscreen_device = {
	.name 		= TOUCHSCREEN_NAME,
	.id 		= -1,
	.dev = {
		.release 	= touchscreen_device_release,
	},
};

#ifdef CONFIG_HAS_EARLYSUSPEND
void atmel_ts_early_suspend(struct early_suspend *h)
{
//	melfas_ts_suspend(PMSG_SUSPEND);
	touchscreen_suspend(&touchscreen_device, PMSG_SUSPEND);
}

void atmel_ts_late_resume(struct early_suspend *h)
{
//	melfas_ts_resume();
	touchscreen_resume(&touchscreen_device);
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */

static int __init touchscreen_init(void)
{
	int ret;

#ifdef CONFIG_SAMSUNG_ARCHER_TARGET_SK
    g_model = ARCHER_KOR_SK;
#elif defined CONFIG_MACH_HALO
	g_model = ARCHER_KOR_SK;
#elif defined(CONFIG_MACH_OSCAR)
    g_model = OSCAR;
#elif defined(CONFIG_MACH_TICKERTAPE)
    g_model = TICKER_TAPE;
#else
    g_model = DEFAULT_MODEL;
#endif

#if defined(CONFIG_SAMSUNG_ARCHER_TARGET_SK) || defined(CONFIG_MACH_OSCAR) || defined(CONFIG_MACH_HALO)
        mdelay(10);

        if (gpio_is_valid(OMAP_GPIO_TSP_EN)) {
    //      if (gpio_request(OMAP_GPIO_TSP_EN, S3C_GPIO_LAVEL(OMAP_GPIO_TSP_EN))) // ryun 20091218 
            if (gpio_request(OMAP_GPIO_TSP_EN, "touch")<0) // ryun 20091218 
                if(debug_print_on)
                        printk(KERN_ERR "Filed to request OMAP_GPIO_TSP_EN!\n");
            gpio_direction_output(OMAP_GPIO_TSP_EN, 0);
        }
        
        gpio_set_value(OMAP_GPIO_TSP_EN, 1);  // TOUCH EN
        mdelay(10);
#endif        
	wake_lock_init(&tsp_firmware_wake_lock, WAKE_LOCK_SUSPEND, "TSP");

	ret = platform_device_register(&touchscreen_device);
	if (ret != 0)
		return -ENODEV;

	ret = platform_driver_register(&touchscreen_driver);
	if (ret != 0) {
		platform_device_unregister(&touchscreen_device);
		return -ENODEV;
	}

	return 0;
}

static void __exit touchscreen_exit(void)
{
	wake_lock_destroy(&tsp_firmware_wake_lock);
	platform_driver_unregister(&touchscreen_driver);
	platform_device_unregister(&touchscreen_device);
}

int touchscreen_get_tsp_int_num(void)
{
        return tsp.irq;
}


#ifdef FEATURE_CAL_BY_INCORRECT_PRESS
unsigned char is_pressed()
{
        return (id2.press == 1 || id3.press == 1);
}
#endif


#ifdef FEATURE_VERIFY_INCORRECT_PRESS
static int repeat_count_id2=0;
static int repeat_count_id3=0;

static int saved_x_id2 = 0;
static int saved_x_id3 = 0;

//function declaration
static unsigned char figure_x_count(int id, int x)
{

        if( id == 2 )
        {
                if( saved_x_id2 == x )
                {
                        repeat_count_id2++;
                        if(debug_print_on)                        
                                printk("[TSP]    x of id2:%d is repeated %d times\n",x, repeat_count_id2);
                }
                else
                {
                        saved_x_id2 = x;
                        repeat_count_id2 = 1;
                        if(debug_print_on)
                                printk("[TSP]    x of id2:%d is updated newly as %d times\n",x, repeat_count_id2);                        
                }
        }
       
        if( id == 3 )
        {
                if( saved_x_id3 == x )
                {
                        repeat_count_id3++;
                        if(debug_print_on)
                                printk("[TSP]    x of id3:%d is repeated %d times\n",x, repeat_count_id3);
                }
                else
                {
                        saved_x_id3 = x; 
                        repeat_count_id3 = 1;
                        if(debug_print_on)
                                printk("[TSP]    x of id3:%d is updated newly as %d times\n",x, repeat_count_id3);                                                
                }
        }

        if( repeat_count_id3 >= incorrect_repeat_count || repeat_count_id2 >= incorrect_repeat_count )
        {
                saved_x_id3 = 0;
                saved_x_id2 = 0;
                repeat_count_id3 = 0;
                repeat_count_id2 = 0;
                if(debug_print_on)
                        printk("[TSP]    incorrect press by noise is detected\n");

                return 1;
        }

        return 0;
}

static init_acc_count(int id)
{
        if(debug_print_on)
                printk("[TSP] ID%d's variable is initialized\n", id);

        if( id == 2 )
        {
                repeat_count_id2 = 0;
                saved_x_id2 = 0;
        }

        if( id == 3 )
        {
                repeat_count_id3 = 0;
                saved_x_id3 = 0;
        }
}

void check_whether_x_is_fixed(void)
{

        if( id2.press )
        {
                if( fixed_id2_x != 0 && tmp2.x==fixed_id2_x )
                {
                        fixed_id2_count++;  
                        if(debug_print_on)
                                printk("[TSP]     id2.x:%d, fixed_id2_count increased to %d\n",tmp2.x, fixed_id2_count );
                        

                }
                else
                {
                        fixed_id2_count=1;
                        fixed_id2_x = tmp2.x;
                        if(debug_print_on)
                                printk("[TSP]     new fixed_id2_count:%d happened\n",fixed_id2_x );
                }
        }
        else
        {
                if(debug_print_on)
                        printk("[TSP]     fixed information id2 is cleared to 0\n");   
                fixed_id2_count = 0;
                fixed_id2_x = 0;
        }


        if( id3.press )
        {
                if( fixed_id3_x != 0 &&  tmp3.x==fixed_id3_x )
                {
                        fixed_id3_count++;    
                        if(debug_print_on)
                                printk("[TSP]     id3.x:%d, fixed_id3_count increased to %d\n",tmp3.x, fixed_id3_count );

                }
                else
                {
                        fixed_id3_count=1;
                        fixed_id3_x = tmp3.x;
                        if(debug_print_on)
                                printk("[TSP]     new fixed_id3_count:%d happened\n",fixed_id3_x );
                }
        }
        else
        {
                if(debug_print_on)
                        printk("[TSP]     fixed information id3 is cleared to 0\n");   
                fixed_id3_count = 0;
                fixed_id3_x = 0;                
        }

        if(fixed_id3_count >= 3 || fixed_id2_count >= 3 )
        {
                fixed_id2_x = 0;
                fixed_id3_x = 0;
                fixed_id2_count = 0;
                fixed_id3_count = 0;

                if(debug_print_on)
                        printk("[TSP] do calibrate chip because of noise when home key is pressed\n");
                calibrate_chip();
        }
}

#endif
module_init(touchscreen_init);
module_exit(touchscreen_exit);

MODULE_LICENSE("GPL");
