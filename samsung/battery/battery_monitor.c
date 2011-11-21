/*
 * module/samsung_battery/battery_monitor.c
 *
 * SAMSUNG battery driver for Linux
 *
 * Copyright (C) 2009 SAMSUNG ELECTRONICS.
 * Author: EUNGON KIM (egstyle.kim@samsung.com)
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/i2c/twl4030-madc.h>
#include <linux/i2c/twl.h>
#include <linux/power_supply.h>
#include <linux/wakelock.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
//#include <mach/resource.h>
#include <plat/omap3-gptimer12.h>
#include "common.h"
// [ To access sysfs file system.
#include <linux/uaccess.h>
#include <linux/fs.h>
// ]
//#include <mach/archer.h>

#define DRIVER_NAME "secBattMonitor"

#define TEMP_DEG    0
#define TEMP_ADC    1

/* In module TWL4030_MODULE_PM_RECEIVER */
#define VSEL_VINTANA2_2V75  0x01
#define CARKIT_ANA_CTRL     0xBB
#define SEL_MADC_MCPC       0x08

static DEFINE_MUTEX( battery_lock );


static int NCP15WB473_batt_table[] = 
{
	/* -15C ~ 85C */

	/*0[-15] ~  14[-1]*/
	360850,342567,322720,304162,287000,
	271697,255331,241075,227712,215182,
	206463,192394,182034,172302,163156, 
	/*15[0] ~ 34[19]*/
	158214,146469,138859,131694,124947,
	122259,118590,112599,106949,101621,
	95227, 91845, 87363, 83128, 79126,
	74730, 71764, 68379, 65175, 62141,
	/*35[20] ~ 74[59]*/    
	59065, 56546, 53966, 51520, 49201,
	47000, 44912, 42929, 41046, 39257,
	37643, 35942, 34406, 32945, 31555,
	30334, 28972, 27773, 26630, 25542,
	24591, 23515, 22571, 21672, 20813,
	20048, 19211, 18463, 17750, 17068,
	16433, 15793, 15197, 14627, 14082,
	13539, 13060, 12582, 12124, 11685,
	/*75[60] ~ 100[85]*/
	11209, 10862, 10476, 10106,  9751,
	9328,  9083,  8770,  8469,  8180,
	7798,  7635,  7379,  7133,  6896,
	6544,  6450,  6240,  6038,  5843,
	5518,  5475,  5302,  5134,  4973,
	4674
};
#if 0
static int NCP15WB473_batt_table[] = 
{
    /* -15C ~ 85C */
    
    /*0[-15] ~  14[-1]*/
    386533,363802,342567,322720,304162,
    286799,270549,255331,241075,227712,
    215182,203427,192394,182034,172302, 
    /*15[0] ~ 34[19]*/
    163156,154557,146469,138859,131694,
    124947,118590,112599,106949,101621,
     96592, 91845, 87363, 83128, 79126,
     75342, 71764, 68379, 65175, 62141,
    /*35[20] ~ 74[59]*/    
     59268, 56546, 53966, 51520, 49201,
     47000, 44912, 42929, 41046, 39257,
     37558, 35942, 34406, 32945, 31555,
     30232, 28972, 27773, 26630, 25542,
     24504, 23515, 22571, 21672, 20813,
     19993, 19211, 18463, 17750, 17068,
     16416, 15793, 15197, 14627, 14082,
     13560, 13060, 12582, 12124, 11685,
    /*75[60] ~ 100[85]*/
     11265, 10862, 10476, 10106,  9751,
      9410,  9083,  8770,  8469,  8180,
      7902,  7635,  7379,  7133,  6896,
      6669,  6450,  6240,  6038,  5843,
      5656,  5475,  5302,  5134,  4973,
      4818
};
#endif

struct battery_device_config
// THIS CONFIG IS SET IN BOARD_FILE.(platform_data)
{
	/* SUPPORT MONITORING CHARGE CURRENT FOR CHECKING FULL */
	int MONITORING_CHG_CURRENT;
	int CHG_CURRENT_ADC_PORT;

	/* SUPPORT MONITORING TEMPERATURE OF THE SYSTEM FOR BLOCKING CHARGE */
	int MONITORING_SYSTEM_TEMP;
	int TEMP_ADC_PORT;
};

static struct battery_device_config *device_config;

struct battery_device_info 
{
	struct device *dev;
	struct delayed_work battery_monitor_work;

	struct regulator *usb1v5;
	struct regulator *usb1v8;
	struct regulator *usb3v1;

	struct power_supply sec_battery;
	struct power_supply sec_ac;
	struct power_supply sec_usb;	
};

static struct device *this_dev;


struct gptimer12_timer batt_gptimer_12;

static SEC_battery_charger_info sec_bci;
SEC_battery_charger_info *get_sec_bci( void )
{
	return &sec_bci;
}

#if 0
static unsigned int debug_fleeting_counter=0;
#endif

static struct wake_lock sec_bc_wakelock, adc_wakelock;

// Prototype
       int _charger_state_change_( int , int, bool );
       int _low_battery_alarm_( void );
       int _get_average_value_( int *, int );
       int _get_t2adc_data_( int );
static int turn_resources_off_for_adc( void );
static int turn_resources_on_for_adc( void );
static int get_elapsed_time_secs( unsigned long long * );
static int t2adc_to_temperature( int , int );
static int do_fuelgauge_reset( void );
static int get_battery_level_adc( void );
static int get_battery_level_ptg( void );
static void update_battery_level_n_go( struct battery_device_info *di );
static int get_system_temperature( bool );
static int get_charging_current_adc_val( void );
static int check_full_charge_using_chg_current( int );
static void get_system_status_in_sleep( int *, int *, int *, int * );
static int battery_monitor_core( bool );
static void battery_monitor_work_handler( struct work_struct * );
static int battery_monitor_fleeting_wakeup_handler( unsigned long ); 
static int samsung_battery_get_property( struct power_supply *, enum power_supply_property , union power_supply_propval * );
static int samsung_ac_get_property( struct power_supply *, enum power_supply_property, union power_supply_propval * );
static int samsung_usb_get_property( struct power_supply *, enum power_supply_property, union power_supply_propval * );
static void samsung_pwr_external_power_changed( struct power_supply * );
static int get_charging_source( void ); 

static int __devinit battery_probe( struct platform_device * );
static int __devexit battery_remove( struct platform_device * );
static int battery_suspend( struct platform_device *, pm_message_t );
static int battery_resume( struct platform_device * );
static int __init battery_init( void );
static void __exit battery_exit( void );

// charger
extern int charger_init( void );
extern void charger_exit( void );
extern int _battery_state_change_( int category, int value, bool is_sleep );
extern int _check_full_charge_dur_sleep_( void );
extern int _cable_status_now_( void );

// fuelgauge
extern int fuelgauge_init( void );
extern void fuelgauge_exit( void );
extern int fuelgauge_quickstart( void );
extern int get_fuelgauge_adc_value( int count, bool is_sleep );
extern int get_fuelgauge_ptg_value( bool is_sleep );

