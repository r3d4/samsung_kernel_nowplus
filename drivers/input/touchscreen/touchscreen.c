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
#include <linux/regulator/consumer.h>
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
//#include <asm/arch/resource.h>
#include "touchscreen.h"

#include <linux/i2c/twl.h>	// ryun 20091125 
#include <linux/earlysuspend.h>	// ryun 20200107 for early suspend
#include <plat/omap-pm.h>	// ryun 20200107 for touch boost

//************************************************************************************
// value defines
//************************************************************************************
#define TOUCHSCREEN_NAME			"touchscreen"
#define DEFAULT_PRESSURE_UP		0
#define DEFAULT_PRESSURE_DOWN		256

#define __TOUCH_DRIVER_MAJOR_VER__ 3
#define __TOUCH_DRIVER_MINOR_VER__ 1

#define MAX_TOUCH_X_RESOLUTION	480
#define MAX_TOUCH_Y_RESOLUTION	800

#ifdef __TSP_I2C_ERROR_RECOVERY__
#define MAX_HANDLER_COUNT		50000
#endif

#define SYNAPTICS_SLEEP_WAKEUP_ADDRESS		0x20

#define	TOUCHSCREEN_SLEEP_WAKEUP_RETRY_COUNTER		3

//************************************************************************************
// enum value value
//************************************************************************************

enum EnTouchSleepStatus 
{
	EN_TOUCH_SLEEP_MODE = 0,
	EN_TOUCH_WAKEUP_MODE = 1
}; 

enum EnTouchPowerStatus 
{
	EN_TOUCH_POWEROFF_MODE = 0,
	EN_TOUCH_POWERON_MODE = 1
}; 

#ifdef	__SYNAPTICS_ALWAYS_ACTIVE_MODE__
enum EnTouchDriveStatus 
{
	EN_TOUCH_USE_DOZE_MODE = 0,
	EN_TOUCH_USE_NOSLEEP_MODE = 1
}; 
#endif

//************************************************************************************
// global value
//************************************************************************************

//struct	semaphore	sem_touch_onoff;
//struct	semaphore	sem_sleep_onoff;
//struct	semaphore	sem_touch_handler;

static struct touchscreen_t tsp;
static struct work_struct tsp_work;
static struct workqueue_struct *tsp_wq;
static int g_touch_onoff_status = EN_TOUCH_POWEROFF_MODE;
static int g_sleep_onoff_status = EN_TOUCH_WAKEUP_MODE;
static int g_enable_touchscreen_handler = 0;	// fixed for i2c timeout error.
static unsigned int g_version_read_addr = 0;
//static unsigned short g_position_read_addr = 0;

extern int nowplus_enable_touch_pins(int enable);//me add
//struct res_handle *tsp_rhandle = NULL;
struct regulator *tsp_rhandle;//me add

#ifndef __CONFIG_CYPRESS_USE_RELEASE_BIT__
static struct timer_list tsp_timer;
#endif
struct touchscreen_t;

struct touchscreen_t {
	struct input_dev * inputdevice;
#ifndef __CONFIG_CYPRESS_USE_RELEASE_BIT__
	struct timer_list ts_timer;
#endif
	int touched;
	int irq;
	int irq_type;
	int irq_enabled;
	struct ts_device *dev;
	spinlock_t lock;
	struct early_suspend	early_suspend;// ryun 20200107 for early suspend
};

#ifdef CONFIG_HAS_EARLYSUSPEND
void touchscreen_early_suspend(struct early_suspend *h);
void touchscreen_late_resume(struct early_suspend *h);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

void (*touchscreen_read)(struct work_struct *work) = NULL;

static u16 syna_old_x_position = 0;
static u16 syna_old_y_position = 0;
static u16 syna_old_x2_position = 0;
static u16 syna_old_y2_position = 0;
static int syna_old_finger_type = 0;
static int finger_switched = 0;

#ifdef __TSP_I2C_ERROR_RECOVERY__
static struct timespec g_last_recovery_time;
static unsigned int g_i2c_error_recovery_count = 0;
static long g_touch_irq_handler_count = 0;
#endif

#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
static unsigned int g_synaptics_unstable_recovery_count = 0;
static unsigned int g_rf_recovery_behavior_status = 1;
static struct timer_list tsp_rf_noise_recovery_timer;
static struct timeval g_last_rf_noise_recovery_time;
#endif

u8 g_synaptics_read_addr = 0;
int g_synaptics_read_cnt = 0;

unsigned char g_pTouchFirmware[SYNAPTICS_FIRMWARE_IMAGE_SIZE];
unsigned int g_FirmwareImageSize = 0;

struct timeval g_current_timestamp;

u8 g_synaptics_device_control_addr = 0;

#ifdef __SYNA_MULTI_TOUCH_SUPPORT__
static int syna_old_old_finger_type=0;
#endif

//************************************************************************************
// extern values
//************************************************************************************
#if 0
// synaptics values
/extern struct omap3430_pin_config touch_i2c_pins_gpio_scl_input[1];
extern struct omap3430_pin_config touch_i2c_pins_gpio_scl_output[1];
extern struct omap3430_pin_config touch_i2c_pins_gpio_sda_input[1];
extern struct omap3430_pin_config touch_i2c_pins_gpio_sda_output[1];
extern struct omap3430_pin_config touch_i2c_pins_gpio_int_input[1];
extern struct omap3430_pin_config touch_i2c_pins_gpio_int_output[1];
#endif
//************************************************************************************
// extern functions
//************************************************************************************
extern int i2c_tsp_sensor_init(void);
extern int i2c_tsp_sensor_read(u8 reg, u8 *val, unsigned int len );
extern int i2c_tsp_sensor_write_reg(u8 address, int data);

//extern unsigned int SynaDoReflash();

//************************************************************************************
// function defines
//************************************************************************************
static irqreturn_t touchscreen_handler(int irq, void *dev_id);

void touchscreen_enter_sleep(void);
void touchscreen_wake_up(void);

void touchscreen_poweroff(void);
void touchscreen_poweron(void);
void set_touch_i2c_mode_init(void);
void synaptics_touchscreen_start_triggering(void);
void touch_ic_recovery();
#ifdef __SYNAPTICS_ALWAYS_ACTIVE_MODE__
static int synaptics_set_drive_mode_bit(enum EnTouchDriveStatus drive_status);
#endif


#if 0
//************************************************************************************
// function bodys
//************************************************************************************
static void synaptics_set_GPIO_direction_out()
{
	// direction output
	omap3430_pad_set_configs(touch_i2c_pins_gpio_sda_output, ARRAY_SIZE(touch_i2c_pins_gpio_sda_output));
	omap3430_pad_set_configs(touch_i2c_pins_gpio_scl_output, ARRAY_SIZE(touch_i2c_pins_gpio_scl_output));
	omap3430_pad_set_configs(touch_i2c_pins_gpio_int_output, ARRAY_SIZE(touch_i2c_pins_gpio_int_output));
	udelay(50);
	omap_set_gpio_direction(OMAP_GPIO_TOUCH_IRQ, GPIO_DIR_OUTPUT);
	omap_set_gpio_direction(OMAP_GPIO_TSP_SCL, GPIO_DIR_OUTPUT);
	omap_set_gpio_direction(OMAP_GPIO_TSP_SDA, GPIO_DIR_OUTPUT);
	udelay(50);
}

static void synaptics_set_touch_direction_in(void)
{
	omap3430_pad_set_configs(touch_i2c_pins_gpio_sda_input, ARRAY_SIZE(touch_i2c_pins_gpio_sda_input));
	omap3430_pad_set_configs(touch_i2c_pins_gpio_scl_input, ARRAY_SIZE(touch_i2c_pins_gpio_scl_input));
	omap3430_pad_set_configs(touch_i2c_pins_gpio_int_input, ARRAY_SIZE(touch_i2c_pins_gpio_int_input));
	udelay(50);
	omap_set_gpio_direction(OMAP_GPIO_TOUCH_IRQ, GPIO_DIR_INPUT);
	omap_set_gpio_direction(OMAP_GPIO_TSP_SCL, GPIO_DIR_INPUT);
	omap_set_gpio_direction(OMAP_GPIO_TSP_SDA, GPIO_DIR_INPUT);
	udelay(50);
}

