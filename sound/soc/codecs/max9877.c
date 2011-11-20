/*
 * max9877.c  --  amp driver for max9877
 *
 * Copyright (C) 2009 Samsung Electronics Co.Ltd
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#include "max9877.h"
#include <linux/workqueue.h>

#define SEC_MAX9877_DEBUG 0

#if SEC_MAX9877_DEBUG
#define P(format,...)\
		printk("[audio:%s]" format "\n", __func__, ## __VA_ARGS__);
#else
#define P(format,...)
#endif

static struct i2c_client *i2c;
struct delayed_work amp_control_work;


//static u8 max9877_regs[5] = { 0x40, 0x00, 0x00, 0x00, 0x49 };
static u8 max9877_regs[5] = { 0x40, 0x1e, 0x1e, 0x1e, 0x81};
#define DEFAULT_INPUT 0x40
#define SPK_VOLUME 0x1e
#define HPL_VOLUME 0x1e
#define HPR_VOLUME 0x1e
static u8 old_output_mode = 0x00;
static u8 prev_output_mode = 0x00;
static u8 curr_output_mode = 0x00;

static void max9877_write_regs(void)
{
	unsigned int i;
	u8 data[6];

    P("");

	data[0] = MAX9877_INPUT_MODE;

	for (i = 0; i < ARRAY_SIZE(max9877_regs); i++)
    {
		data[i + 1] = max9877_regs[i];
         //printk("*** Max9877 Register [ 0x%x : 0x%x] \n", i+1, max9877_regs[i]);
    }

	if (i2c_master_send(i2c, data, 6) != 6)
		dev_err(&i2c->dev, "i2c write failed\n");

}

static void amp_control_work_handler(struct work_struct *work )
{
	P("");
	
	max9877_regs[MAX9877_SPK_VOLUME] = SPK_VOLUME;
	    
	if(curr_output_mode==3)
	{
		max9877_regs[MAX9877_HPL_VOLUME] = 0x11;
		max9877_regs[MAX9877_HPR_VOLUME] = 0x11;
	}
	else
	{
		max9877_regs[MAX9877_HPL_VOLUME] = HPL_VOLUME;
		max9877_regs[MAX9877_HPR_VOLUME] = HPR_VOLUME;
	}

	max9877_write_regs();
}

static int max9877_get_reg(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int mask = mc->max;
	unsigned int invert = mc->invert;

	ucontrol->value.integer.value[0] = (max9877_regs[reg] >> shift) & mask;

	if (invert)
		ucontrol->value.integer.value[0] =
			mask - ucontrol->value.integer.value[0];

	return 0;
}

static int max9877_set_reg(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int mask = mc->max;
	unsigned int invert = mc->invert;
	unsigned int val = (ucontrol->value.integer.value[0] & mask);

    P("");

	if (invert)
		val = mask - val;

	if (((max9877_regs[reg] >> shift) & mask) == val)
		return 0;

	max9877_regs[reg] &= ~(mask << shift);
	max9877_regs[reg] |= val << shift;
	max9877_write_regs();

	return 1;
}

static int max9877_get_2reg(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	unsigned int mask = mc->max;

	ucontrol->value.integer.value[0] = (max9877_regs[reg] >> shift) & mask;
	ucontrol->value.integer.value[1] = (max9877_regs[reg2] >> shift) & mask;

	return 0;
}

static int max9877_set_2reg(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int reg2 = mc->rreg;
	unsigned int shift = mc->shift;
	unsigned int mask = mc->max;
	unsigned int val = (ucontrol->value.integer.value[0] & mask);
	unsigned int val2 = (ucontrol->value.integer.value[1] & mask);
	unsigned int change = 1;

    P("");

	if (((max9877_regs[reg] >> shift) & mask) == val)
		change = 0;

	if (((max9877_regs[reg2] >> shift) & mask) == val2)
		change = 0;

	if (change) 
	{
		max9877_regs[reg] &= ~(mask << shift);
		max9877_regs[reg] |= val << shift;
		max9877_regs[reg2] &= ~(mask << shift);
		max9877_regs[reg2] |= val2 << shift;
		max9877_write_regs();
	}

	return change;
}

void max9877_power_down_mode(void)
{
        max9877_regs[0] = 0x0;
        max9877_regs[1] = 0x0;
        max9877_regs[2] = 0x0;
        max9877_regs[3] = 0x0;
        max9877_regs[4] = 0x40;

        max9877_write_regs();
}
EXPORT_SYMBOL_GPL(max9877_power_down_mode);

static int max9877_get_out_mode(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	u8 value = max9877_regs[MAX9877_OUTPUT_MODE] & MAX9877_OUTMODE_MASK;

    P("");

	if (value)
		value -= 1;

    P(" value : ox%x", value);

	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int max9877_set_out_mode(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{

    curr_output_mode = ucontrol->value.integer.value[0];

	curr_output_mode += 1;

    P(" curr_output_mode : ox%x", curr_output_mode);


    if(curr_output_mode == 3 ||curr_output_mode == 5 )
    {
		max9877_regs[MAX9877_INPUT_MODE] = DEFAULT_INPUT;
		max9877_regs[MAX9877_SPK_VOLUME] = SPK_VOLUME; //0x00;
		max9877_regs[MAX9877_HPL_VOLUME] = 0x00;
		max9877_regs[MAX9877_HPR_VOLUME] = 0x00;
	}
    else
    {
		max9877_regs[MAX9877_INPUT_MODE] = DEFAULT_INPUT;
		max9877_regs[MAX9877_SPK_VOLUME] = SPK_VOLUME;
		max9877_regs[MAX9877_HPL_VOLUME] = HPL_VOLUME;
		max9877_regs[MAX9877_HPR_VOLUME] = HPR_VOLUME;
	}


	max9877_regs[MAX9877_OUTPUT_MODE] &= ~MAX9877_BYPASS;
	max9877_regs[MAX9877_OUTPUT_MODE] &= ~MAX9877_OUTMODE_MASK;
	max9877_regs[MAX9877_OUTPUT_MODE] |= curr_output_mode;	
	max9877_regs[MAX9877_OUTPUT_MODE] |= MAX9877_SHDN;

	if(prev_output_mode==max9877_regs[MAX9877_OUTPUT_MODE])
	{		
		P(" same output mode!!! mode = %d",prev_output_mode );
		return 1;
	}

    //max9877_write_regs();

	old_output_mode = max9877_regs[MAX9877_OUTPUT_MODE] ;
	prev_output_mode = max9877_regs[MAX9877_OUTPUT_MODE] ;

    if( curr_output_mode == 5 )
	{
		schedule_delayed_work( &amp_control_work, HZ/50 );
	}
    else
	{
		schedule_delayed_work( &amp_control_work, 3 );
	}

    #if 0
    if(curr_output_mode == 3 ||curr_output_mode == 5 )
    {
        //mdelay(5);
	    max9877_regs[MAX9877_SPK_VOLUME] = SPK_VOLUME;
	    if(curr_output_mode==3)
	    {
	  	    max9877_regs[MAX9877_HPL_VOLUME] = 0x11;
	 	    max9877_regs[MAX9877_HPR_VOLUME] = 0x11;
	    }
	    else
	    {
	  	    max9877_regs[MAX9877_HPL_VOLUME] = HPL_VOLUME;
	 	    max9877_regs[MAX9877_HPR_VOLUME] = HPR_VOLUME;
	    }
	    max9877_write_regs();
    }
    #endif

	return 1;
}

static int max9877_get_osc_mode(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	u8 value = (max9877_regs[MAX9877_OUTPUT_MODE] & MAX9877_OSC_MASK);

    P("");

	value = value >> MAX9877_OSC_OFFSET;

    P(" value : ox%x", value);

	ucontrol->value.integer.value[0] = value;
	return 0;
}

static int max9877_set_osc_mode(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
	u8 value = ucontrol->value.integer.value[0];

	value = value << MAX9877_OSC_OFFSET;

    P("");      

	if ((max9877_regs[MAX9877_OUTPUT_MODE] & MAX9877_OSC_MASK) == value)
		return 0;

    P("value : 0x%x", value);

	max9877_regs[MAX9877_OUTPUT_MODE] &= ~MAX9877_OSC_MASK;
	max9877_regs[MAX9877_OUTPUT_MODE] |= value;
	max9877_write_regs();

	return 1;
}

static int max9877_set_power_control(struct snd_kcontrol *kcontrol,
                struct snd_ctl_elem_value *ucontrol)
{
	P(" max9877_power_control value = %d \n", (int)ucontrol->value.integer.value[0]);

	if(ucontrol->value.integer.value[0])  //off case
	{
		max9877_regs[0] = 0x0;
		max9877_regs[1] = 0x0;
		max9877_regs[2] = 0x0;
		max9877_regs[3] = 0x0;
		max9877_regs[4] = 0x40;
		prev_output_mode = max9877_regs[4];
	}
	else
	{
		max9877_regs[0] = DEFAULT_INPUT;
		max9877_regs[1] = SPK_VOLUME;
		max9877_regs[2] = HPL_VOLUME;
		max9877_regs[3] = HPR_VOLUME;
		max9877_regs[4] = old_output_mode;
	}

	max9877_write_regs();

        return 0;
}

static int max9877_get_power_control(struct snd_kcontrol *kcontrol,
                struct snd_ctl_elem_value *ucontrol)
{
	int power_on = 0;

	if (max9877_regs[MAX9877_OUTPUT_MODE] &= MAX9877_SHDN) 
	{
		power_on = 1;
	}
	else
		power_on = 0;

	P(" get power %d",power_on);
	ucontrol->value.integer.value[0] = power_on;

	return 0;
}
static const unsigned int max9877_pgain_tlv[] = {
	TLV_DB_RANGE_HEAD(2),
	0, 1, TLV_DB_SCALE_ITEM(0, 900, 0),
	2, 2, TLV_DB_SCALE_ITEM(2000, 0, 0),
};

static const unsigned int max9877_output_tlv[] = {
	TLV_DB_RANGE_HEAD(4),
	0, 7, TLV_DB_SCALE_ITEM(-7900, 400, 1),
	8, 15, TLV_DB_SCALE_ITEM(-4700, 300, 0),
	16, 23, TLV_DB_SCALE_ITEM(-2300, 200, 0),
	24, 31, TLV_DB_SCALE_ITEM(-700, 100, 0),
};

static const char *max9877_out_mode[] = {
	"INA -> SPK",
	"INA -> HP",
	"INA -> SPK and HP",
	"INB -> SPK",
	"INB -> HP",
	"INB -> SPK and HP",
	"INA + INB -> SPK",
	"INA + INB -> HP",
	"INA + INB -> SPK and HP",
};

static const char *max9877_osc_mode[] = {
	"1176KHz",
	"1100KHz",
	"700KHz",
};

static const char *max9877_power_onoff[]={
	"ON",
	"OFF"
};

static const struct soc_enum max9877_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(max9877_out_mode), max9877_out_mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(max9877_osc_mode), max9877_osc_mode),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(max9877_power_onoff), max9877_power_onoff),
};

static const struct snd_kcontrol_new max9877_controls[] = {
    SOC_SINGLE_EXT_TLV("MAX9877 PGAINB Playback Volume",
			MAX9877_INPUT_MODE, 0, 2, 0,
			max9877_get_reg, max9877_set_reg, max9877_pgain_tlv),
    SOC_SINGLE_EXT_TLV("MAX9877 PGAINA Playback Volume",
			MAX9877_INPUT_MODE, 2, 2, 0,
			max9877_get_reg, max9877_set_reg, max9877_pgain_tlv),
	SOC_SINGLE_EXT_TLV("MAX9877 Amp Speaker Playback Volume",
			MAX9877_SPK_VOLUME, 0, 31, 0,
			max9877_get_reg, max9877_set_reg, max9877_output_tlv),
	SOC_DOUBLE_R_EXT_TLV("MAX9877 Amp HP Playback Volume",
			MAX9877_HPL_VOLUME, MAX9877_HPR_VOLUME, 0, 31, 0,
			max9877_get_2reg, max9877_set_2reg, max9877_output_tlv),
	SOC_SINGLE_EXT("MAX9877 INB Stereo Switch",
			MAX9877_INPUT_MODE, 4, 1, 1,
			max9877_get_reg, max9877_set_reg),
	SOC_SINGLE_EXT("MAX9877 INA Stereo Switch",
			MAX9877_INPUT_MODE, 5, 1, 1,
			max9877_get_reg, max9877_set_reg),
	SOC_SINGLE_EXT("MAX9877 Zero-crossing detection Switch",
			MAX9877_INPUT_MODE, 6, 1, 0,
			max9877_get_reg, max9877_set_reg),
	SOC_SINGLE_EXT("MAX9877 Bypass Mode Switch",
			MAX9877_OUTPUT_MODE, 6, 1, 0,
			max9877_get_reg, max9877_set_reg),
	SOC_SINGLE_EXT("MAX9877 Shutdown Mode Switch",
			MAX9877_OUTPUT_MODE, 7, 1, 1,
			max9877_get_reg, max9877_set_reg),
	SOC_ENUM_EXT("MAX9877 Output Mode", max9877_enum[0],
			max9877_get_out_mode, max9877_set_out_mode),
	SOC_ENUM_EXT("MAX9877 Oscillator Mode", max9877_enum[1],
			max9877_get_osc_mode, max9877_set_osc_mode),
	SOC_ENUM_EXT("Amp Enable", max9877_enum[2],
			max9877_get_power_control, max9877_set_power_control),
};

#if 0
/* This function is called from ASoC machine driver */
int max9877_add_controls(struct snd_soc_codec *codec)
{
	return snd_soc_add_controls(codec, max9877_controls,
			ARRAY_SIZE(max9877_controls));
}
EXPORT_SYMBOL_GPL(max9877_add_controls);
#else 
  /* This function is called from ASoC machine driver */