// sleep i2c, adc
extern void twl4030_i2c_init( void );
extern void twl4030_i2c_disinit( void );
extern void normal_i2c_init( void );
extern void normal_i2c_disinit( void );
extern s32 t2_adc_data( u8 channel );

extern unsigned long long sched_clock( void );

extern void (*sec_set_param_value)(int idx, void *value);
extern void (*sec_get_param_value)(int idx, void *value);


// ------------------------------------------------------------------------- // 
//                                                                           //
//                           sysfs interface                                 //
//                                                                           //
// ------------------------------------------------------------------------- // 
#define  __ATTR_SHOW_CALLBACK( _name, _ret_val ) \
static ssize_t _name( struct kobject *kobj, \
		      struct kobj_attribute *attr, \
		      char *buf ) \
{ \
	return sprintf ( buf, "%d\n", _ret_val ); \
} 

static int get_batt_monitor_temp( void )
{
	return sec_bci.battery.support_monitor_temp;
}

static ssize_t store_batt_monitor_temp(struct kobject *kobj,
				    struct kobj_attribute *attr,
				    const char *buf, size_t size)
{
	int flag;
	
	sscanf( buf, "%d", &flag );

	printk("[BR] change value %d\n",flag);

	sec_bci.battery.support_monitor_temp = flag;
	sec_bci.battery.support_monitor_timeout = flag;
	sec_bci.battery.support_monitor_full = flag;
	return size;
}


__ATTR_SHOW_CALLBACK( show_batt_vol_toolow, sec_bci.battery.battery_vol_toolow )
	
static struct kobj_attribute batt_vol_toolow =
	__ATTR( batt_vol_toolow, 0644, show_batt_vol_toolow, NULL );



__ATTR_SHOW_CALLBACK( show_charging_source, get_charging_source() )
	
static struct kobj_attribute charging_source =
	__ATTR( charging_source, 0644, show_charging_source, NULL );


__ATTR_SHOW_CALLBACK( show_batt_vol, get_battery_level_adc() )
__ATTR_SHOW_CALLBACK( show_batt_vol_adc, 0 )
__ATTR_SHOW_CALLBACK( show_batt_temp, get_system_temperature( TEMP_DEG ) )
__ATTR_SHOW_CALLBACK( show_batt_temp_adc, get_system_temperature( TEMP_ADC ) )
__ATTR_SHOW_CALLBACK( show_batt_v_f_adc, 0 )
__ATTR_SHOW_CALLBACK( show_batt_capacity, get_battery_level_ptg() )
__ATTR_SHOW_CALLBACK( do_batt_fuelgauge_reset, do_fuelgauge_reset() )
__ATTR_SHOW_CALLBACK( show_batt_monitor_temp, get_batt_monitor_temp() )


static struct kobj_attribute batt_sysfs_testmode[] = {
	__ATTR( batt_vol, 0644, show_batt_vol, NULL ),
	__ATTR( batt_vol_adc, 0644, show_batt_vol_adc, NULL ),
	__ATTR( batt_temp, 0644, show_batt_temp, NULL ),
	__ATTR( batt_temp_adc, 0644, show_batt_temp_adc, NULL ),
	__ATTR( batt_v_f_adc, 0644, show_batt_v_f_adc, NULL ),
	__ATTR( batt_capacity, 0644, show_batt_capacity, NULL ),
	__ATTR( batt_fuelgauge_reset, 0644, do_batt_fuelgauge_reset, NULL ),
	__ATTR( batt_monitor_temp, 0664, show_batt_monitor_temp, store_batt_monitor_temp ),
};
// ------------------------------------------------------------------------- //



int _charger_state_change_( int category, int value, bool is_sleep )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument : 
// Return Value   :  
{	
	printk( "[BR] cate: %d, value: %d\n", category, value );

	if( category == STATUS_CATEGORY_CABLE )
	{
		switch( value )
		{
		case POWER_SUPPLY_TYPE_BATTERY :
			/*Stop monitoring the batt. level for Re-charging*/
			sec_bci.battery.monitor_field_rechg_vol = false;

			/*Stop monitoring the temperature*/
			sec_bci.battery.monitor_field_temp = false;

			sec_bci.battery.confirm_full_by_current = 0;
				//sec_bci.battery.confirm_changing_freq = 0;
			sec_bci.battery.confirm_recharge = 0;

			sec_bci.charger.charging_timeout = DEFAULT_CHARGING_TIMEOUT;

			sec_bci.charger.full_charge_dur_sleep = 0x0;
			
			break;

		case POWER_SUPPLY_TYPE_MAINS :
			sec_bci.charger.charging_timeout = DEFAULT_CHARGING_TIMEOUT;
			wake_lock_timeout( &sec_bc_wakelock , HZ );			
			break;

		case POWER_SUPPLY_TYPE_USB :			
			break;

		default :
			;
		}

		goto Out_Charger_State_Change;
	}
	else if( category == STATUS_CATEGORY_CHARGING )
	{
		switch( value )
		{
		case POWER_SUPPLY_STATUS_UNKNOWN :
		case POWER_SUPPLY_STATUS_NOT_CHARGING :
			//sec_bci.charger.full_charge = false;
			
			/*Stop monitoring the batt. level for Re-charging*/
			sec_bci.battery.monitor_field_rechg_vol = false;

			if( sec_bci.battery.battery_health != POWER_SUPPLY_HEALTH_OVERHEAT 
				&& sec_bci.battery.battery_health != POWER_SUPPLY_HEALTH_COLD )
			{
				/*Stop monitoring the temperature*/
				sec_bci.battery.monitor_field_temp = false;
			}

			break;

		case POWER_SUPPLY_STATUS_DISCHARGING :
			//sec_bci.charger.full_charge = false;

			break;

		case POWER_SUPPLY_STATUS_FULL :
			//sec_bci.charger.full_charge = true;
			
			/*Start monitoring the batt. level for Re-charging*/
			sec_bci.battery.monitor_field_rechg_vol = true;

			/*Stop monitoring the temperature*/
			sec_bci.battery.monitor_field_temp = false;

			wake_lock_timeout( &sec_bc_wakelock , HZ );

			break;

		case POWER_SUPPLY_STATUS_CHARGING :
			/*Start monitoring the temperature*/
			sec_bci.battery.monitor_field_temp = true;

			/*Stop monitoring the batt. level for Re-charging*/
			sec_bci.battery.monitor_field_rechg_vol = false;

			break;

		case POWER_SUPPLY_STATUS_RECHARGING_FOR_FULL :

			//sec_bci.charger.charging_timeout = DEFAULT_RECHARGING_TIMEOUT;
			
			/*Start monitoring the temperature*/
			sec_bci.battery.monitor_field_temp = true;

			/*Stop monitoring the batt. level for Re-charging*/
			sec_bci.battery.monitor_field_rechg_vol = false;

			break;

		case POWER_SUPPLY_STATUS_RECHARGING_FOR_TEMP :
			/*Start monitoring the temperature*/
			sec_bci.battery.monitor_field_temp = true;

			/*Stop monitoring the batt. level for Re-charging*/
			sec_bci.battery.monitor_field_rechg_vol = false;

			break;

		default :
			break;
		}

	}
	else
	{


	}
    
	if( !is_sleep )
	{
		struct battery_device_info *di;
		struct platform_device *pdev;

		pdev = to_platform_device( this_dev );
		di = platform_get_drvdata( pdev );

		cancel_delayed_work( &di->battery_monitor_work );
		//schedule_delayed_work( &di->battery_monitor_work, 5 * HZ );
		queue_delayed_work( sec_bci.sec_battery_workq, &di->battery_monitor_work, 5 * HZ );	

		power_supply_changed( &di->sec_battery );
		power_supply_changed( &di->sec_ac );
		power_supply_changed( &di->sec_usb );

	}
	else
	{
		release_gptimer12( &batt_gptimer_12 );
		request_gptimer12( &batt_gptimer_12 );
	}

Out_Charger_State_Change :
	
    return 0;
}