#endif
// never call irq handler
void synaptics_touchscreen_start_triggering(void)
{
	u8 *data;
	int ret = 0;

	data = kmalloc(g_synaptics_read_cnt, GFP_KERNEL);
	if(data == NULL)
	{
		printk("[TSP][ERROR] %s() kmalloc fail\n", __FUNCTION__ );
		return;
	}
	ret = i2c_tsp_sensor_read(g_synaptics_read_addr, data, g_synaptics_read_cnt);
	if(ret != 0)
	{
		printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
	}
	kfree(data);

#ifdef __SYNAPTICS_ALWAYS_ACTIVE_MODE__
	synaptics_set_drive_mode_bit(EN_TOUCH_USE_NOSLEEP_MODE);
#endif

}

void synaptics_get_page_address(unsigned short *position_start_addr, unsigned short *firmware_revision_addr)
{
	u8 data;
	int i=0, ret=0;;
	unsigned short base_addr = 0xEE;
	int getcount=0;


	for(i=0 ; i<10 ; i++)
	{
		ret = i2c_tsp_sensor_read(base_addr, &data, 1);
		if(ret != 0)
		{
			printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
			return;
		}
		
		if(data == 0x0)
			udelay(1000);
		else
			break;
	}
	if(i == 10)
	{
		printk("[TSP][ERROR] i2c read fail\n");
		return;
	}
	
	for(i=0 ; i<10 ; i++)
	{
		ret = i2c_tsp_sensor_read(base_addr, &data, 1);
		if(ret != 0)
		{
			printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
			return;
		}
		
		if(data == 0x01)
		{
			(*firmware_revision_addr) = base_addr - 5;
			ret = i2c_tsp_sensor_read((*firmware_revision_addr), &data, 1);
			if(ret != 0)
			{
				printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
				return;
			}
			(*firmware_revision_addr) = data+3;
			base_addr -= 6;
			getcount++;
		}
		else if(data == 0x11)
		{
			(*position_start_addr) = base_addr - 2;

			base_addr -= 6;
			getcount++;
		}

		if(getcount == 2)
			break;
	}
}

static void issp_request_firmware(char* update_file_name)
{
	int idx_src = 0;
	int idx_dst = 0;
	int line_no = 0;
	int dummy_no = 0;
	char buf[2];
		
	struct device *dev = &tsp.inputdevice->dev;
	const struct firmware * fw_entry;

	request_firmware(&fw_entry, update_file_name, dev);
//	printk("[TSP][DEBUG] firmware size = %d\n", fw_entry->size);

	do {
		g_pTouchFirmware[idx_dst] = fw_entry->data[idx_src];
		idx_src++;
		idx_dst++;
	} while (idx_src < g_FirmwareImageSize-2);
	//end value '0xFFFF'
	g_pTouchFirmware[g_FirmwareImageSize-2]=0xFF;
	g_pTouchFirmware[g_FirmwareImageSize-1]=0xFF;
}

static int touch_firmware_update(char* firmware_file_name)
{
	printk("[TSP] firmware file name : %s\n", firmware_file_name);
	
	printk("[TSP] start update synaptics touch firmware !!\n");
	g_FirmwareImageSize = SYNAPTICS_FIRMWARE_IMAGE_SIZE;

	if(g_pTouchFirmware == NULL)
	{
		printk("[TSP][ERROR] %s() kmalloc fail !! \n", __FUNCTION__);
		return -1;
	}

	//down_interruptible(&sem_touch_handler);
	if(g_enable_touchscreen_handler == 1)
	{
		g_enable_touchscreen_handler = 0;
		free_irq(tsp.irq, &tsp);
	}
	//up(&sem_touch_handler);

	issp_request_firmware(firmware_file_name);

//	SynaDoReflash();
	
	g_FirmwareImageSize = 0;

	touchscreen_poweroff();
	touchscreen_poweron();

	synaptics_touchscreen_start_triggering();

	return 0;
}

static ssize_t sysfs_issp_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int fw_upgrade=0;
	u8 data[2] = {0, };
	char filename[50] = {'\0',};
	int ret=0;
#ifdef __CONFIG_FIRMWARE_AUTO_UPDATE__
	u8 new_firmware_version = 0;
	u8 current_firmware_version = 0;
	int i;

	new_firmware_version = 0x7;
#endif

	touchscreen_poweron();
	udelay(10);

	ret = sscanf(buf, "%d", &fw_upgrade);

//////////////////////////// default touch firmware update.
	if (fw_upgrade == 1)
	{
		ret = touch_firmware_update("touch_c.img");
	}
//////////////////////////// touch firmware update given file name.
	else if (fw_upgrade == 2)
	{
		sscanf(buf+ret, "%50s", filename);
		if(filename[0] == (char)'\0')
		{
			printk("[ERROR] please input update file name.\n");
			return count;
		}else
		{
			printk("\nupdate file name is \"%s\"\n", filename);
		}
		
		printk("start update touch firmware(%d) !!\n", fw_upgrade);
		ret = touch_firmware_update(filename);
	}
//////////////////////////// auto touch firmware update checked current version.
	else if (fw_upgrade == 3)
	{
#ifdef __CONFIG_FIRMWARE_AUTO_UPDATE__
		int	flag_firmware_update = 0;

		ret = i2c_tsp_sensor_read(g_version_read_addr, data, 2);
		if(ret != 0)
		{
			printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
			flag_firmware_update = 1;
			//return count; // have to firmware update, if touch ic s/w was broken
		}

		current_firmware_version = data[0];
		printk("firmware S/W revision info = %d\n", current_firmware_version);

		if(current_firmware_version == 0)
		{
			for(i=0; i<5; i++)
			{
				mdelay(400);
				ret = i2c_tsp_sensor_read(g_version_read_addr, data, 2);
				if(ret != 0)
				{
					printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
					flag_firmware_update = 1;
					//return count; // have to firmware update, if touch ic s/w was broken
				}
				current_firmware_version = data[0];
				if(current_firmware_version != 0)
					break;
			}

			if(current_firmware_version == 0)
			{
				printk("[TSP][ERROR] firmware version read fail \n");
				flag_firmware_update = 1;
			}else
			{
				flag_firmware_update = 0;
			}
		}

		if( (new_firmware_version > current_firmware_version) || (flag_firmware_update==1))
		{			

			ret = touch_firmware_update("touch_c.img");

			printk("[TSP] latest touch firmware installed : %d\n", new_firmware_version);

			//#ifdef	__SYNAPTICS_PRINT_SCREEN_FIRMWARE_UPDATE__	
				//display_dbg_msg("touch firmware update finished !", KHB_DEFAULT_FONT_SIZE, KHB_DISCARD_PREV_MSG);
			//#endif
			
		}
		else
			printk("[TSP] No excution touch firmware update. (%d, %d)\n", new_firmware_version, current_firmware_version);
#endif
	}
//////////////////////////// touch firmware update by mmc.
	else if (fw_upgrade == 4)
	{
		ret = touch_firmware_update("mmc_ptouch.img");
	}
//////////////////////////// Can't find matched command
	else
	{
		ret = -1;
		printk("[TSP][ERROR] wrong touch firmware update command\n");
	}

#ifdef CONFIG_LATE_HANDLER_ENABLE
	// irq handler registration !!!!!!!!!!!!!!!!
	touchscreen_poweron();
#endif

	if(ret < 0)
	{
		printk("[TSP][ERROR] %s() firmware update fail !!\n", __FUNCTION__);
	}else
	{
		printk("[TSP] %s() firmware update success !!\n", __FUNCTION__);
	}

	return count;
}

