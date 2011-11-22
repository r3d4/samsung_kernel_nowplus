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

 #ifndef _MAX17040_H_
 #define _MAX17040_H_
 
 

#define MAX17040_VCELL_MSB	0x02
#define MAX17040_VCELL_LSB	0x03
#define MAX17040_SOC_MSB	0x04
#define MAX17040_SOC_LSB	0x05
#define MAX17040_MODE_MSB	0x06
#define MAX17040_MODE_LSB	0x07
#define MAX17040_VER_MSB	0x08
#define MAX17040_VER_LSB	0x09
#define MAX17040_RCOMP_MSB	0x0C
#define MAX17040_RCOMP_LSB	0x0D
#define MAX17040_CMD_MSB	0xFE
#define MAX17040_CMD_LSB	0xFF

#define MAX17040_DELAY		msecs_to_jiffies(1000)
#define MAX17040_BATTERY_FULL	95
#define MAX17040_BATTERY_CHG	90  //add for hysteresis


#define TEMP_ADC_CH		5		// twl channel on which the ntc is connected
#define TEMP_ADC_AVG	5		// averaging for ADC conversion
#define TEMP_ADC_CYCLE	30		// only sample every ... max17040_work cycle


int max17040_get_temperature(struct i2c_client *client);
int max17040_init_temp(struct i2c_client *client);
int max17040_exit_temp(void);

struct max17040_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		battery;
	struct power_supply		ac;
	struct power_supply		usb;
	struct max17040_platform_data	*pdata;

	/* State Of Connect */
	int online;
	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;
	/* State Of Charge */
	int status;
	/* usb online */
	int usb_online;
	
	int temp;
	
	struct regulator *usb1v5;
	struct regulator *usb1v8;
	struct regulator *usb3v1;
	bool usb3v1_is_enabled;
};


 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 #endif