int _low_battery_alarm_()
{
	struct battery_device_info *di;
	struct platform_device *pdev;
	int level;

	pdev = to_platform_device( this_dev );
	di = platform_get_drvdata( pdev );
	
	level = get_battery_level_ptg();
	if ( level == 1 )
		sec_bci.battery.battery_level_ptg = 0;
	else 
		sec_bci.battery.battery_level_ptg = level;

	wake_lock_timeout( &sec_bc_wakelock , HZ );
	power_supply_changed( &di->sec_battery );

	return 0;
}

int _get_average_value_( int *data, int count )
{
	int average;
	int min = 0;
	int max = 0;
	int sum = 0;
	int i;

	if ( count >= 5 )
	{
		min = max = data[0];
		for( i = 0; i < count; i++ )
		{
			if( data[i] < min )
				min = data[i];

			if( data[i] > max )
				max = data[i];

			sum += data[i];
		}

		average = ( sum - max - min ) / ( count - 2 );
	}
	else
	{
		for( i = 0; i < count; i++ )
			sum += data[i];

		average = sum / count;
	}

	return average;	
}

int _get_t2adc_data_( int ch )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	int ret = 0;
	int val[5];
	int i;
	//u8 ana_val;
	struct twl4030_madc_request req;

//	mutex_lock( &battery_lock );

//	msleep( 100 );

	wake_lock(&adc_wakelock);
	if ( ch >= 1 && ch <= 7 )
	{

		turn_resources_on_for_adc();

		//msleep(100);

		//twl4030_i2c_read_u8(TWL4030_MODULE_USB, &ana_val, CARKIT_ANA_CTRL);
		twl_i2c_write_u8( TWL4030_MODULE_USB, SEL_MADC_MCPC, CARKIT_ANA_CTRL );

		msleep(100);
	}

#if 0
	req.channels = ( 1 << ch );
	req.do_avg = 0;
	req.method = TWL4030_MADC_SW1;
	req.active = 0;
	req.func_cb = NULL;
	twl4030_madc_conversion( &req );

	ret = req.rbuf[ch];

#else

	req.channels = ( 1 << ch );
	req.do_avg = 0;
	req.method = TWL4030_MADC_SW1;
	req.active = 0;
	req.func_cb = NULL;
	
	for ( i = 0; i < 5 ; i++ )
	{
		twl4030_madc_conversion( &req );
		val[i] = req.rbuf[ch];
	}

	ret = _get_average_value_( val, 5 );

#endif

	if ( ch >= 1 && ch <= 7 )
	{
		//twl4030_i2c_write_u8( TWL4030_MODULE_USB, ana_val, CARKIT_ANA_CTRL ); // rechg egkim
		turn_resources_off_for_adc();
	}
	wake_unlock(&adc_wakelock);
//	mutex_unlock( &battery_lock );

	return ret;
}

int turn_resources_on_for_adc()
{
	int ret;
	u8 val;	
	
	struct battery_device_info *di;
	struct platform_device *pdev;

	pdev = to_platform_device( this_dev );
	di = platform_get_drvdata( pdev );

	ret = twl_i2c_read_u8( TWL4030_MODULE_MADC, &val, TWL4030_MADC_CTRL1 );
	val &= ~TWL4030_MADC_MADCON;
	ret = twl_i2c_write_u8( TWL4030_MODULE_MADC, val, TWL4030_MADC_CTRL1 );
	msleep( 10 );
	ret = twl_i2c_read_u8( TWL4030_MODULE_MADC, &val, TWL4030_MADC_CTRL1 );
	val |= TWL4030_MADC_MADCON;
	ret = twl_i2c_write_u8( TWL4030_MODULE_MADC, val, TWL4030_MADC_CTRL1 );

	ret = regulator_enable( di->usb3v1 );
	if ( ret )
		printk("Regulator 3v1 error!!\n");

	ret = regulator_enable( di->usb1v8 );
	if ( ret )
		printk("Regulator 1v8 error!!\n");

	ret = regulator_enable( di->usb1v5 );
	if ( ret )
		printk("Regulator 1v5 error!!\n");

	// !!!!!!!!!!!!!
	//twl4030_i2c_write_u8( TWL4030_MODULE_USB, 0, 0xFD/*PHY_PWR_CTRL*/ );
	/*
	 * TI CSR OMAPS00222879: "MP3 playback current consumption is very high"
	 * This a workaround to reduce the battery temperature.
	 */
	// FXIME twl_i2c_write_u8(TWL4030_MODULE_USB, 0, 0xFD/*PHY_PWR_CTRL*/);

#if 1
	twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x14, 0x7D/*VUSB_DEDICATED1*/ );
	twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x0, 0x7E/*VUSB_DEDICATED2*/ );
	twl_i2c_read_u8( TWL4030_MODULE_USB, &val, 0xFE/*PHY_CLK_CTRL*/ );
	val |= 0x1;
	twl_i2c_write_u8( TWL4030_MODULE_USB, val, 0xFE/*PHY_CLK_CTRL*/ );

	twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, VSEL_VINTANA2_2V75, TWL4030_VINTANA2_DEDICATED );
	twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x20, TWL4030_VINTANA2_DEV_GRP );
#endif
	return 0;
}