static ssize_t sysfs_issp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	u8 data[2] = {0,};
//	unsigned char	fIsError;

	ret = i2c_tsp_sensor_read(g_version_read_addr, data, 2);
	if(ret != 0)
	{
		printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
		return sprintf(buf, "touch driver version : i2c read fail\n");
	}

	printk("touch driver version : %d.%d, firmware S/W Revision Info = %d\n", 
		__TOUCH_DRIVER_MAJOR_VER__,__TOUCH_DRIVER_MINOR_VER__, data[0]);

	return sprintf(buf, "%d", data[0]);
}

static DEVICE_ATTR(issp, S_IRUGO | S_IWUSR, sysfs_issp_show, sysfs_issp_store);

#ifdef CONFIG_LATE_HANDLER_ENABLE
static ssize_t sysfs_tsp_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int touch_enable;
	sscanf(buf, "%d", &touch_enable);

	if(touch_enable == 0) {
		//down_interruptible(&sem_touch_handler);
		if(g_enable_touchscreen_handler == 0)
			printk("[TSP][WARNING] Touch handler already disabled.\n");
		else
		{
			g_enable_touchscreen_handler = 0;
			free_irq(tsp.irq, &tsp);
			printk("[TSP] Touch handler disable !!\n");
		}
		//up(&sem_touch_handler);
	}
	else if(touch_enable == 1) {
		//down_interruptible(&sem_touch_handler);
		if(g_enable_touchscreen_handler == 1)
			printk("[TSP][WARNING] Touch handler already enabled.\n");
		else
		{
			g_enable_touchscreen_handler = 1;
			if (request_irq(tsp.irq, touchscreen_handler, tsp.irq_type, TOUCHSCREEN_NAME, &tsp))	
			{
				printk("[TSP][ERROR] Could not allocate touchscreen IRQ!\n");
				tsp.irq = -1;
				input_free_device(tsp.inputdevice);
				return count;
			}
			printk("[TSP] Touch handler already enable !!\n");
		}
		//up(&sem_touch_handler);
	}
	else
		printk("[ERROR] wrong touch enable change command\n");
	return count;
}

static ssize_t sysfs_tsp_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("[TSP] touch handler enable : %d\n", g_enable_touchscreen_handler);
	return sprintf(buf, "%d", g_enable_touchscreen_handler);
}

static DEVICE_ATTR(tsp_enable, S_IRUGO | S_IWUSR, sysfs_tsp_enable_show, sysfs_tsp_enable_store);
#else
;//#error build-test
#endif // CONFIG_LATE_HANDLER_ENABLE

static ssize_t sysfs_touch_control_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int touch_control_flag, ret=0;
	unsigned char data[2] = {0,};
	
	sscanf(buf, "%d", &touch_control_flag);

	if( touch_control_flag == 0) {
		printk("[TSP] %s() enter sleep mode...\n", __FUNCTION__);
		touchscreen_enter_sleep();
	}
	else if( touch_control_flag == 1) {
		printk("[TSP] %s() wake-up mode...\n", __FUNCTION__);

		touchscreen_wake_up();
		
		ret = i2c_tsp_sensor_read(g_version_read_addr, data, 2);
		if(ret != 0)
		{
			printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
			return count;
		}
		//printk(DCM_INP, "[TSP] touch driver version : %d.%d, firmware S/W Revision Info = %x\n", __TOUCH_DRIVER_MAJOR_VER__,__TOUCH_DRIVER_MINOR_VER__, data[0]);

		synaptics_touchscreen_start_triggering();
	}
	else if( touch_control_flag == 2) {
		printk("[TSP] %s() power off mode...\n", __FUNCTION__);
		touchscreen_poweroff();
	}
	else if( touch_control_flag == 3) {
		printk("[TSP] %s() power on mode...\n", __FUNCTION__);
		touchscreen_poweron();
		ret = i2c_tsp_sensor_read(g_version_read_addr, data, 2);
		if(ret != 0)
		{
			printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
			touch_ic_recovery();
			return count;
		}
		printk( "[TSP] touch driver version : %d.%d, firmware S/W Revision Info = %x\n", 
			__TOUCH_DRIVER_MAJOR_VER__,__TOUCH_DRIVER_MINOR_VER__, data[0]);

		synaptics_touchscreen_start_triggering();
	}
	else
		printk("[TSP][ERROR] wrong touch control command\n");
	return count;
}

static ssize_t sysfs_touch_control_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static DEVICE_ATTR(touchcontrol, S_IRUGO | S_IWUSR, sysfs_touch_control_show, sysfs_touch_control_store);

static int find_next_string_in_buf(const char *buf, size_t count)
{
	int ii=0;
	for(ii=0 ; ii<count ; ii++)
	{
		if(buf[ii] == ' ')
			return ii;
	}
	return -1;
}

static ssize_t sysfs_get_register_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	u8 *pData;
	int start_address = 0, end_address = 0, ret = 0, cnt = 0;
	unsigned int read_size;

	if((ret = sscanf(buf, "%x", &start_address)) <= 0)
	{
		printk("[TSP][ERROR] %s() input start address and then end address !!\n", __FUNCTION__);
		return count;
	}
	
	cnt = find_next_string_in_buf(buf, count);
	if(cnt > 0)
	{
		ret = sscanf(buf+cnt, "%x", &end_address);
		if(ret <= 0 || end_address == 0 || end_address < start_address)
		{
			read_size = 1;
		}else
		{
			read_size = end_address - start_address + 1;
		}
	}else{
		read_size = 1;
	}
		
	
	pData = kmalloc(read_size, GFP_KERNEL);
	ret = i2c_tsp_sensor_read((u8)start_address, pData, read_size);
	if(ret != 0)
	{
		printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
		return count;
	}
		if(read_size == 1)
		printk("[TSP] address : 0x%x, value: 0x%x\n", start_address, pData[0]);
	else
	{
		printk("[TSP] read touch ic register - start address : 0x%x -- end address : 0x%x\n", start_address, end_address);
		printk("======================================================================\n");
		for(cnt=0 ; cnt < read_size ; cnt++)
		{
			printk("0x%02x ", pData[cnt]);
			if((cnt+1)%10 == 0 && cnt != 0)
				printk("\n");
		}
		printk("\n======================================================================\n");
	}
	kfree(pData);
	return count;
}

static ssize_t sysfs_get_register_show(struct device *dev, struct device_attribute *attr, char *buf)
{

	printk("[TSP] synaptics version address : 0x%x\n", g_version_read_addr);
	printk("[TSP] synaptics read address : 0x%x\n", g_synaptics_read_addr);
	printk("[TSP] USAGE : echo [start address] {end address} > get_register\n");

	return 0;
}

static DEVICE_ATTR(get_register, S_IRUGO | S_IWUSR, sysfs_get_register_show, sysfs_get_register_store);

static ssize_t sysfs_set_register_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int address = 0;
	int data = 0, ret = 0, cnt=0;

	if((ret = sscanf(buf, "%x", &address)) <= 0)
	{
		printk("[TSP][ERROR] %s() input start address and then end address !!\n", __FUNCTION__);
		return count;
	}

	cnt = find_next_string_in_buf(buf, count);
	if(cnt < 0)
	{
		printk("[TSP][ERROR] %s() input start address and then end address !!\n", __FUNCTION__);
		return count;
	}
	
	if((ret = sscanf(buf+cnt, "%x", &data)) <= 0)
	{
		printk("[TSP][ERROR] %s() input start address and then end address !!\n", __FUNCTION__);
		return count;
	}
	printk("[TSP] try to write : address 0x%x, write value : 0x%x\n", address, data);

	if((ret = i2c_tsp_sensor_write_reg((u8)address, data)) != 0)
	{
		printk("[TSP][ERROR] %s() i2c_tsp_sensor_write_reg error : %d\n", __FUNCTION__ , ret);
		return count;
	}

	data = 0;
	
	ret = i2c_tsp_sensor_read((u8)address, (u8*)&data, 1);
	if(ret != 0)
	{
		printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
		return count;
	}

	printk("[TSP] read again : address 0x%x, write value : 0x%x\n", address, data);

	return count;
}

