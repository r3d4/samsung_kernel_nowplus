/*
 *  max17040_battery.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Minkyu Kang <mk7.kang@samsung.com>
 *  Copyright (C) 2009 hardkernel Co.,ltd.
 * Hakjoo Kim <ruppi.kim@hardkernel.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/max17040_battery.h>
#include <linux/i2c/twl4030-madc.h>
#include <linux/i2c/twl.h>
#include <linux/regulator/consumer.h>
//#include <mach/resource.h>

#include <asm/mach/irq.h>
#include <asm/io.h>
#include <mach/gpio.h>

#include "max17040.h"

#define VSEL_VINTANA2_2V75  0x01
#define CARKIT_ANA_CTRL     0xBB
#define SEL_MADC_MCPC       0x08
#define USB_PHY_CLK_CTRL	0xFE

#define TURN_ON 			1
#define TURN_OFF 			0

#ifdef CONFIG_BATTERY_MAX17040_TEMP	
 
static struct max17040_chip *this_max = NULL;
static struct wake_lock adc_wakelock;


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
	temp -=1; // fixme for temperature test change -4 ==> -1

	return temp;
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

/*
LIMO:
	resource_request(t2_vusb, T2_VUSB3V1_3V1);
	resource_request(t2_bci, T2_VINTANA2_2V75);	
	
	get_average_adc_value(..);
	
	resource_release(t2_bci);
	resource_release(t2_vusb);
*/
int switch_resources_for_adc(int state)
{
	int ret;
	u8 val = 0;	
	static int old_state = TURN_OFF;
	
//printk("%s\n", __func__);
	
	if(old_state==state)
		printk("[MAX17040] %s: resources alread %s", __func__, state?"enabled":"disabled");

	if(state == TURN_ON)
	{
		ret = regulator_enable( this_max->usb3v1 );
		if ( ret )
			printk("Regulator 3v1 error!!\n");

		ret = regulator_enable( this_max->usb1v8 );
		if ( ret )
			printk("Regulator 1v8 error!!\n");

		ret = regulator_enable( this_max->usb1v5 );
		if ( ret )
			printk("Regulator 1v5 error!!\n");

		// !!!!!!!!!!!!!
		//twl_i2c_write_u8( TWL4030_MODULE_USB, 0, 0xFD/*PHY_PWR_CTRL*/ );
		/*
		 * TI CSR OMAPS00222879: "MP3 playback current consumption is very high"
		 * This a workaround to reduce the battery temperature.
		 */
		// FXIME twl_i2c_write_u8(TWL4030_MODULE_USB, 0, 0xFD/*PHY_PWR_CTRL*/);

	#if 1
		twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x14, TWL4030_VUSB_DEDICATED1 );	// 0x14= SW2VBAT | WKUPCOMP_EN
		twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x0,  TWL4030_VUSB_DEDICATED2 );	// remapping to sleep disabled
		
		// Powers the PHY and wakes up the clock
		twl_i2c_read_u8( TWL4030_MODULE_USB, &val, USB_PHY_CLK_CTRL );
		val |= 0x1;
		twl_i2c_write_u8( TWL4030_MODULE_USB, val, USB_PHY_CLK_CTRL );

		//turn on VINTANA2 = 2.75V
		twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, VSEL_VINTANA2_2V75,   TWL4030_VINTANA2_DEDICATED );    
		twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x20, 				TWL4030_VINTANA2_DEV_GRP );
	#endif

		//twl_i2c_read_u8(TWL4030_MODULE_USB, &ana_val, CARKIT_ANA_CTRL);
		//route RTSO to ADCIN5
		twl_i2c_write_u8( TWL4030_MODULE_USB, SEL_MADC_MCPC, CARKIT_ANA_CTRL );
		
		//power on MADC (off -> wait -> on)
		ret = twl_i2c_read_u8( TWL4030_MODULE_MADC, &val, TWL4030_MADC_CTRL1 );
		val &= ~TWL4030_MADC_MADCON;
		ret = twl_i2c_write_u8( TWL4030_MODULE_MADC, val, TWL4030_MADC_CTRL1 );
		msleep( 10 );
		ret = twl_i2c_read_u8( TWL4030_MODULE_MADC, &val, TWL4030_MADC_CTRL1 );
		val |= TWL4030_MADC_MADCON;
		ret = twl_i2c_write_u8( TWL4030_MODULE_MADC, val, TWL4030_MADC_CTRL1 );
			
	
	}
	else
	{
		//power off MADC
		twl_i2c_read_u8( TWL4030_MODULE_MADC, &val, TWL4030_MADC_CTRL1 );
		val &= ~TWL4030_MADC_MADCON;
		twl_i2c_write_u8( TWL4030_MODULE_MADC, val, TWL4030_MADC_CTRL1 );
		
		if ( !(this_max->online) )	// charger not connected?
		{
	#if 1
			// !!!!!!!!!!!!!
		//	twl_i2c_write_u8( TWL4030_MODULE_USB, 1, 0xFD/*PHY_PWR_CTRL*/ );
		
			//twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x14, 0x7D/*VUSB_DEDICATED1*/ );
			//twl_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x0, 0x7E/*VUSB_DEDICATED2*/ );
			
			twl_i2c_read_u8( TWL4030_MODULE_USB, &val, 0xFE/*PHY_CLK_CTRL*/ );
			val &= 0xFE;	//clear bit 0 > PHY state depends on the SUSPENDM bit in the FUNC_CTRL register
			twl_i2c_write_u8( TWL4030_MODULE_USB, val, 0xFE/*PHY_CLK_CTRL*/ );
	#endif
		}

		regulator_disable( this_max->usb1v5 );
		regulator_disable( this_max->usb1v8 );
		regulator_disable( this_max->usb3v1 );

		//twl4030_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0x00, VINTANA2_DEV_GRP);
		//twl4030_i2c_write_u8(TWL4030_MODULE_USB, ana_val, CARKIT_ANA_CTRL);
		//twl4030_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, REMAP_OFF, TWL4030_VINTANA2_REMAP );
		/*
		 * TI CSR OMAPS00222879: "MP3 playback current consumption is very high"
		 * This a workaround to reduce the battery temperature.
		 */
		// FIXME twl_i2c_write_u8(TWL4030_MODULE_USB, 1, 0xFD/*PHY_PWR_CTRL*/);

	}
	old_state = state;
	
	return 0;
}