int turn_resources_off_for_adc()
{
	u8 val;	
	struct battery_device_info *di;
	struct platform_device *pdev;

	pdev = to_platform_device( this_dev );
	di = platform_get_drvdata( pdev );

	if ( sec_bci.charger.cable_status == POWER_SUPPLY_TYPE_BATTERY )
	{
#if 1
		// !!!!!!!!!!!!!
	//	twl4030_i2c_write_u8( TWL4030_MODULE_USB, 1, 0xFD/*PHY_PWR_CTRL*/ );
	
		//twl4030_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x14, 0x7D/*VUSB_DEDICATED1*/ );
		//twl4030_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x0, 0x7E/*VUSB_DEDICATED2*/ );
		twl_i2c_read_u8( TWL4030_MODULE_USB, &val, 0xFE/*PHY_CLK_CTRL*/ );
		val &= 0xFE;
		twl_i2c_write_u8( TWL4030_MODULE_USB, val, 0xFE/*PHY_CLK_CTRL*/ );
#endif
	}


	regulator_disable( di->usb1v5 );
	regulator_disable( di->usb1v8 );
	regulator_disable( di->usb3v1 );

	//twl4030_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0x00, VINTANA2_DEV_GRP);
	//twl4030_i2c_write_u8(TWL4030_MODULE_USB, ana_val, CARKIT_ANA_CTRL);
	//twl4030_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VINTANA2_REMAP );
	/*
	 * TI CSR OMAPS00222879: "MP3 playback current consumption is very high"
	 * This a workaround to reduce the battery temperature.
	 */
	// FIXME twl_i2c_write_u8(TWL4030_MODULE_USB, 1, 0xFD/*PHY_PWR_CTRL*/);

	return 0;
}

static int get_elapsed_time_secs( unsigned long long *start )
{
	unsigned long long now;
	unsigned long long diff;
	unsigned long long max = 0xFFFFFFFF;
	max = ( max << 32 ) | 0xFFFFFFFF;
	
	now = sched_clock();

	if ( now >= *start )
	{
		diff = now - *start;
	}
	else 
	{
		diff = max - *start + now;
	}

	do_div(diff, 1000000000L);

	printk( KERN_DEBUG "[BR] now: %llu, start: %llu, diff:%d\n",
		now, *start, (int)diff );

	return (int)diff;	
		
}

static int t2adc_to_temperature( int value, int channel )
{
	int mvolt, r;
	int temp;

	/*Caluclating voltage(adc) and resistance of thermistor */

	if( channel == 5 )
	{
		// vol = conv_result * step-size / R (TWLTRM Table 9-4)
		mvolt = value * 2500 / 1023; // mV
	}
	else
	{
		mvolt = 0;
	}

	//printk(" [TA] TEMP. adc: %d, mVolt: %d\n", adc, mvolt );

	// for ZEUS - VDD: 3000mV
	// Rt = ( 100K * Vadc  ) / ( VDD - 2 * Vadc )
	r = ( 100000 * mvolt ) / ( 3000 - 2 * mvolt );

	/*calculating temperature*/
	for( temp = 100; temp >= 0; temp-- )
	{
		if( ( NCP15WB473_batt_table[temp] - r ) >= 0 )
			break;
	}
	temp -= 15;
	temp=temp-1; // fixme for temperature test change -4 ==> -1

	return temp;
}

static int do_fuelgauge_reset( void )
{
	fuelgauge_quickstart();
	return 1;
}

static int get_charging_source( void )
{
/*
	android platform type.
	typedef enum {
	 CHARGER_BATTERY = 0,
	 CHARGER_USB,
	 CHARGER_AC,
	 CHARGER_DISCHARGE
	} charger_type_t;
*/
	
	if ( sec_bci.battery.battery_health != POWER_SUPPLY_HEALTH_GOOD )
		return 3;
	
	switch ( sec_bci.charger.cable_status )
	{
		case POWER_SUPPLY_TYPE_BATTERY :
			return 0;

		case POWER_SUPPLY_TYPE_MAINS :
			return 2;

		case POWER_SUPPLY_TYPE_USB : 
			return 1;
			
		default : 
			return 0;

	}

}

static int get_battery_level_adc( void )
// ----------------------------------------------------------------------------
// Description    : The result value is returned as a voltage level of cell.
// Input Argument :  
// Return Value   : 
{
	return get_fuelgauge_adc_value( 5, CHARGE_DUR_ACTIVE );
}

static int get_battery_level_ptg( void )
// ----------------------------------------------------------------------------
// Description    : The result value is returned as a percentage of cell's full 
//                  capacity.
// Input Argument :  
// Return Value   : 
{
	int value;

	value = get_fuelgauge_ptg_value( CHARGE_DUR_ACTIVE );

	if ( value < 0 )
		value=sec_bci.battery.battery_level_ptg;

	return value;
}

static void update_battery_level_n_go( struct battery_device_info *di )
// ----------------------------------------------------------------------------
// Description    :
// Input Argument :  
// Return Value   : 
{
	sec_bci.battery.battery_level_ptg = get_battery_level_ptg();

	power_supply_changed( &di->sec_battery );

	queue_delayed_work( sec_bci.sec_battery_workq, 
				&di->battery_monitor_work, 
				4 * HZ);
}

static int get_system_temperature( bool flag )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	int adc;
	int temp;
	
	adc = _get_t2adc_data_( device_config->TEMP_ADC_PORT );

	//printk("[BR] TEMP ADC: %d\n", adc);

	if( flag )
		return adc;

	temp = t2adc_to_temperature( adc, device_config->TEMP_ADC_PORT );


#if 0
	/*get voltage(adc) from t2*/

	adc = _get_t2adc_data_( 12 );

	// vol = conv_result * step-size / R (TWLTRM Table 9-4)
	mvolt = adc * 6000 / 1023; // mV

	printk( "\n ## [BCI] VOL : %d\n\n", mvolt );
    
#endif
	return temp;
}

static int get_charging_current_adc_val( void )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	int adc;
	
	adc = _get_t2adc_data_( device_config->CHG_CURRENT_ADC_PORT );
	
	//printk("                     [BR] CHG CURRENT ADC: %d, VOL: %d\n\n ", adc, (adc * 2500 / 1023) );
	
	return adc;
}

static int check_full_charge_using_chg_current( int charge_current_adc )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   :
{

	if ( sec_bci.battery.battery_level_vol < 4000 )
	{
		sec_bci.battery.confirm_full_by_current = 0;
		return 0;
	}

	/*
	// change the frequency of monitoring
	if ( sec_bci.battery.confirm_changing_freq < 5 )
	{
		int aa;
		aa = CHARGE_FULL_CURRENT_ADC * 1.1;
		
		if ( charge_current_adc <= aa )
			sec_bci.battery.confirm_changing_freq++;
		else
			sec_bci.battery.confirm_changing_freq = 0;
	}
	else
	{
		batt_gptimer_12.expire_time = 5; // sec
		sec_bci.battery.monitor_duration = 5; // sec
	}
	*/

	if (sec_bci.battery.support_monitor_full)
	{
		if ( charge_current_adc <= CHARGE_FULL_CURRENT_ADC )
		{
			sec_bci.battery.confirm_full_by_current++;

			// changing freq. of monitoring adc to Burst.
			batt_gptimer_12.expire_time = 5; // sec
			sec_bci.battery.monitor_duration = 5; // sec		
		}
		else
		{
			sec_bci.battery.confirm_full_by_current = 0;
			// changing freq. of monitoring adc to Default.
			batt_gptimer_12.expire_time = MONITOR_DURATION_DUR_SLEEP; // sec
			sec_bci.battery.monitor_duration = MONITOR_DEFAULT_DURATION; // sec	
		}

		if ( sec_bci.battery.confirm_full_by_current >= 4 )
		{
			batt_gptimer_12.expire_time = MONITOR_DURATION_DUR_SLEEP;
			sec_bci.battery.monitor_duration = MONITOR_DEFAULT_DURATION;
			sec_bci.battery.confirm_full_by_current = 0;
				//sec_bci.battery.confirm_changing_freq = 0;
			return 1;
		}	
	}
	return 0; 
}