static ssize_t sysfs_set_register_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("[TSP] USAGE : echo [address] [value] > set_register\n");
	return 0;
}

static DEVICE_ATTR(set_register, S_IRUGO | S_IWUSR, sysfs_set_register_show, sysfs_set_register_store);

#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
static ssize_t sysfs_rf_recovery_behavior_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int status;

	sscanf(buf, "%d", &status);

	if(status == 0) {
		g_rf_recovery_behavior_status = 0;
		printk("[TSP] synaptics rf recovery disable\n");
	}
	else if(status == 1) {
		g_rf_recovery_behavior_status = 1;
		printk("[TSP] synaptics rf recovery by rezero\n");
	}
	else if(status == 2) {
		g_rf_recovery_behavior_status = 2;
		printk("[TSP] synaptics rf recovery by reset\n");
	}
	else {
		printk("[TSP] wrong command of touch debug\n");
	}
	return count;
}

static ssize_t sysfs_rf_recovery_behavior_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("g_rf_recovery_behavior_status = %d\n", g_rf_recovery_behavior_status);
	return sprintf(buf, "%d\n", g_rf_recovery_behavior_status);
}

static DEVICE_ATTR(rf_recovery_behavior, S_IRUGO | S_IWUSR, sysfs_rf_recovery_behavior_show, sysfs_rf_recovery_behavior_store);
#endif

#ifndef __CONFIG_CYPRESS_USE_RELEASE_BIT__
static void tsp_timer_handler(unsigned long data)
{
	input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
	input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
	input_sync(tsp.inputdevice);

	//printk( "[TSP] timer : up\n");
}
#endif
#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__

static void tsp_rf_noise_recovery_timer_handler(unsigned long data)
{
	if(
		(g_last_rf_noise_recovery_time.tv_sec > g_current_timestamp.tv_sec) || 
		((g_last_rf_noise_recovery_time.tv_sec == g_current_timestamp.tv_sec) 
			&& (g_last_rf_noise_recovery_time.tv_usec  > g_current_timestamp.tv_usec))
	)
	{
		input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
		input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
		input_sync(tsp.inputdevice);
		printk("[TSP][WARNING] == recovery timer : up\n");
	}

	del_timer(&tsp_rf_noise_recovery_timer);
}

#endif

#ifdef __SYNAPTICS_ALWAYS_ACTIVE_MODE__
static int synaptics_set_drive_mode_bit(enum EnTouchDriveStatus drive_status)
{
	int data = 0, ret = 0;
	
	ret = i2c_tsp_sensor_read((u8)g_synaptics_device_control_addr, (u8*)&data, 1);
	if(ret != 0)
	{
		printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
		return -1;
	}

	printk( "%s() data before = 0x%x\n", __FUNCTION__, data);
	
	if(drive_status == EN_TOUCH_USE_DOZE_MODE) // clear bit
	{
		data &=	0xfb;	// and 1111 1011
	}else if(drive_status == EN_TOUCH_USE_NOSLEEP_MODE) // set 1
	{
		data |= 0x04;		// or 0000 0100
	}

	printk( "%s() data after = 0x%x\n", __FUNCTION__, data);	
	
	if(g_synaptics_device_control_addr != 0)
	{
		if((ret = i2c_tsp_sensor_write_reg(g_synaptics_device_control_addr, data)) != 0)
		{
			printk("[TSP][ERROR] %s() i2c_tsp_sensor_write_reg error : %d\n", __FUNCTION__ , ret);
			return -1;
		}
	}else{
		printk("[TSP][ERROR] %s() g_synaptics_device_control_addr is not setted\n", __FUNCTION__);
		return -1;
	}
	return 1;
}

#endif // __SYNAPTICS_ALWAYS_ACTIVE_MODE__


/**************************************************
//	Device Control register
//          7                6           5    4   3         2            1      0
// | Configured | Report Rate | - | - | - | No Sleep | Sleep Mode |
**************************************************/

static int synaptics_write_sleep_bit(enum EnTouchSleepStatus sleep_status)
{
	int data = 0, ret = 0;
	
	ret = i2c_tsp_sensor_read((u8)g_synaptics_device_control_addr, (u8*)&data, 1);
	if(ret != 0)
	{
		printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
		return -1;
	}

	printk( "%s() data before = 0x%x\n", __FUNCTION__, data);

#ifdef __SYNAPTICS_ALWAYS_ACTIVE_MODE__
	if(sleep_status == EN_TOUCH_SLEEP_MODE) // set 1
	{
		data &=	0xf8;	// and 1111 1000 - clear 0~2
		data |= 0x01;		// or 0000 0001
	}else if(sleep_status == EN_TOUCH_WAKEUP_MODE) // set 0
	{
		data &=	0xfc;	// and 1111 1100
		data |= 0x04;		// or 0000 0100
	}
#else
	if(sleep_status == EN_TOUCH_SLEEP_MODE) // set 1
	{
		data &=	0xfc; // and 1111 1100
		data |= 0x01;		// or 0000 0001
	}else if(sleep_status == EN_TOUCH_WAKEUP_MODE) // set 0
	{
		data &=	0xfc; // and 1111 1100
	}
#endif

	printk( "%s() data after = 0x%x\n", __FUNCTION__, data);	
	
	if(g_synaptics_device_control_addr != 0)
	{
		if((ret = i2c_tsp_sensor_write_reg(g_synaptics_device_control_addr, data)) != 0)
		{
			printk("[TSP][ERROR] %s() i2c_tsp_sensor_write_reg error : %d\n", __FUNCTION__ , ret);
			return -1;
		}
	}else{
		printk("[TSP][ERROR] %s() g_synaptics_device_control_addr is not setted\n", __FUNCTION__);
		return -1;
	}
	return 1;
}


void touchscreen_enter_sleep(void)
{
	int ret=0, ii=0;
	//down_interruptible(&sem_sleep_onoff);
	if(g_sleep_onoff_status == EN_TOUCH_WAKEUP_MODE)
	{
		//down_interruptible(&sem_touch_handler);
		if(g_enable_touchscreen_handler == 1)
		{
			g_enable_touchscreen_handler = 0;
			free_irq(tsp.irq, &tsp);
		}else{
			printk( "[TSP][WARNING] %s() handler is already disabled \n", __FUNCTION__);
		}
		//up(&sem_touch_handler);

		g_sleep_onoff_status = EN_TOUCH_SLEEP_MODE;

		//[synaptics] normal mode -> sleep mode

		// must be success !!
		for(ii=0 ; ii<TOUCHSCREEN_SLEEP_WAKEUP_RETRY_COUNTER ; ii++)
		{
			if(synaptics_write_sleep_bit(EN_TOUCH_SLEEP_MODE) == -1)
			{
				printk("[TSP][ERROR] %s() sleep bit control error\n", __FUNCTION__);
			}else
			{
				break;
			}
		}

		if(ii == TOUCHSCREEN_SLEEP_WAKEUP_RETRY_COUNTER)
			goto error_return;
		printk("[TSP] touchscreen enter sleep !\n");
	}else{
		printk( "[TSP][WARNING] %s() call but g_sleep_onoff_status = %d !\n", __FUNCTION__, g_sleep_onoff_status);
	}
	//up(&sem_sleep_onoff);
	return;
	
error_return:
	//up(&sem_sleep_onoff);
	return;
	
}

