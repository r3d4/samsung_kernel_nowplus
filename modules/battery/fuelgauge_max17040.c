/*
 * module/samsung_battery/fuelgauge_max17040.c
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
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <mach/gpio.h>
#include <linux/power_supply.h>
#include "common.h"

#define DRIVER_NAME "secFuelgaugeDev"

#define I2C_M_WR    0x00

#define REG_VCELL   0x2
#define REG_SOC     0x4
#define REG_MODE    0x6
#define REG_VERSION 0x8
#define REG_RCOMP   0xC
#define REG_CONFIG  0xD
#define REG_COMMAND 0xFE

static struct i2c_client * fuelgauge_i2c_client;

struct delayed_work fuelgauge_work;

#if defined(CONFIG_MACH_HALO)
unsigned int startingRcomp;
#endif

static SEC_battery_charger_info *sec_bci;

// Prototype
       int get_fuelgauge_adc_value( int, bool );
       int get_fuelgauge_ptg_value( bool );
       int fuelgauge_quickstart( void );
static int get_average_value( int *, int );
static int i2c_read( unsigned char );
static int i2c_write( unsigned char *, u8 );
static int i2c_read_dur_sleep( unsigned char );
static irqreturn_t low_battery_isr( int, void * );
static void fuelgauge_work_handler( struct work_struct * );
static int fuelgauge_probe( struct i2c_client *, const struct i2c_device_id * );
static int fuelgauge_remove( struct i2c_client * );
static void fuelgauge_shutdown( struct i2c_client * );
static int fuelgauge_suspend( struct i2c_client * , pm_message_t );
static int fuelgauge_resume( struct i2c_client * );
       int fuelgauge_init( void );
       void fuelgauge_exit( void );

extern SEC_battery_charger_info *get_sec_bci( void );
extern int _low_battery_alarm_( void );
extern s32 normal_i2c_read_word( u8 , u8 , u8 * );

int get_fuelgauge_adc_value( int count, bool is_sleep )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	int val[count];
	int result;
	int i;

#if 0	
	for ( i = 0; i < count ; i++ )
	{
		if( is_sleep )
			val[i] = i2c_read_dur_sleep( REG_VCELL );
		else
			val[i] = i2c_read( REG_VCELL );

		// resoulution 1.25mV / 1step
		val[i] = ( val[i] >> 4 ) * 125 / 100;
	}

	result = get_average_value( val, count );
#else
	if( is_sleep )
		result = i2c_read_dur_sleep( REG_VCELL );
	else
		result = i2c_read( REG_VCELL );
	result = ( result >> 4 ) * 125 / 100;
#endif

	return result;
}

int get_fuelgauge_ptg_value( bool is_sleep )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	int val;
	int additional_val = 0;

	if( is_sleep )
		val = i2c_read_dur_sleep( REG_SOC ); 
	else
		val = i2c_read( REG_SOC );

#if defined(CONFIG_MACH_HALO)
	if(val == -1){
		val = 100;
		goto i2c_fail;
	}
	printk("[FG] get_fuelgauge_ptg_value soc1 : %d, soc2 : %d\n", (val >> 8), (val & 0xFF));
	val=(((val >> 8)*256)+(val & 0xFF)) * 5 / 256; // 0.001953125
	if((val % 10) > 0)
		val+=10;
	val /= 10;
#else
	if ( val == -1 ) 
	{
		return -1;
	}
	if ((val & 0x0F) >= 128)
	{
		val = (val >> 8) + 1;
	}
	else
	{
		val = val >> 8;
	}
	val = ((val - 1)*1000)/952;

// battery compentation for oscar
	if(val < 41)
		val = val + (val/6);
	else if(val < 56)
		val = val + (val/6) - (val-43);
	else if(val < 73)
		val = val + (val/6) -12;
	
#endif
	if(val < 0)
		val = 0;
#if defined(CONFIG_MACH_HALO)
i2c_fail:
#endif

	return ( val > 100 ) ? 100 : val;
}

int fuelgauge_quickstart( void )
{
	//printk( KERN_ERR "[FG] Quickstart read dodododoodo" );
	unsigned char buf[3];
	buf[0] = REG_MODE;
	buf[1] = 0x40;
	buf[2] = 0x00;
	i2c_write( buf, 3 );

	return 0;
}

#if defined(CONFIG_MACH_HALO)
int update_rcomp_by_temperature(int temp)
{
	static unsigned int appliedRcomp = 0;
	int tempCoHot = -75;
	int tempCoCold = -75;
	unsigned int newRcomp = 0;
	int ret = 0;
	unsigned char buf[3];

	if(temp > 20)
		newRcomp = startingRcomp + (((temp - 20) * tempCoHot)/100);
	else if(temp < 20)
		newRcomp = startingRcomp + (((temp - 20) * tempCoCold)/100);
	else
		newRcomp = startingRcomp;

	printk("%s, startingRcomp=0x%x, appliedRcomp=0x%x, newRcomp=0x%x \n", __func__, startingRcomp, appliedRcomp, newRcomp);
	if(newRcomp != appliedRcomp)
	{
		ret = i2c_read( REG_RCOMP );
		buf[0] = REG_RCOMP;
		buf[1] = newRcomp;
		buf[2] = ret & 0xFF;
		i2c_write( buf, 3 );
		appliedRcomp = newRcomp;
		printk("%s, buf[0]=0x%x, buf[1]=0x%x, buf[2]=0x%x\n", __func__, buf[0], buf[1], buf[2]);
		return 1;
	}
	return 0;
}
#endif

static int get_average_value( int *data, int count )
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

static int i2c_read( unsigned char reg_addr )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	int ret = 0;
	struct i2c_msg msg1[1],msg2[1];
	unsigned char buf[2];

	msg1->addr = fuelgauge_i2c_client->addr;
	msg1->flags = I2C_M_WR;
	msg1->len = 1;
	msg1->buf = &reg_addr;

	ret = i2c_transfer(fuelgauge_i2c_client->adapter, msg1, 1);
    if( ret < 0 )
    {
        printk( KERN_ERR "[FG] fail to read max17040." );
		return -1;
    }
	else
	{
		msg2->addr = fuelgauge_i2c_client->addr;
		msg2->flags = I2C_M_RD;
		msg2->len = 2;
		msg2->buf = buf;

		ret = i2c_transfer( fuelgauge_i2c_client->adapter, msg2, 1 );

		if( ret < 0 )
		{
			printk( KERN_ERR "[FG] fail to read max17040." );
			return -1;
		}
	}

	ret = buf[0] << 8 | buf[1];

	return ret;
}

static int i2c_write( unsigned char *buf, u8 len )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	int ret = 0;
	struct i2c_msg msg;

	msg.addr	= fuelgauge_i2c_client->addr;
	msg.flags = I2C_M_WR;
	msg.len = len;
	msg.buf = buf;

	ret = i2c_transfer( fuelgauge_i2c_client->adapter, &msg, 1 );

	if( ret < 0 )
	{
		printk( KERN_ERR "[FG] fail to write max17040." );
		return -1;
	}

	return ret;
}

static int i2c_read_dur_sleep( unsigned char reg_addr )
{
	unsigned char buf[2];
	unsigned int ret = normal_i2c_read_word(fuelgauge_i2c_client->addr, reg_addr, buf);
	if(ret < 0)
	{
		printk(KERN_ERR"[%s] Fail to Read max17040\n", __FUNCTION__);
		return -1;
	} 

	ret = buf[0] << 8 | buf[1];

	return ret;
}

static irqreturn_t low_battery_isr( int irq, void *_di )
{
	if ( sec_bci->ready )
	{
		cancel_delayed_work( &fuelgauge_work );
		schedule_delayed_work( &fuelgauge_work, 0 );
	}
	
	return IRQ_HANDLED;	
}

static void fuelgauge_work_handler( struct work_struct *work )
{
	printk( "[FG] ext.low_battery!\n" );
	_low_battery_alarm_();
}

static int fuelgauge_probe( struct i2c_client *client, 
                            const struct i2c_device_id *id )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	int ret = 0;
	unsigned char buf[3];

	printk( "[FG] Fuelgauge Probe.\n" );

	sec_bci = get_sec_bci();

	if( strcmp( client->name, DRIVER_NAME) != 0 )
	{
		ret = -1;
		printk( "[FG] device not supported.\n" );
	}

	fuelgauge_i2c_client = client;

	if ( client->irq )
	{
		INIT_DELAYED_WORK( &fuelgauge_work, fuelgauge_work_handler );

		// set alert threshold to 1%
		ret = i2c_read( REG_RCOMP );
		buf[0] = REG_RCOMP;
		buf[1] = ret >> 8;
		buf[2] = 0x1F; // 1%
		i2c_write( buf, 3 );

		ret = i2c_read( REG_RCOMP );
		
#if defined(CONFIG_MACH_HALO)
		startingRcomp = ret >> 8;
		printk( "[FG] startingRcomp = 0x%x \n", startingRcomp );
#endif
		printk( "[FG] val : %x\n", ret );

		ret = irq_to_gpio( client->irq );
		printk( "[FG] FUEL_INT_GPIO : %d \n", ret );

		set_irq_type( client->irq, IRQ_TYPE_EDGE_FALLING );
		ret = request_irq( client->irq, low_battery_isr, IRQF_DISABLED, client->name, NULL );
		if ( ret )
		{
			printk( "[FG] could not request irq %d, status %d\n", client->irq, ret );
		}

	}

	sec_bci->charger.fuelgauge_full_soc = 95;  // for adjust fuelgauge

	return ret;
}

static int fuelgauge_remove( struct i2c_client * client )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	return 0;
}

static void fuelgauge_shutdown( struct i2c_client * client )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	return;
}

static int fuelgauge_suspend( struct i2c_client * client , pm_message_t mesg )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{

#if !defined(CONFIG_MACH_OSCAR)
	int batt_ptg = 0;
	int ret = 0;
	unsigned char buf[3];
	unsigned char alert_th;

	if(!sec_bci->charger.is_charging)
	{
		batt_ptg = get_fuelgauge_ptg_value(false);
		if(batt_ptg >= 15)
		{
			alert_th = 0x11; // 15%
		}
		else if(batt_ptg >= 4)
		{
			alert_th = 0x1C; // 4%
		}
		else
		{
			alert_th = 0x1F; // 1%
		}
		// set alert threshold to 1%
		ret = i2c_read( REG_RCOMP );
		buf[0] = REG_RCOMP;
		buf[1] = ret >> 8;
		buf[2] = alert_th; // 1%
		i2c_write( buf, 3 );
		printk("%s, set alert threshold 0x%2x\n", __func__, alert_th);
	}
#endif
	
	return 0;
}

static int fuelgauge_resume( struct i2c_client * client )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	return 0;
}

static const struct i2c_device_id fuelgauge_i2c_id[] = {
	{ DRIVER_NAME, 0 },
	{ },
};

static struct i2c_driver fuelgauge_i2c_driver =
{
	.driver = {
		.name   = DRIVER_NAME,
		.owner  = THIS_MODULE,
	},       

	.probe      = fuelgauge_probe,
	.remove     = __devexit_p( fuelgauge_remove ),
	.shutdown   = fuelgauge_shutdown,
	.suspend    = fuelgauge_suspend,
	.resume     = fuelgauge_resume,
	.id_table	= fuelgauge_i2c_id,
};

int fuelgauge_init( void )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	int ret;

	printk("[FG] Fuelgauge Init.\n");

	if( ( ret = i2c_add_driver( &fuelgauge_i2c_driver ) < 0 ) )
	{
		printk( KERN_ERR "[FG] i2c_add_driver failed.\n" );
	}

	return ret;    
}

void fuelgauge_exit( void )
// ----------------------------------------------------------------------------
// Description    : 
// Input Argument :  
// Return Value   : 
{
	printk("[FG] Fuelgauge Exit.\n");
	i2c_del_driver( &fuelgauge_i2c_driver );
}