static void get_system_status_in_sleep( int *battery_level_ptg, 
					int *battery_level_vol, 
					int *battery_temp, 
					int *charge_current_adc )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   :
{
	int temp_adc;
	int ptg_val;

	twl4030_i2c_init();
	normal_i2c_init();

	ptg_val = get_fuelgauge_ptg_value( CHARGE_DUR_SLEEP );

	if ( ptg_val >= 0 )
	{
		*battery_level_ptg = ptg_val;
	}

	*battery_level_vol = get_fuelgauge_adc_value( 5, CHARGE_DUR_SLEEP );

	temp_adc = t2_adc_data( device_config->TEMP_ADC_PORT );

	if ( device_config->MONITORING_CHG_CURRENT )
		*charge_current_adc = t2_adc_data ( device_config->CHG_CURRENT_ADC_PORT );

	normal_i2c_disinit();
	twl4030_i2c_disinit();

	//printk("\n<BR> TEMP ADC: %d\n", adc);
	//printk("                     <BR> CHG CURRENT ADC: %d, VOL: %d\n\n ", *charge_current_adc, (*charge_current_adc * 2500 / 1023) );

	*battery_temp = t2adc_to_temperature( temp_adc, device_config->TEMP_ADC_PORT );	

//	mdelay(1000);
}

static int battery_monitor_core( bool is_sleep )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   :
{

	int charging_time;
	
	/*Monitoring the system temperature*/
	if ( sec_bci.battery.monitor_field_temp )
	{

		if ( sec_bci.battery.support_monitor_timeout )
		{
			/*Check charging time*/
			charging_time = get_elapsed_time_secs( &sec_bci.charger.charge_start_time );
			if ( charging_time >= sec_bci.charger.charging_timeout )
			{
				if ( is_sleep ) 
					sec_bci.charger.full_charge_dur_sleep = 0x2;
				else
					_battery_state_change_( STATUS_CATEGORY_CHARGING, 
								POWER_SUPPLY_STATUS_CHARGING_OVERTIME, 
								is_sleep );

				return -1;
			}
		}
		if ( sec_bci.battery.support_monitor_temp )
		{
			if ( sec_bci.battery.battery_health == POWER_SUPPLY_HEALTH_OVERHEAT 
				|| sec_bci.battery.battery_health == POWER_SUPPLY_HEALTH_COLD )
			{
				if ( sec_bci.battery.battery_temp <= CHARGE_RECOVER_TEMPERATURE_MAX 
					&& sec_bci.battery.battery_temp >= CHARGE_RECOVER_TEMPERATURE_MIN )
				{
					sec_bci.battery.battery_health = POWER_SUPPLY_HEALTH_GOOD;
					_battery_state_change_( STATUS_CATEGORY_TEMP, 
								BATTERY_TEMPERATURE_NORMAL, 
								is_sleep );
				}

			}
			else
			{
				if ( sec_bci.battery.monitor_duration > MONITOR_TEMP_DURATION )
					sec_bci.battery.monitor_duration = MONITOR_TEMP_DURATION;

				if ( sec_bci.battery.battery_temp >= CHARGE_STOP_TEMPERATURE_MAX )
				{
					if ( sec_bci.battery.battery_health != POWER_SUPPLY_HEALTH_OVERHEAT )
					{
						sec_bci.battery.battery_health = POWER_SUPPLY_HEALTH_OVERHEAT;

						_battery_state_change_( STATUS_CATEGORY_TEMP, 
									BATTERY_TEMPERATURE_HIGH, 
									is_sleep );
					}
				}
				else if ( sec_bci.battery.battery_temp <= CHARGE_STOP_TEMPERATURE_MIN )
				{
					if ( sec_bci.battery.battery_health != POWER_SUPPLY_HEALTH_COLD )
					{
						sec_bci.battery.battery_health = POWER_SUPPLY_HEALTH_COLD;

						_battery_state_change_( STATUS_CATEGORY_TEMP, 
									BATTERY_TEMPERATURE_LOW, 
									is_sleep );
					}
				}
				else
				{
					if ( sec_bci.battery.battery_health != POWER_SUPPLY_HEALTH_GOOD )
					{
						sec_bci.battery.battery_health = POWER_SUPPLY_HEALTH_GOOD;
						_battery_state_change_( STATUS_CATEGORY_TEMP, 
									BATTERY_TEMPERATURE_NORMAL, 
									is_sleep );
					}
				}
			}
		}
	}

	/*Monitoring the battery level for Re-charging*/
	if ( sec_bci.battery.monitor_field_rechg_vol )
	{
		if ( sec_bci.battery.monitor_duration > MONITOR_RECHG_VOL_DURATION )
			sec_bci.battery.monitor_duration = MONITOR_RECHG_VOL_DURATION;

		if ( sec_bci.battery.battery_level_vol <= 4120 )
		{
			sec_bci.battery.confirm_recharge++;
			if ( sec_bci.battery.confirm_recharge >= 2 )
			{
				printk( "[TA] RE-charging vol\n" );
				sec_bci.battery.confirm_recharge = 0;	
				_battery_state_change_( STATUS_CATEGORY_CHARGING, 
							POWER_SUPPLY_STATUS_RECHARGING_FOR_FULL, 
							is_sleep );
			}

		}
		else
			sec_bci.battery.confirm_recharge = 0;
		
		

	}

	return 0;
	
}