void touchscreen_poweroff(void)
{
	int ret = 0;
	//down_interruptible(&sem_touch_onoff);
	if(g_touch_onoff_status == EN_TOUCH_POWERON_MODE)
	{
		//down_interruptible(&sem_touch_handler);
		if(g_enable_touchscreen_handler == 1)
		{
			g_enable_touchscreen_handler = 0;
			free_irq(tsp.irq, &tsp);
		}else{
			printk( "[TSP][WARNING] %s() handler is already disabled \n", __FUNCTION__);
		}
		//up(&sem_touch_handler);

		g_touch_onoff_status = EN_TOUCH_POWEROFF_MODE;
#if 0
		ret = resource_release(tsp_rhandle);
		if(ret < 0)
		{
			printk("[TSP] %s() touchscreen resource release fail : %d\n", __FUNCTION__, ret);
			goto error_return;
		}
		udelay(10);
#endif

		printk("[TSP] touchscreen power off !\n");
	}else{
		printk("[TSP] %s() call but g_touch_onoff_status = %d !\n", __FUNCTION__, g_touch_onoff_status);
	}
	//up(&sem_touch_onoff);
	
	return;

error_return:
	//up(&sem_sleep_onoff);
	return;

}

#if 0
struct omap3430_pin_config touch_i2c_gpio_init[] = {
	OMAP3430_PAD_CFG("TOUCH INT",		OMAP3430_PAD_TOUCH_INT, PAD_MODE4, PAD_INPUT_PU, PAD_OFF_PD, PAD_WAKEUP_NA),
	OMAP3430_PAD_CFG("TOUCH I2C SCL", OMAP3430_PAD_I2C_TOUCH_SCL, PAD_MODE0, PAD_INPUT_PU, PAD_OFF_PU, PAD_WAKEUP_NA),
	OMAP3430_PAD_CFG("TOUCH I2C SDA", OMAP3430_PAD_I2C_TOUCH_SDA, PAD_MODE0, PAD_INPUT_PU, PAD_OFF_PU, PAD_WAKEUP_NA),
};

struct omap3430_pin_config synaptics_touch_i2c_gpio_init[] = {
	OMAP3430_PAD_CFG("TOUCH INT",		OMAP3430_PAD_TOUCH_INT, PAD_MODE4, PAD_INPUT_PU, PAD_OFF_INPUT, PAD_WAKEUP_NA),
	OMAP3430_PAD_CFG("TOUCH I2C SCL", OMAP3430_PAD_I2C_TOUCH_SCL, PAD_MODE0, PAD_INPUT_PU, PAD_OFF_PU, PAD_WAKEUP_NA),
	OMAP3430_PAD_CFG("TOUCH I2C SDA", OMAP3430_PAD_I2C_TOUCH_SDA, PAD_MODE0, PAD_INPUT_PU, PAD_OFF_PU, PAD_WAKEUP_NA),
};
#endif

void set_touch_i2c_mode_init(void)
{
	//omap3430_pad_set_configs(synaptics_touch_i2c_gpio_init, ARRAY_SIZE(synaptics_touch_i2c_gpio_init));
	//omap_set_gpio_direction(OMAP_GPIO_TOUCH_IRQ, GPIO_DIR_INPUT);
	return;
}

void touchscreen_wake_up(void)
{
	int ret=0, ii=0;
	
	set_touch_i2c_mode_init();

	udelay(100);
	//down_interruptible(&sem_sleep_onoff);
	if(g_sleep_onoff_status == EN_TOUCH_SLEEP_MODE)
	{

		//[synaptics] sleep mode -> normal mode
		for(ii=0 ; ii<TOUCHSCREEN_SLEEP_WAKEUP_RETRY_COUNTER ; ii++)
		{
			if(synaptics_write_sleep_bit(EN_TOUCH_WAKEUP_MODE) == -1)
			{
				printk("[TSP][ERROR] %s() sleep bit control error\n", __FUNCTION__);
			}else
				break;
		}
		if(ii == TOUCHSCREEN_SLEEP_WAKEUP_RETRY_COUNTER)
			goto error_return;
		synaptics_touchscreen_start_triggering();
		//down_interruptible(&sem_touch_handler);
		if(g_enable_touchscreen_handler == 0)
		{
			g_enable_touchscreen_handler = 1;
			if (request_irq(tsp.irq, touchscreen_handler, tsp.irq_type, TOUCHSCREEN_NAME, &tsp))	
			{
				printk("[TSP][ERROR] %s() Could not allocate touchscreen IRQ!\n", __FUNCTION__);
				tsp.irq = -1;
				input_free_device(tsp.inputdevice);
				goto error_return_2;
			}else
				printk( "[TSP] %s() success register touchscreen IRQ!\n", __FUNCTION__);
		}else{
			printk( "[TSP][WARNING] %s() handler is already enabled \n", __FUNCTION__);
		}
		//up(&sem_touch_handler);
		
		g_sleep_onoff_status = EN_TOUCH_WAKEUP_MODE;
		printk("[TSP] touchscreen_wake_up() success!!\n");
	}else{
		printk( "[TSP][WARNING] %s() call but g_sleep_onoff_status = %d !\n", __FUNCTION__, g_sleep_onoff_status);
	}
	//up(&sem_sleep_onoff);
	return;

error_return:
	//up(&sem_sleep_onoff);
	return;

error_return_2:
	//up(&sem_touch_handler);
	//up(&sem_sleep_onoff);
	return;
}

void touchscreen_poweron(void)
{
	int ret = 0;

	set_touch_i2c_mode_init();

	udelay(100);

	//down_interruptible(&sem_touch_onoff);
	if(g_touch_onoff_status == EN_TOUCH_POWEROFF_MODE)
	{
		printk("[TSP] synaptics touchscreen power on\n");
		//synaptics_set_GPIO_direction_out();
		// set gpio low
		//gpio_direction_output(OMAP_GPIO_TOUCH_IRQ, 0);
		//gpio_direction_output(OMAP_GPIO_TSP_SCL, 0);
		//gpio_direction_output(OMAP_GPIO_TSP_SDA, 0);
		//udelay(50);
	#if 0	
		if(tsp_rhandle != NULL)
		{
			ret = resource_request(tsp_rhandle, T2_VAUX2_2V80);
			if(ret < 0)
			{
				printk("[TSP][ERROR] %s() resource_request fail : %d\n", __FUNCTION__, ret);
				goto error_return;
			}
		}
		else
		{
			printk("[TSP][ERROR] enable_tsp_pins() - tsp_rhandle is NULL\n");
			goto error_return;
		}
	#endif	
                //nowplus_enable_touch_pins(1);//me add
		set_touch_i2c_mode_init();
		mdelay(200);

		synaptics_touchscreen_start_triggering();

		//down_interruptible(&sem_touch_handler);
		if(g_enable_touchscreen_handler == 0)
		{
			g_enable_touchscreen_handler = 1;
			if (request_irq(tsp.irq, touchscreen_handler, tsp.irq_type, TOUCHSCREEN_NAME, &tsp))	
			{
				printk("[TSP][ERROR] %s() Could not allocate touchscreen IRQ!\n", __FUNCTION__);
				tsp.irq = -1;
				input_free_device(tsp.inputdevice);
				goto error_return_2;
			}else
				printk( "[TSP] %s() success register touchscreen IRQ!\n", __FUNCTION__);
		}else{
			printk( "[TSP][WARNING] %s() handler is already enabled \n", __FUNCTION__);
		}
		//up(&sem_touch_handler);
		
		g_touch_onoff_status = EN_TOUCH_POWERON_MODE;
		g_sleep_onoff_status = EN_TOUCH_WAKEUP_MODE;
	}else{
		printk("[TSP] %s() call but g_touch_onoff_status = %d !\n", __FUNCTION__, g_touch_onoff_status);
	}
	//up(&sem_touch_onoff);

	return;

error_return:
	//up(&sem_touch_onoff);
	return;

error_return_2:
	//up(&sem_touch_handler);
	//up(&sem_touch_onoff);
	return;

}