int max9877_add_controls(struct snd_soc_codec *codec)
{
        int err, i;

        for (i = 0; i < ARRAY_SIZE(max9877_controls); i++) {
                err = snd_ctl_add(codec->snd_card,
                                snd_soc_cnew(&max9877_controls[i],
                                        codec, NULL));
                if (err < 0)
                        return err;
        }

        return 0;
}
EXPORT_SYMBOL_GPL(max9877_add_controls);
#endif

static int __devinit max9877_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	i2c = client;

    P("");

    //max9877_write_regs();

	return 0;
}

static __devexit int max9877_i2c_remove(struct i2c_client *client)
{
	i2c = NULL;

	return 0;
}

static int max9877_suspend(struct i2c_client *client, pm_message_t mesg)
{
	P("");

#if 0
	max9877_regs[0] = 0x0;
	max9877_regs[1] = 0x0;
	max9877_regs[2] = 0x0;
	max9877_regs[3] = 0x0;
	max9877_regs[4] = 0x40;

	max9877_write_regs();
#endif

	return 0;
}

static int max9877_resume(struct i2c_client *client)
{
	#if 0
	max9877_regs[0] = 0x40;
	max9877_regs[1] = 0x1e;
	max9877_regs[2] = 0x1e;
	max9877_regs[3] = 0x1e;
	max9877_regs[4] = 0x81;

	max9877_write_regs();
	#endif
	return 0;
}