//extern int resource_get_level(const char *name);
static void battery_monitor_work_handler( struct work_struct *work )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   :
{
	int is_full = 0;
	int charge_current_adc;
	struct battery_device_info *di = container_of( work,
							struct battery_device_info,
							battery_monitor_work.work );
	//printk("OPP: %d\n", resource_get_level("vdd1_opp"));
	//printk( KERN_DEBUG "[BR] battery_monitor_work\n" );
#if 0
    /*
	printk( "[BR] battery monitor [Level:%d, ADC:%d, TEMP.:%d, cable: %d] \n",\
		get_battery_level_ptg(),\
		get_battery_level_adc(),\
		get_system_temperature(),\
		sec_bci.charger.cable_status );*/
#endif

	//printk("VF: %d\n", _get_t2adc_data_(1));


	/*Monitoring the battery info.*/
	sec_bci.battery.battery_level_ptg = get_battery_level_ptg();
	sec_bci.battery.battery_level_vol= get_battery_level_adc();
	
	if ( device_config->MONITORING_SYSTEM_TEMP )
		sec_bci.battery.battery_temp = get_system_temperature( TEMP_DEG );
	else
		sec_bci.battery.battery_temp = 0;


	if( !( sec_bci.battery.monitor_field_temp ) 
	 	&& !( sec_bci.battery.monitor_field_rechg_vol ) )
	{
		sec_bci.battery.monitor_duration = MONITOR_DEFAULT_DURATION;
	}
	else
	{
		// Workaround : check status of cabel at this point.
		if ( !_cable_status_now_() )
		{
			_battery_state_change_( STATUS_CATEGORY_ETC, 
						ETC_CABLE_IS_DISCONNECTED, 
						CHARGE_DUR_ACTIVE );

			return;
		}

		if ( sec_bci.charger.is_charging && device_config->MONITORING_CHG_CURRENT )
		// in charging && enable monitor_chg_current
		{
			charge_current_adc = get_charging_current_adc_val();
			is_full = check_full_charge_using_chg_current( charge_current_adc );

			if ( is_full )
			{
//				do_fuelgauge_reset();
				_battery_state_change_( STATUS_CATEGORY_CHARGING, 
							POWER_SUPPLY_STATUS_FULL, 
							CHARGE_DUR_ACTIVE );
			}
			else
				battery_monitor_core( CHARGE_DUR_ACTIVE );
		}
		else
		{
			battery_monitor_core( CHARGE_DUR_ACTIVE );
		}
			
	}


#if 0 
	printk( "[BR] monitor BATT.(%d%%, %dmV, %d*)\n", 
			sec_bci.battery.battery_level_ptg,
			sec_bci.battery.battery_level_vol,
			sec_bci.battery.battery_temp );
#endif

	power_supply_changed( &di->sec_battery );
	power_supply_changed( &di->sec_ac );
	power_supply_changed( &di->sec_usb );


	//schedule_delayed_work( &di->battery_monitor_work, 
				//sec_bci.battery.monitor_duration * HZ );

	cancel_delayed_work( &di->battery_monitor_work );
	queue_delayed_work( sec_bci.sec_battery_workq, 
				&di->battery_monitor_work, 
				sec_bci.battery.monitor_duration * HZ);


}

static int battery_monitor_fleeting_wakeup_handler( unsigned long arg ) 
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   :
{
	int ret = 0;
	int is_full = 0;
	int charge_current_adc = 0;

	//struct battery_device_info *di = (struct battery_device_info *) arg;

	//printk("? %s\n", di->dev->kobj.name);
	//printk("opp: %d\n", resource_get_level("vdd1_opp"));
	
	get_system_status_in_sleep( &sec_bci.battery.battery_level_ptg,
				&sec_bci.battery.battery_level_vol,
				&sec_bci.battery.battery_temp, 
				&charge_current_adc );

	if ( sec_bci.charger.is_charging )
	{
		if ( device_config->MONITORING_CHG_CURRENT )
			is_full = check_full_charge_using_chg_current( charge_current_adc );
		else
			is_full = _check_full_charge_dur_sleep_();
	}


	if ( is_full )
	{
		sec_bci.charger.full_charge_dur_sleep = 0x1;
		ret = -1;
	}
	else
	{
		sec_bci.charger.full_charge_dur_sleep = 0x0;
		ret = battery_monitor_core( CHARGE_DUR_SLEEP );
	}

	if ( ret >= 0 )
		request_gptimer12( &batt_gptimer_12 );

#if 0
	debug_fleeting_counter++;
	printk( "<br> monitor BATT.(%d%%, %dmV, %d*) - %d\n", 
	sec_bci.battery.battery_level_ptg,
	sec_bci.battery.battery_level_vol,
	sec_bci.battery.battery_temp,
	debug_fleeting_counter );
#endif

	return ret;

}

// ------------------------------------------------------------------------- // 
//                                                                           //
//                            Power supply monitor                           //
//                                                                           //
// ------------------------------------------------------------------------- // 

static enum power_supply_property samsung_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY, // in percents
	POWER_SUPPLY_PROP_TEMP,
};

static enum power_supply_property samsung_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static enum power_supply_property samsung_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static int samsung_battery_get_property( struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val )
{

	switch ( psp ) 
	{
	case POWER_SUPPLY_PROP_STATUS:
		if( sec_bci.charger.charge_status == POWER_SUPPLY_STATUS_RECHARGING_FOR_FULL 
			|| sec_bci.charger.charge_status == POWER_SUPPLY_STATUS_RECHARGING_FOR_TEMP )
			val->intval = POWER_SUPPLY_STATUS_CHARGING;
		else
			val->intval = sec_bci.charger.charge_status;

		break;

	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = sec_bci.battery.battery_health;
		break;

	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = sec_bci.battery.battery_level_vol * 1000;
		val->intval = val->intval <= 0 ? 0 : 1;
		break;

	case POWER_SUPPLY_PROP_ONLINE :
		val->intval = sec_bci.charger.cable_status;
		break;

	case POWER_SUPPLY_PROP_TECHNOLOGY :
		val->intval = sec_bci.battery.battery_technology;
		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW :
		val->intval = sec_bci.battery.battery_level_vol * 1000;
		break;

	case POWER_SUPPLY_PROP_CAPACITY : /* in percents! */
		val->intval = sec_bci.battery.battery_level_ptg;
		break;

	case POWER_SUPPLY_PROP_TEMP :
		val->intval = sec_bci.battery.battery_temp * 10;
		break;

	default :
		return -EINVAL;
	}

	//printk(" [GET] %d, %d  !!! \n", psp, val->intval );

	return 0;
}

static int samsung_ac_get_property( struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val )
{

	switch ( psp ) 
	{        
	case POWER_SUPPLY_PROP_ONLINE :
		if ( sec_bci.charger.cable_status == POWER_SUPPLY_TYPE_MAINS )
			val->intval = 1;
		else 
			val->intval = 0;

		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW :
		val->intval = sec_bci.battery.battery_level_vol * 1000;
		break;

	default :
			return -EINVAL;
	}

	return 0;
}

static int samsung_usb_get_property( struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val )
{

	switch ( psp ) 
	{        
	case POWER_SUPPLY_PROP_ONLINE :
		if ( sec_bci.charger.cable_status == POWER_SUPPLY_TYPE_USB )
			val->intval = 1;
		else 
			val->intval = 0;

		break;

	case POWER_SUPPLY_PROP_VOLTAGE_NOW :
		val->intval = sec_bci.battery.battery_level_vol * 1000;
		break;

	default :
		return -EINVAL;
	}


	return 0;
}

static char *samsung_bci_supplied_to[] = {
	"battery",
};

static void samsung_pwr_external_power_changed( struct power_supply *psy ) 
{
	//cancel_delayed_work(&di->twl4030_bci_monitor_work);
	//schedule_delayed_work(&di->twl4030_bci_monitor_work, 0);
}


// ------------------------------------------------------------------------- // 
//                                                                           //
//                           Driver interface                                //
//                                                                           //
// ------------------------------------------------------------------------- // 