#ifdef __TSP_I2C_ERROR_RECOVERY__
void touch_ic_recovery()
{
	printk("[TSP] try to touch IC reset...\n");
	g_last_recovery_time = current_kernel_time();
	g_i2c_error_recovery_count++;
	touchscreen_poweroff();
	touchscreen_poweron();
	printk("[TSP] complete to reset touch IC...\n");
}
#endif


#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
static void touch_RF_nosie_recovery(int wx1, int wy1, int wx2, int wy2, int finger_type)
{
	printk("[TSP][WARNING] == touch is unstable state.!! (%d, %d, %d, %d), finger_type(%d)\n", wx1, wy1, wx2, wy2, finger_type);
	do_gettimeofday(&g_last_rf_noise_recovery_time);
	
	if(g_rf_recovery_behavior_status == 1) {
		printk("[TSP][WARNING] == reset touch chip baseband by 'rezero'.\n");
		i2c_tsp_sensor_write_reg(0x67, 0x1);
	}
	else if(g_rf_recovery_behavior_status == 2) {
		printk("[TSP][WARNING] == reset touch chip baseband by 'reset'.\n");
		i2c_tsp_sensor_write_reg(0x66, 0x1);
	}
	else //g_rf_recovery_behavior_status == 0
		printk("[TSP][WARNING] == NO reset touch chip baseband.\n");

	del_timer(&tsp_rf_noise_recovery_timer);
	tsp_rf_noise_recovery_timer.expires = (jiffies + msecs_to_jiffies(60));
	add_timer(&tsp_rf_noise_recovery_timer);

	g_synaptics_unstable_recovery_count++;

}
#endif

/******************************************************
******************   FUNCTION NOTICE   ********************
*
*****  synaptics_touchscreen_read(struct work_struct *work)    *****
*
* never call this function in irq handler directly.
* have to call "kfree()", If you want to "return" suddenly.
*
*******************************************************/
void synaptics_touchscreen_read(struct work_struct *work)
{
	int ret = 0;
	u8 *data;
	u16 x_position = 0;
	u16 y_position = 0;
	u16 x2_position = 0;
	u16 y2_position = 0;
	int press = 0;
	int press2 = 0;
	int finger_type = 0;
#ifdef __CONFIG_SYNAPTICS_MULTI_TOUCH__
	int muilti_touch_id = 0;
#endif
#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
	u8 WX1 = 0;
	u8 WY1 = 0;
	u8 WX2 = 0;
	u8 WY2 = 0;
#endif
	do_gettimeofday(&g_current_timestamp);
//	if (touch_debug_status) {
//		printk("[TSP][DEBUG]read start time : %ld sec, %6ld microsec\n", g_current_timestamp.tv_sec, g_current_timestamp.tv_usec);
//	}

	data = kmalloc(g_synaptics_read_cnt, GFP_KERNEL);
	if(data == NULL)
	{
		printk("[TSP][ERROR] %s() kmalloc fail\n", __FUNCTION__ );
		kfree(data);
		return;
	}

	ret = i2c_tsp_sensor_read(g_synaptics_read_addr, data, g_synaptics_read_cnt);
	if(ret != 0)
	{
		printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
		kfree(data);
		return;
	}

	press = (data[0x15-g_synaptics_read_addr] & 0x03);
	x_position = data[0x16-g_synaptics_read_addr];
	x_position = (x_position << 4) | (data[0x18-g_synaptics_read_addr] & 0x0f);
	y_position = data[0x17-g_synaptics_read_addr];
	y_position = (y_position << 4) | (data[0x18-g_synaptics_read_addr]  >> 4);
	press2 = ((data[0x15-g_synaptics_read_addr] & 0x0C)>>2);
	x2_position = data[0x1B-g_synaptics_read_addr];
	x2_position = (x2_position << 4) | (data[0x1D-g_synaptics_read_addr] & 0x0f);
	y2_position = data[0x1C-g_synaptics_read_addr];
	y2_position = (y2_position << 4) | (data[0x1D-g_synaptics_read_addr]  >> 4);

	if((press == 0) && (press2 == 0)) {
		finger_type=0;
	}
	else if((press == 1) && (press2 == 0)) {
		finger_type=1;
		finger_switched=0;
	}
	else if((press == 0) && (press2 == 1)) {
		finger_type=2;
		finger_switched=1;
	}
	else if((press == 1) && (press2 == 1))
		finger_type=3;
	else if((press == 2) && (press2 == 2))
		finger_type=4;
	else if((press == 0) && (press2 == 2)) {
		finger_type=5;
		finger_switched=1;
	}
	else if((press == 2) && (press2 == 0)) {
		finger_type=6;
		finger_switched=0;
	}
	else
		finger_type=7;
#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
	WX1 = (data[0x19-g_synaptics_read_addr] & 0x0F);
	WY1 = (data[0x19-g_synaptics_read_addr] >> 4);
	WX2 = (data[0x1E - g_synaptics_read_addr] & 0x0F);
	WY2 = (data[0x1E - g_synaptics_read_addr] >> 4);
#endif

/////////////////////////////////////////////	 press event processing start   //////////////////////////////////////////////////

	if ((finger_type > 0) && (finger_type < 7))		// press
	{
		//same point filtering
		if( finger_type == syna_old_finger_type) 
		{
			if((x_position == syna_old_x_position) && (y_position == syna_old_y_position))
			{
				if((x2_position == syna_old_x2_position) && (y2_position == syna_old_y2_position)) {
					kfree(data);
					return;
				}
			}
		}
		//end same point filtering

		syna_old_x_position = x_position;
	 	syna_old_y_position = y_position;
		syna_old_x2_position = x2_position;
		syna_old_y2_position = y2_position;

#ifdef __SYNA_MULTI_TOUCH_SUPPORT__
		//making release event(2finger => 1finger)
		if((syna_old_finger_type==3) || (syna_old_finger_type==4))
		{
			if((finger_type == 2) || (finger_type == 5)) {
				if((syna_old_old_finger_type == 1) || (syna_old_old_finger_type == 6)) {
					input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
					input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
					input_sync(tsp.inputdevice);
					//printk( "[TSP][UP][M->S]type:%d, x=%d, y=%d\n", finger_type, x_position, y_position);
				}
			}
			else if((finger_type == 1) || (finger_type == 6)) {
				if((syna_old_old_finger_type == 2) || (syna_old_old_finger_type == 5)) {
					input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
					input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
					input_sync(tsp.inputdevice);
					//printk( "[TSP][UP][M->S]type:%d, x=%d, y=%d\n", finger_type, x2_position, y2_position);
				}
			}
		}
		//2finger => 1finger making release event done
#endif //__SYNA_MULTI_TOUCH_SUPPORT__

		//send press event
		if((finger_type == 1) || (finger_type == 2) || (finger_type == 5) || (finger_type == 6)) {
#ifdef __MAKE_MISSED_UP_EVENT__
			if(finger_type == 1) {
				if(syna_old_finger_type == 2) {
                                        input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
					input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
					input_sync(tsp.inputdevice);
					//printk( "[TSP][UP][F]type:%d, x=%d, y=%d\n", finger_type, x2_position, y2_position);
				}
			}
			else if(finger_type == 2) {
				if(syna_old_finger_type == 1) {
					input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
					input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
					input_sync(tsp.inputdevice);
					//printk( "[TSP][UP][F]type:%d, x=%d, y=%d\n", finger_type, x_position, y_position);
				}
			}
#endif
#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
			if((WX1 > 0xa) || (WY1 > 0xa) || (WX2 > 0xa) || (WY2 > 0xa)) {
				touch_RF_nosie_recovery(WX1, WY1, WX2, WY2, finger_type);
				if(g_rf_recovery_behavior_status !=0) {
					kfree(data);
					return;
				}
			}
#endif	// __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
			if(finger_switched == 0) {
				input_report_abs(tsp.inputdevice, ABS_X, x_position);
				input_report_abs(tsp.inputdevice, ABS_Y, y_position);
			}
			else {
				input_report_abs(tsp.inputdevice, ABS_X, x2_position);
				input_report_abs(tsp.inputdevice, ABS_Y, y2_position);
			}
			input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_DOWN);//me add
                        input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_DOWN);
			input_sync(tsp.inputdevice);
#if 0 //me change
			if(finger_switched == 0)
				//printk( "[TSP][DOWN]type:%d, x=%d, y=%d\n", finger_type, x_position, y_position);
			else if(finger_switched == 1)
				//printk( "[TSP][DOWN]type:%d, x=%d, y=%d\n", finger_type, x2_position, y2_position);
#endif
		}
		else if((finger_type == 3) || (finger_type == 4)) {
#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
//change rf_noise recovery excution condition of multi touch ( W value > 0xa => 0x0 )
			touch_RF_nosie_recovery(WX1, WY1, WX2, WY2, finger_type);
			if(g_rf_recovery_behavior_status !=0) {
				kfree(data);
				return;
			}
#endif	// __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
			printk( "[TSP][DOWN][WARNING]type:%d, Multi touch is not supported.\n", finger_type);
		}
	}
		//end press event((finger_type > 0) && (finger_type < 7))

