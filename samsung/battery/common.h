#ifndef _COMMON_H_
#define _COMMON_H_

#define WR_ADC 
#ifdef WR_ADC
/* Workaround to get proper adc value */
#include <linux/i2c/twl.h>
#endif

#define MONITOR_DURATION_DUR_SLEEP  30
#define MONITOR_DEFAULT_DURATION    30
#define MONITOR_TEMP_DURATION       30 // sec
#define MONITOR_RECHG_VOL_DURATION  30 // sec

#define DEFAULT_CHARGING_TIMEOUT        5 * 60 * 60                 // 5 hours (18000 sec)
#define DEFAULT_RECHARGING_TIMEOUT        2 * 60 * 60                 // 2 hours (18000 sec)

#define CHARGE_STOP_TEMPERATURE_MAX		50
#define CHARGE_STOP_TEMPERATURE_MIN		-5
#define CHARGE_RECOVER_TEMPERATURE_MAX		43
#define CHARGE_RECOVER_TEMPERATURE_MIN		 1

#define CHARGE_FULL_CURRENT_ADC	250 // archer: 190mA (ADC 170)==> 170mA (ADC 250) change


#define CHARGE_DUR_ACTIVE 0
#define CHARGE_DUR_SLEEP  1

#define STATUS_CATEGORY_CABLE       1
#define STATUS_CATEGORY_CHARGING    2
#define STATUS_CATEGORY_TEMP        3
#define STATUS_CATEGORY_ETC         4

enum {
/*	POWER_SUPPLY_STATUS_UNKNOWN = 0, // power_supply.h
	POWER_SUPPLY_STATUS_CHARGING,
	POWER_SUPPLY_STATUS_DISCHARGING,
	POWER_SUPPLY_STATUS_NOT_CHARGING,
	POWER_SUPPLY_STATUS_FULL,*/
	POWER_SUPPLY_STATUS_CHARGING_OVERTIME = 5,
	POWER_SUPPLY_STATUS_RECHARGING_FOR_FULL = 6,
	POWER_SUPPLY_STATUS_RECHARGING_FOR_TEMP = 7,
	POWER_SUPPLY_STATUS_FULL_DUR_SLEEP = 8,
};

enum {
	BATTERY_TEMPERATURE_NORMAL = 0,
	BATTERY_TEMPERATURE_LOW,
	BATTERY_TEMPERATURE_HIGH,
};

enum {
	ETC_CABLE_IS_DISCONNECTED = 0,
};

typedef struct
{
	bool ready;

	/*Battery info*/
	struct _battery 
	{
		int  battery_technology;
		int  battery_level_ptg;
		int  battery_level_vol; // mV
		int  battery_temp;
		int  battery_health;
		bool battery_vf_ok;
		bool battery_vol_toolow;

		int  monitor_duration;
		bool monitor_field_temp;
		bool monitor_field_rechg_vol;
		int  support_monitor_temp;
		int  support_monitor_timeout;
		int  support_monitor_full;

		int confirm_full_by_current;
			//int confirm_changing_freq;
		int confirm_recharge;
	}battery;

	/*Charger info*/
	struct _charger
	{
		int prev_cable_status;
		int cable_status;

		int prev_charge_status;
		int charge_status;

		char full_charge_dur_sleep; // 0x0: No Full, 0x1: Full, 0x2: 5Hours
		//bool full_charge;
		bool is_charging;

		unsigned long long  charge_start_time;
		unsigned long charged_time;
		int charging_timeout; // 5 hours (18000 sec), 2 hours for recharging.

		bool use_ta_nconnected_irq;

		int fuelgauge_full_soc;   // for adjust fuelgauge
		// during sleep
		//bool chg_full_wakeup;
	}charger; 

	struct workqueue_struct *sec_battery_workq;

}SEC_battery_charger_info;

#endif