static int __devinit battery_probe( struct platform_device *pdev )
// ----------------------------------------------------------------------------
// Description    : probe function for battery driver.
// Input Argument :  
// Return Value   : 
{
	int ret = 0;
	int i = 0;
	int Debug_Usepopup = 1;

	struct battery_device_info *di;
	
	printk( "[BR] Battery Probe...\n\n" );

	this_dev = &pdev->dev; 

	di = kzalloc( sizeof(*di), GFP_KERNEL );
	if(!di)
		return -ENOMEM;

	platform_set_drvdata( pdev, di );
	
	di->dev = &pdev->dev;
	device_config = pdev->dev.platform_data;

	INIT_DELAYED_WORK( &di->battery_monitor_work, battery_monitor_work_handler );

// [ USE_REGULATOR
	di->usb3v1 = regulator_get( &pdev->dev, "usb3v1" );
	if( IS_ERR( di->usb3v1 ) )
		goto fail_regulator1;

	di->usb1v8 = regulator_get( &pdev->dev, "usb1v8" );
	if( IS_ERR( di->usb1v8 ) )
		goto fail_regulator2;

	di->usb1v5 = regulator_get( &pdev->dev, "usb1v5" );
	if( IS_ERR( di->usb1v5 ) )
		goto fail_regulator3;
// ]

	/*Create power supplies*/
	di->sec_battery.name = "battery";
	di->sec_battery.type = POWER_SUPPLY_TYPE_BATTERY;
	di->sec_battery.properties = samsung_battery_props;
	di->sec_battery.num_properties = ARRAY_SIZE( samsung_battery_props );
	di->sec_battery.get_property = samsung_battery_get_property;
	di->sec_battery.external_power_changed = 
	samsung_pwr_external_power_changed;
	//di->sec_battery.use_for_apm = 1;

	di->sec_ac.name = "ac";
	di->sec_ac.type = POWER_SUPPLY_TYPE_MAINS;
	di->sec_ac.supplied_to = samsung_bci_supplied_to;
	di->sec_ac.num_supplicants = ARRAY_SIZE( samsung_bci_supplied_to );
	di->sec_ac.properties = samsung_ac_props;
	di->sec_ac.num_properties = ARRAY_SIZE( samsung_ac_props );
	di->sec_ac.get_property = samsung_ac_get_property;
	di->sec_ac.external_power_changed = 
	samsung_pwr_external_power_changed;

	di->sec_usb.name = "usb";
	di->sec_usb.type = POWER_SUPPLY_TYPE_USB;
	di->sec_usb.supplied_to = samsung_bci_supplied_to;
	di->sec_usb.num_supplicants = ARRAY_SIZE( samsung_bci_supplied_to );
	di->sec_usb.properties = samsung_usb_props;
	di->sec_usb.num_properties = ARRAY_SIZE( samsung_usb_props );
	di->sec_usb.get_property = samsung_usb_get_property;
	di->sec_usb.external_power_changed = 
	samsung_pwr_external_power_changed;

	ret = power_supply_register( &pdev->dev, &di->sec_battery );
	if( ret )
	{
		printk( "failed to register main battery, charger\n" );
		goto batt_regi_fail1;
	}

	ret = power_supply_register( &pdev->dev, &di->sec_ac );
	if( ret )
	{
		printk( "failed to register ac\n" );
		goto batt_regi_fail2;
	}

	ret = power_supply_register( &pdev->dev, &di->sec_usb );
	if( ret )
	{
		printk( "failed to register usb\n" );
		goto batt_regi_fail3;
	}

	ret = sysfs_create_file( &di->sec_battery.dev->kobj, 
				 &batt_vol_toolow.attr );
	if ( ret )
	{
		printk( "sysfs create fail - %s\n", batt_vol_toolow.attr.name );
	}

	ret = sysfs_create_file( &di->sec_battery.dev->kobj, 
				 &charging_source.attr );
	if ( ret )
	{
		printk( "sysfs create fail - %s\n", charging_source.attr.name );
	}

	for( i = 0; i < ARRAY_SIZE( batt_sysfs_testmode ); i++ )
	{
		ret = sysfs_create_file( &di->sec_battery.dev->kobj, 
					 &batt_sysfs_testmode[i].attr );
		if ( ret )
		{
			printk( "sysfs create fail - %s\n", batt_sysfs_testmode[i].attr.name );
		}
	}

	// Init. ADC
	turn_resources_on_for_adc();
	twl_i2c_write_u8( TWL4030_MODULE_USB, SEL_MADC_MCPC, CARKIT_ANA_CTRL );
	turn_resources_off_for_adc();

	batt_gptimer_12.name = "samsung_battery_timer";
	batt_gptimer_12.expire_time =(unsigned int) MONITOR_DURATION_DUR_SLEEP;
	batt_gptimer_12.expire_callback = &battery_monitor_fleeting_wakeup_handler;
	batt_gptimer_12.data = (unsigned long) di;
#if 0 //r3d4
	if ( sec_get_param_value )
	{
		sec_get_param_value(__DEBUG_BLOCKPOPUP, &Debug_Usepopup);
	}

	if ( (Debug_Usepopup & 0x1) == 1 )
	{
		sec_bci.battery.support_monitor_temp = 1;
		sec_bci.battery.support_monitor_timeout = 1;
		sec_bci.battery.support_monitor_full = 1;
	}
	else if ( (Debug_Usepopup & 0x1) == 0 )
	{
		sec_bci.battery.support_monitor_temp = 0;
		sec_bci.battery.support_monitor_timeout = 0;
		sec_bci.battery.support_monitor_full = 0;
	}
#endif
	//schedule_delayed_work( &di->battery_monitor_work, 3*HZ );
	queue_delayed_work( sec_bci.sec_battery_workq, &di->battery_monitor_work, 3*HZ );

	// ready to go!!
	sec_bci.ready = true;

	return 0;

batt_regi_fail3:
	power_supply_unregister( &di->sec_ac );

batt_regi_fail2:
	power_supply_unregister( &di->sec_battery );

batt_regi_fail1:
// [ USE_REGULATOR
	regulator_put( di->usb1v5 );
	di->usb1v5 = NULL;

fail_regulator3:
	regulator_put( di->usb1v8 );
	di->usb1v8 = NULL;

fail_regulator2:
	regulator_put( di->usb3v1 );
	di->usb3v1 = NULL;

fail_regulator1:
// ]
	kfree(di);

	return ret;
}

static int __devexit battery_remove( struct platform_device *pdev )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument : 
// Return Value   : 
{
	struct battery_device_info *di = platform_get_drvdata( pdev );

	flush_scheduled_work();
	cancel_delayed_work( &di->battery_monitor_work );

	power_supply_unregister( &di->sec_ac );
	power_supply_unregister( &di->sec_battery );

// [ USE_REGULATOR
	regulator_put( di->usb1v5 );
	regulator_put( di->usb1v8 );
	regulator_put( di->usb3v1 );
// ]

	platform_set_drvdata( pdev, NULL );

	kfree( di );

	return 0;
}