int _get_t2adc_data_( int ch )
{
	int ret = 0;
	int val[TEMP_ADC_AVG];
	int i;
	//u8 ana_val;
	struct twl4030_madc_request req;
	
//	mutex_lock( &battery_lock );

//	msleep( 100 );

	wake_lock(&adc_wakelock);
	
	if ( ch >= 1 && ch <= 7 )
	{
		switch_resources_for_adc(TURN_ON);
		msleep(100);
	}

	req.channels = ( 1 << ch );
	req.do_avg = 0;
	req.method = TWL4030_MADC_SW1;
	req.active = 0;
	req.func_cb = NULL;
	
	for ( i = 0; i < TEMP_ADC_AVG ; i++ )
	{
		twl4030_madc_conversion( &req );
		val[i] = req.rbuf[ch];
	}

	ret = _get_average_value_( val, TEMP_ADC_AVG );

	if ( ch >= 1 && ch <= 7 )
	{
		switch_resources_for_adc(TURN_OFF);
	}
	
	wake_unlock(&adc_wakelock);
//	mutex_unlock( &battery_lock );

	return ret;
}

int max17040_get_temperature(struct i2c_client *client)
{
	//struct max17040_chip *chip = i2c_get_clientdata(client);
	int adc;
	int temp;
	static int cycle = 0;
//printk("%s: %d\n", __func__, __LINE__);

	if(this_max == NULL)
		return 1;
		
	if(cycle++%TEMP_ADC_CYCLE)
		return 0;
		
//printk("%s: %d\n", __func__, __LINE__);
		
	adc = _get_t2adc_data_( TEMP_ADC_CH );
	temp = t2adc_to_temperature( adc, TEMP_ADC_CH );
	
//printk("[MAX17040] TEMP adc= %d, temp= %d deg\n", adc, temp);

	this_max->temp = temp;
	
	return 0;
}


int max17040_init_temp(struct i2c_client *client)
{
	struct max17040_chip *chip = i2c_get_clientdata(client);
//printk("%s: %d\n", __func__, __LINE__);
	
	//wakelock for adc conversion
	wake_lock_init( &adc_wakelock, WAKE_LOCK_SUSPEND, "max17040-adc" );

	// reserve regulators needed for ADC
	chip->usb3v1 = regulator_get( &client->dev, "usb3v1" );
	if( IS_ERR( chip->usb3v1 ) )
		goto fail_regulator1;

	chip->usb1v8 = regulator_get( &client->dev, "usb1v8" );
	if( IS_ERR( chip->usb1v8 ) )
		goto fail_regulator2;

	chip->usb1v5 = regulator_get( &client->dev, "usb1v5" );
	if( IS_ERR( chip->usb1v5 ) )
		goto fail_regulator3;
//printk("%s: %d\n", __func__, __LINE__);
	this_max = chip;
	return 0;
	
fail_regulator3:
	regulator_put( chip->usb1v8 );
	chip->usb1v8 = NULL;
fail_regulator2:
	regulator_put( chip->usb3v1 );
	chip->usb3v1 = NULL;
fail_regulator1:		
//printk("%s: %d\n", __func__, __LINE__);
	return 1;		
}

int max17040_exit_temp(void)
{	
//printk("%s: %d\n", __func__, __LINE__);

	if(this_max == NULL)
		return 1;
//printk("%s: %d\n", __func__, __LINE__);
	
	regulator_put( this_max->usb1v5 );
	regulator_put( this_max->usb1v8 );
	regulator_put( this_max->usb3v1 );
	
	return 0;
}

#endif