/////////////////////////////////////////////    release event processing start   //////////////////////////////////////////////////
	else if(finger_type == 0)
	{
		if((syna_old_finger_type == 1) || (syna_old_finger_type == 2) || (syna_old_finger_type == 5) || (syna_old_finger_type == 6)) {
			input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
					input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
			input_sync(tsp.inputdevice);
#if 0 //me change			
                       if(finger_switched == 0)
				printk( "[TSP][UP]type:%d, x=%d, y=%d\n", finger_type, x_position, y_position);
			else if(finger_switched == 1)
				printk( "[TSP][UP]type:%d, x=%d, y=%d\n", finger_type, x2_position, y2_position);
#endif
#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
			if((WX1 > 0xa) || (WY1 > 0xa) || (WX2 > 0xa) || (WY2 > 0xa)) {
				touch_RF_nosie_recovery(WX1, WY1, WX2, WY2, finger_type);
				if(g_rf_recovery_behavior_status !=0) {
					kfree(data);
					return;
				}
			}
#endif	//__SYNAPTICS_UNSTABLE_TSP_RECOVERY__
		}
		else if((syna_old_finger_type == 3) || (syna_old_finger_type == 4)) {
#ifndef __SYNA_MULTI_TOUCH_SUPPORT__
			input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
					input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
			input_sync(tsp.inputdevice);
			//printk( "[TSP][UP][WARNING]type:%d, Multi touch is not supported. But send release event.\n", finger_type);
#else //__SYNA_MULTI_TOUCH_SUPPORT__
			//making two release event(2finger => release at the sametime)
			//1st finger up event
			input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
					input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
			input_sync(tsp.inputdevice);

			//2nd finger down event
			if((syna_old_old_finger_type == 1) || (syna_old_old_finger_type == 6)) {
				input_report_abs(tsp.inputdevice, ABS_X, x2_position);
				input_report_abs(tsp.inputdevice, ABS_Y, y2_position);
			}
			else if((syna_old_old_finger_type == 2) || (syna_old_old_finger_type == 5)) {
				input_report_abs(tsp.inputdevice, ABS_X, x_position);
				input_report_abs(tsp.inputdevice, ABS_Y, y_position);
			}
			input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_DOWN);
			input_sync(tsp.inputdevice);

			//2nd finger up event
			input_report_key(tsp.inputdevice, BTN_TOUCH, DEFAULT_PRESSURE_UP);//me add
					input_report_abs(tsp.inputdevice, ABS_PRESSURE, DEFAULT_PRESSURE_UP);
			input_sync(tsp.inputdevice);
			if((syna_old_old_finger_type == 1) || (syna_old_old_finger_type == 6)) {
				printk( "[TSP][UP][M->R]type:%d, x=%d, y=%d\n", finger_type, x_position, y_position);
				printk( "[TSP][DOWN][M->R]type:%d, x=%d, y=%d\n", finger_type, x2_position, y2_position);
				printk( "[TSP][UP][M->R]type:%d, x=%d, y=%d\n", finger_type, x2_position, y2_position);
			}
			else if((syna_old_old_finger_type == 2) || (syna_old_old_finger_type == 5)) {
				printk( "[TSP][UP][M->R]type:%d, x=%d, y=%d\n", finger_type, x2_position, y2_position);
				printk( "[TSP][DOWN][M->R]type:%d, x=%d, y=%d\n", finger_type, x_position, y_position);
				printk( "[TSP][UP][M->R]type:%d, x=%d, y=%d\n", finger_type, x_position, y_position);
			}
#endif //__SYNA_MULTI_TOUCH_SUPPORT__
#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
			if((WX1 > 0xa) || (WY1 > 0xa) || (WX2 > 0xa) || (WY2 > 0xa)) {
				touch_RF_nosie_recovery(WX1, WY1, WX2, WY2, finger_type);
				if(g_rf_recovery_behavior_status !=0) {
					kfree(data);
					return;
				}
			}
#endif	// __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
		}

		finger_switched = 0;
		syna_old_x_position = -1;
	 	syna_old_y_position = -1;
		syna_old_x2_position = -1;
		syna_old_y2_position = -1;
	}

//////////////////////////////////////////	 unknown finger type  processing start   ///////////////////////////////////////////////
	else
	{
#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
		if((WX1 > 0xa) || (WY1 > 0xa) || (WX2 > 0xa) || (WY2 > 0xa)) {
			touch_RF_nosie_recovery(WX1, WY1, WX2, WY2, finger_type);
			if(g_rf_recovery_behavior_status !=0) {
				kfree(data);
				return;
			}
		}
#endif	// __SYNAPTICS_UNSTABLE_TSP_RECOVERY__

		//printk("[TSP][ERROR] Unknown finger_type : %d\n", finger_type);
		//printk( "[TSP][ERROR] press : %d, press2 : %d\n",press, press2);
	}
	syna_old_finger_type = finger_type;
#ifdef __SYNA_MULTI_TOUCH_SUPPORT__
		if((syna_old_finger_type != 3) && (syna_old_finger_type != 4))
			syna_old_old_finger_type = finger_type;
#endif
	kfree(data);
}

static irqreturn_t touchscreen_handler(int irq, void *dev_id)
{
//printk("[TSP] touchscreen_handler----------- !! \n");
#ifdef __TSP_I2C_ERROR_RECOVERY__
	if(g_touch_irq_handler_count < MAX_HANDLER_COUNT)
		g_touch_irq_handler_count++;
	else
		g_touch_irq_handler_count = 0;
#endif

//	cancel_delayed_work(&tsp_up_work);	 
	//schedule_work(&tsp_work);
	queue_work(tsp_wq, &tsp_work);
//	schedule_delayed_work(&tsp_up_work, msecs_to_jiffies(30));

#ifndef __CONFIG_CYPRESS_USE_RELEASE_BIT__
	del_timer(&tsp_timer);
	tsp_timer.expires  = (jiffies + msecs_to_jiffies(60));
	add_timer(&tsp_timer);
#endif

	return IRQ_HANDLED;
}

void touchscreen_poweronoff(int onoff)
{
	printk( "[TSP] %s(%d)\n", __FUNCTION__, onoff);
	if (onoff)
	{
		touchscreen_wake_up();
	}
	else
	{
		touchscreen_enter_sleep();
	}
	
	return;
}
EXPORT_SYMBOL(touchscreen_poweronoff);