static void max9877_shutdown(struct i2c_client *client)
{

	printk("shutdown !!\n");

	max9877_regs[0] = 0x0;
	max9877_regs[1] = 0x0;
	max9877_regs[2] = 0x0;
	max9877_regs[3] = 0x0;
	max9877_regs[4] = 0x40;

	max9877_write_regs();
}

static const struct i2c_device_id max9877_i2c_id[] = {
	{ "max9877", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, max9877_i2c_id);

static struct i2c_driver max9877_i2c_driver = {
	.driver = {
		.name = "max9877",
		.owner = THIS_MODULE,
	},
	.probe = max9877_i2c_probe,
	.remove = __devexit_p(max9877_i2c_remove),
	.suspend = max9877_suspend,
	.resume = max9877_resume,
	.shutdown = max9877_shutdown,
	.id_table = max9877_i2c_id,
};

static int __init max9877_init(void)
{
    INIT_DELAYED_WORK( &amp_control_work, amp_control_work_handler ); //sec_lilkan	

	return i2c_add_driver(&max9877_i2c_driver);
}
module_init(max9877_init);

static void __exit max9877_exit(void)
{
	i2c_del_driver(&max9877_i2c_driver);
}
module_exit(max9877_exit);

MODULE_DESCRIPTION("ASoC MAX9877 amp driver");
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_LICENSE("GPL");