static int battery_suspend( struct platform_device *pdev,
                            pm_message_t state )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument : 
// Return Value   : 
{
	struct battery_device_info *di = platform_get_drvdata( pdev );

	cancel_delayed_work( &di->battery_monitor_work );
	flush_delayed_work( &di->battery_monitor_work );

/*
	if ( sec_bci.charger.cable_status == POWER_SUPPLY_TYPE_BATTERY )
		twl4030_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0x00, TWL4030_VINTANA2_DEV_GRP);
*/ 

	if( sec_bci.charger.cable_status == POWER_SUPPLY_TYPE_MAINS )
	{
#if 0
		struct file *filp;
		char buf;
		int count;
		int retval = 0;
		mm_segment_t fs;

		fs = get_fs();
		set_fs(KERNEL_DS);
		filp = filp_open("/sys/power/vdd1_lock", 
				00000001/*O_WRONLY*/|00010000/*O_SYNC*/, 
				0x0);
		buf='1';
		count=filp->f_op->write(filp, &buf, 1, &filp->f_pos);
		retval = filp_close(filp, NULL);
		set_fs(fs);
#endif
		request_gptimer12( &batt_gptimer_12 );

	}



	return 0;
}

static int battery_resume( struct platform_device *pdev )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument : 
// Return Value   : 
{
	struct battery_device_info *di = platform_get_drvdata( pdev );

	if ( batt_gptimer_12.active )
	{
#if 0
		struct file *filp;
		char buf;
		int count;
		int retval = 0;
		mm_segment_t fs;

		fs = get_fs();
		set_fs(KERNEL_DS);
		filp = filp_open("/sys/power/vdd1_lock", 
				00000001/*O_WRONLY*/|00010000/*O_SYNC*/, 
				0x0);
		buf='0';
		count=filp->f_op->write(filp, &buf, 1, &filp->f_pos);
		retval = filp_close(filp, NULL);
		set_fs(fs);
#endif
		release_gptimer12( &batt_gptimer_12 );
	}


	if ( sec_bci.charger.full_charge_dur_sleep )
		wake_lock_timeout( &sec_bc_wakelock , HZ );			
	
	switch ( sec_bci.charger.full_charge_dur_sleep )
	{
	case 0x1 : 
//		do_fuelgauge_reset();
		_battery_state_change_( STATUS_CATEGORY_CHARGING, 
					POWER_SUPPLY_STATUS_FULL_DUR_SLEEP, 
					CHARGE_DUR_ACTIVE );
		break;

	case 0x2 : 
		_battery_state_change_( STATUS_CATEGORY_CHARGING, 
					POWER_SUPPLY_STATUS_CHARGING_OVERTIME, 
					CHARGE_DUR_ACTIVE );
		break;

	default : 
		;
	}


#if 0
	power_supply_changed( &di->sec_battery );
	power_supply_changed( &di->sec_ac );
	power_supply_changed( &di->sec_usb );
#endif

	sec_bci.charger.full_charge_dur_sleep = 0x0;

	// update battery status bar & schedule the battery work
	update_battery_level_n_go( di );

	//schedule_delayed_work( &di->battery_monitor_work, HZ );
	//queue_delayed_work( sec_bci.sec_battery_workq, &di->battery_monitor_work, 1*HZ );

	return 0;
}

struct platform_driver battery_platform_driver = {
	.probe		= battery_probe,
	.remove		= __devexit_p( battery_remove ),
	.suspend	= &battery_suspend,
	.resume		= &battery_resume,
	.driver		= {
		.name = DRIVER_NAME,
	},
};

//static struct platform_device * battery_platform_device;


static int __init battery_init( void )
// ----------------------------------------------------------------------------
// Description    : init fuction.
// Input Argument : none. 
// Return Value   :
{
	int ret;

	printk( "\n[BR] Battery Init.\n" );

	sec_bci.ready = false;
	sec_bci.battery.battery_health = POWER_SUPPLY_HEALTH_GOOD;
	sec_bci.battery.battery_technology = POWER_SUPPLY_TECHNOLOGY_LION;
	sec_bci.battery.battery_level_ptg = 0;
	sec_bci.battery.battery_level_vol = 0;
	sec_bci.battery.monitor_duration = MONITOR_DEFAULT_DURATION;
	sec_bci.battery.monitor_field_temp = false;
	sec_bci.battery.monitor_field_rechg_vol = false;
	sec_bci.battery.confirm_full_by_current = 0;
	sec_bci.battery.support_monitor_temp = 1;
	sec_bci.battery.support_monitor_timeout = 1;
	sec_bci.battery.support_monitor_full = 1;
		//sec_bci.battery.confirm_changing_freq = 0;
	sec_bci.battery.confirm_recharge = 0;
	sec_bci.charger.prev_cable_status = -1;
	sec_bci.charger.cable_status = -1;
	sec_bci.charger.prev_charge_status = 0;
	sec_bci.charger.charge_status = 0;
	//sec_bci.charger.full_charge = false;
	sec_bci.charger.full_charge_dur_sleep = 0x0;
	sec_bci.charger.is_charging = false;
	sec_bci.charger.charge_start_time = 0;
	sec_bci.charger.charged_time = 0;
	sec_bci.charger.charging_timeout = DEFAULT_CHARGING_TIMEOUT; // 5 hours
	sec_bci.charger.use_ta_nconnected_irq = false;

	sec_bci.sec_battery_workq = create_singlethread_workqueue("sec_battery_workq");

	init_gptimer12();

	printk( "[BR] Init Gptimer called \n" );

	/*Get the charger driver*/
	if( ( ret = charger_init() < 0 ) )
	{
		printk( "[BR] fail to get charger driver.\n" );
		return ret;
	}

	/*Get the fuelgauge driver*/
	if( ( ret = fuelgauge_init() < 0 ) )
	{
		printk( "[BR] fail to get fuelgauge driver.\n" );        
		return ret;
	}

	wake_lock_init( &sec_bc_wakelock, WAKE_LOCK_SUSPEND, "samsung-battery" );
	wake_lock_init( &adc_wakelock, WAKE_LOCK_SUSPEND, "adc-battery" );

	ret = platform_driver_register( &battery_platform_driver );

	return ret;
}
module_init( battery_init );

static void __exit battery_exit( void )
// ----------------------------------------------------------------------------
// Description    : exit fuction.
// Input Argument : none.
// Return Value   : none. 
{
	/*Remove the charger driver*/
	charger_exit();
	/*Remove the fuelgauge driver*/
	fuelgauge_exit();

	finish_gptimer12();
	
	platform_driver_unregister( &battery_platform_driver );
	printk( KERN_ALERT "Battery Driver Exit.\n" );
}

module_exit( battery_exit );

MODULE_AUTHOR( "EUNGON KIM <egstyle.kim@samsung.com>" );
MODULE_DESCRIPTION( "Samsung Battery monitor driver" );
MODULE_LICENSE( "GPL" );