static int __init touchscreen_probe(struct platform_device *pdev)
{
	int ret = 0;
        u8 data[2] = {0, };
	printk("[TSP] touchscreen_probe !! \n");

	// init target dependent value
	touchscreen_read = synaptics_touchscreen_read;
	g_version_read_addr = 0x75;
	g_synaptics_read_addr = 0x13;
	g_synaptics_read_cnt = 13;
	g_synaptics_device_control_addr = 0x23;
	
	//sema_init(&sem_touch_onoff, 1);
	//sema_init(&sem_sleep_onoff, 1);
	//sema_init(&sem_touch_handler, 1);
	
	memset(&tsp, 0, sizeof(tsp));
	
	tsp.inputdevice = input_allocate_device();

	if (!tsp.inputdevice)
	{
		printk("[TSP][ERROR] input_allocate_device fail \n");
		return -ENOMEM;
	}

	spin_lock_init(&tsp.lock);

	/* request irq */
	if (tsp.irq != -1)
	{
		tsp.irq = OMAP_GPIO_IRQ(OMAP_GPIO_TOUCH_IRQ);
		tsp.irq_type = IRQF_TRIGGER_FALLING;

#ifndef CONFIG_LATE_HANDLER_ENABLE
		if (request_irq(tsp.irq, touchscreen_handler, tsp.irq_type, TOUCHSCREEN_NAME, &tsp))	
		{
			printk("[TSP][ERROR] Could not allocate touchscreen IRQ!\n");
			tsp.irq = -1;
			input_free_device(tsp.inputdevice);
			return -EINVAL;
		}

		//down_interruptible(&sem_touch_handler);
		g_enable_touchscreen_handler = 1;
		//up(&sem_touch_handler);
#endif // CONFIG_LATE_HANDLER_ENABLE

		tsp.irq_enabled = 1;
	}
	
	tsp.inputdevice->name = TOUCHSCREEN_NAME;
	tsp.inputdevice->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) | BIT_MASK(EV_SYN);//BIT(EV_ABS);me change
	tsp.inputdevice->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
        //tsp.inputdevice->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
        tsp.inputdevice->id.bustype = BUS_I2C;
	tsp.inputdevice->id.vendor  = 0;
	tsp.inputdevice->id.product =0;
 	tsp.inputdevice->id.version =0;
        input_set_abs_params(tsp.inputdevice, ABS_X, 0, MAX_TOUCH_X_RESOLUTION, 0, 0);
        input_set_abs_params(tsp.inputdevice, ABS_Y, 0, MAX_TOUCH_Y_RESOLUTION, 0, 0);
        input_set_abs_params(tsp.inputdevice, ABS_PRESSURE, 0, 1, 0, 0);

	tsp_wq = create_singlethread_workqueue("tsp_wq");
	INIT_WORK(&tsp_work, touchscreen_read);

#ifndef __CONFIG_CYPRESS_USE_RELEASE_BIT__
	init_timer(&tsp_timer);
	tsp_timer.expires = (jiffies + msecs_to_jiffies(60));
	tsp_timer.function = tsp_timer_handler;
#endif

	init_timer(&tsp_rf_noise_recovery_timer);
	tsp_rf_noise_recovery_timer.expires = (jiffies + msecs_to_jiffies(60));
	tsp_rf_noise_recovery_timer.function = tsp_rf_noise_recovery_timer_handler;

	ret = input_register_device(tsp.inputdevice);
#ifdef CONFIG_HAS_EARLYSUSPEND
	tsp.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	tsp.early_suspend.suspend = touchscreen_early_suspend;
	tsp.early_suspend.resume = touchscreen_late_resume;
	register_early_suspend(&tsp.early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */
#if 0//me add
	tsp_rhandle = regulator_get(&pdev->dev, "vaux2");//resource_get("TSP", "t2_vaux2");
	if (tsp_rhandle == NULL)
	{
		printk(KERN_ERR "[TSP][ERROR] : Failed to get tsp power resources !! \n");
		return -ENODEV;
	}
        regulator_enable(tsp_rhandle);
#endif
	i2c_tsp_sensor_init();
	
	ret = device_create_file(&tsp.inputdevice->dev, &dev_attr_issp);

#ifdef CONFIG_LATE_HANDLER_ENABLE
	ret = device_create_file(&tsp.inputdevice->dev, &dev_attr_tsp_enable);
#endif

	ret = device_create_file(&tsp.inputdevice->dev, &dev_attr_touchcontrol);
	ret = device_create_file(&tsp.inputdevice->dev, &dev_attr_get_register);
	ret = device_create_file(&tsp.inputdevice->dev, &dev_attr_set_register);

#ifdef __SYNAPTICS_UNSTABLE_TSP_RECOVERY__
	ret = device_create_file(&tsp.inputdevice->dev, &dev_attr_rf_recovery_behavior);
#endif

#ifndef CONFIG_LATE_HANDLER_ENABLE
	ret = i2c_tsp_sensor_read(g_version_read_addr, data, 2);
	if(ret != 0)
	{
		printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
	}

	printk("[TSP] touch driver version : %d.%d, firmware S/W Revision Info = %x\n", 
		__TOUCH_DRIVER_MAJOR_VER__,__TOUCH_DRIVER_MINOR_VER__, data[0]);
#endif
	printk("[TSP] success %s() !\n", __FUNCTION__);
#if 1  //me add
        touchscreen_poweron();
        ret = i2c_tsp_sensor_read(g_version_read_addr, data, 2);
		if(ret != 0)
		{
			printk("[TSP][ERROR] %s() i2c_tsp_sensor_read error : %d\n", __FUNCTION__ , ret);
			touch_ic_recovery();
		}
		printk( "[TSP] touch driver version : %d.%d, firmware S/W Revision Info = %x\n", 
			__TOUCH_DRIVER_MAJOR_VER__,__TOUCH_DRIVER_MINOR_VER__, data[0]);

		synaptics_touchscreen_start_triggering();
#endif
	return 0;
}

static int touchscreen_remove(struct platform_device *pdev)
{

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&tsp.early_suspend);
#endif	/* CONFIG_HAS_EARLYSUSPEND */

	input_unregister_device(tsp.inputdevice);

	if (tsp.irq != -1)
	{
		//down_interruptible(&sem_touch_handler);
		if(g_enable_touchscreen_handler == 1)
		{
			free_irq(tsp.irq, &tsp);
			g_enable_touchscreen_handler = 0;
		}else{
			printk( "[TSP][WARNING] %s() handler is already disabled \n", __FUNCTION__);
		}
		//up(&sem_touch_handler);
	}
#if 0 //me add
                regulator_disable(tsp_rhandle);
		regulator_put(tsp_rhandle);
#endif
#if 0
	resource_put(tsp_rhandle);
	tsp_rhandle = NULL;
#endif
	return 0;
}


static int touchscreen_suspend(struct platform_device *pdev, pm_message_t state)
{
// touch power is controled only by sysfs.
	printk("touchscreen_suspend : touch power off\n");
	touchscreen_poweronoff(0);
        nowplus_enable_touch_pins(0);//me add
	return 0;
}

static int touchscreen_resume(struct platform_device *pdev)
{
// touch power is controled only by sysfs.
	printk("touchscreen_resume : touch power on\n");
        nowplus_enable_touch_pins(1);//me add
	touchscreen_poweronoff(1);
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
void touchscreen_early_suspend(struct early_suspend *h)
{
//	melfas_ts_suspend(PMSG_SUSPEND);
	touchscreen_suspend(&touchscreen_device, PMSG_SUSPEND);
}

void touchscreen_late_resume(struct early_suspend *h)
{
//	melfas_ts_resume();
	touchscreen_resume(&touchscreen_device);
}
#endif	/* CONFIG_HAS_EARLYSUSPEND */

static int __init touchscreen_init(void)
{
	int ret;

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
	platform_driver_unregister(&touchscreen_driver);
	platform_device_unregister(&touchscreen_device);
}

module_init(touchscreen_init);
module_exit(touchscreen_exit);

MODULE_LICENSE("GPL");
