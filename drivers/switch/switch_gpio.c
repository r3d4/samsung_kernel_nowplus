/*
 *  drivers/switch/switch_gpio.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/i2c/twl4030-madc.h>
#include <linux/i2c/twl.h>
#include <linux/regulator/consumer.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <mach/hardware.h>
#include <plat/mux.h>

#if defined(CONFIG_SEC_HEADSET_CLASS_FOR_OMS)
#include "./accessory.h"
#endif
#include "switch_omap_gpio.h"
//#define CONFIG_DEBUG_SEC_HEADSET

#ifdef CONFIG_DEBUG_SEC_HEADSET
#define SEC_HEADSET_DBG(fmt, arg...) printk(KERN_INFO "[HEADSET] %s () " fmt "\r\n", __func__, ## arg)
#else
#define SEC_HEADSET_DBG(fmt, arg...) do {} while(0)
#endif

#define HEADSET_ATTACH_COUNT		5
#define HEADSET_DETACH_COUNT		3
#define HEADSET_CHECK_TIME			get_jiffies_64() + (HZ/20)// 1000ms / 10 = 100ms
#define SEND_END_ENABLE_TIME 		get_jiffies_64() + (HZ*2)// 1000ms * 2 = 2sec

struct gpio_switch_data	*data = NULL;
static unsigned short int headset_status;
static struct timer_list headset_detect_timer;
static unsigned int headset_detect_timer_token;

static struct wake_lock headset_wake_lock;

//for regulator control for getting adc value
#define VINTANA2_DEV_GRP   0x43
#define VINTANA2_DEDICATED 0x46
#define VSEL_VINTANA2_2V75 0x01

#define VUSB1V8_DEV_GRP     0x74
#define VUSB3V1_DEV_GRP     0x77
#define CARKIT_ANA_CTRL     0xBB
#define SEL_MADC_MCPC       0x08
//#define USE_REGULATOR

struct switch_device_info
{
        struct device *dev;

#ifdef USE_REGULATOR
        struct regulator *usb1v5;
        struct regulator *usb1v8;
        struct regulator *usb3v1;
#endif

};

static struct device *this_dev;



#ifdef CONFIG_SEC_HEADSET_CLASS_FOR_OMS
#define DETECTING		(1 << 2)

static int micco_headset_get_property(struct accessory *acy,
				  enum accessory_property pacy,
				  union accessory_propval *val)
{
	val->intval = headset_status;

	switch (pacy) {
	case ACCESSORY_PROP_ONLINE:
		if ( val->intval & DETECTING ) 
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static enum accessory_property micco_headset_props[] = {
	ACCESSORY_PROP_ONLINE,
};
static struct accessory micco_headset_accessory = {
	.name = "headset",
	.type = ACCESSORY_TYPE_HEADSET,
	.properties = micco_headset_props,
	.num_properties = ARRAY_SIZE(micco_headset_props),
	.get_property = micco_headset_get_property,
	.put_property = NULL,

};
#endif


#ifdef CONFIG_INPUT_ZEUS_EAR_KEY
extern void ear_key_disable_irq(void);
extern void ear_key_enable_irq(void);
extern void release_ear_key(void);
#endif

struct gpio_switch_data {
	struct switch_dev sdev;
	unsigned gpio;
	const char *name_on;
	const char *name_off;
	const char *state_on;
	const char *state_off;
	int irq;
	struct work_struct work;
};

short int get_headset_status(void)
{
	SEC_HEADSET_DBG(" headset_status %d", headset_status);
	return headset_status;
}
EXPORT_SYMBOL(get_headset_status);

static void release_headset_event(unsigned long arg)
{
	printk("[Headset]Headset attached\n");
	headset_status = 1;
	switch_set_state(&data->sdev, 1);
}
static DECLARE_DELAYED_WORK(release_headset_event_work, release_headset_event);

static int get_t2adc_data( int ch )
{
    int ret = 0;
    struct twl4030_madc_request req;
    
    req.channels = ( 1 << ch );
    req.do_avg = 0;
    req.method = TWL4030_MADC_SW1;
    req.active = 0;
    req.func_cb = NULL;
    twl4030_madc_conversion( &req );

    ret = req.rbuf[ch];
	//printk("adc value is : %d", ret);

    return ret;
}
EXPORT_SYMBOL_GPL(get_t2adc_data);

static void ear_adc_caculrator(unsigned long arg)
{
	int adc = 0;
	int state = 0;
	
	state = gpio_get_value(EAR_DET_GPIO) ^ EAR_DETECT_INVERT_ENABLE;	

	gpio_direction_output(EAR_MIC_BIAS_GPIO, 1);
	msleep(50);
	
	if (state)
	{
	#ifdef USE_REGULATOR
		struct switch_device_info *di;
		struct platform_device *pdev;
		
		pdev = to_platform_device(this_dev);
		di = platform_get_drvdata( pdev );
		twl4030_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, VSEL_VINTANA2_2V75, VINTANA2_DEDICATED );
		twl4030_i2c_write_u8( TWL4030_MODULE_PM_RECEIVER, 0x20, VINTANA2_DEV_GRP );

		//wake_lock(&headset_wake_lock); // disallow to enter deep sleep keeping usb voltage

		ret = regulator_enable( di->usb3v1 );
		if ( ret )
			printk("Regulator 3v1 error!!\n");

		ret = regulator_enable( di->usb1v8 );
		if ( ret )
			printk("Regulator 1v8 error!!\n");

		ret = regulator_enable( di->usb1v5 );
		if ( ret )
			printk("Regulator 1v5 error!!\n");

		mdelay(5);

		//twl4030_i2c_read_u8(TWL4030_MODULE_USB, &ana_val, CARKIT_ANA_CTRL);
		twl4030_i2c_write_u8(TWL4030_MODULE_USB, SEL_MADC_MCPC, CARKIT_ANA_CTRL);
	#endif

		adc = get_t2adc_data(3);

	#ifdef USE_REGULATOR
		regulator_disable( di->usb1v5 );
        regulator_disable( di->usb1v8 );
        regulator_disable( di->usb3v1 );
	#endif

		//wake_unlock(&headset_wake_lock);

		if(adc >= 100)
		{
			#ifdef CONFIG_MACH_OSCAR
			switch_set_state(&data->sdev,1);
			#else
			switch_set_state(&data->sdev,HEADSET_4POLE_WITH_MIC);
			#endif
			headset_status = HEADSET_4POLE_WITH_MIC;
		
			#ifdef CONFIG_SEC_HEADSET_CLASS_FOR_OMS
			accessory_changed(&micco_headset_accessory);
			#endif
			printk("[Headset] 4pole ear-mic adc is %d\n", adc);
			#ifdef CONFIG_INPUT_ZEUS_EAR_KEY
			ear_key_enable_irq();
			#endif

			gpio_direction_output(EAR_ADC_SEL_GPIO , 1);
		}
		else if(adc < 100)
		{
			#ifdef CONFIG_MACH_OSCAR
			switch_set_state(&data->sdev,1);
			#else
			switch_set_state(&data->sdev, HEADSET_3POLE);
			#endif
			headset_status = HEADSET_3POLE;

			#ifdef CONFIG_SEC_HEADSET_CLASS_FOR_OMS
			accessory_changed(&micco_headset_accessory);
			//printk("[Headset] force detecting for 4pole\n", adc);
			#endif

			printk("[Headset] 3pole earphone adc is %d\n", adc);
			//#ifdef CONFIG_INPUT_ZEUS_EAR_KEY
			//ear_key_enable_irq();
			//#endif

			gpio_direction_output(EAR_ADC_SEL_GPIO , 1);
			gpio_direction_output(EAR_MIC_BIAS_GPIO, 0);
		}
		else
		{
			printk(KERN_ALERT "Wrong adc value!! adc is %d\n", adc);
			gpio_direction_output(EAR_MIC_BIAS_GPIO, 0);
			headset_status = 0;
		}
	}
	else
	{
		printk("[Headset] Headset detached\n");        	
		switch_set_state(&data->sdev, state);

		#ifdef CONFIG_SEC_HEADSET_CLASS_FOR_OMS
		accessory_changed(&micco_headset_accessory);
		#endif
		#ifdef CONFIG_INPUT_ZEUS_EAR_KEY
		//ear_key_disable_irq();
		#endif	
		if(headset_status == HEADSET_4POLE_WITH_MIC)
			release_ear_key();
		
		headset_status = HEADSET_DISCONNET;
		gpio_direction_output(EAR_MIC_BIAS_GPIO, 0);
	}

	//wake_unlock(&headset_sendend_wake_lock);
}
static DECLARE_DELAYED_WORK(ear_adc_cal_work, ear_adc_caculrator);

static void headset_detect_timer_handler(unsigned long arg)
{
	int state;
	int count = 0;
	state = gpio_get_value(EAR_DET_GPIO) ^ EAR_DETECT_INVERT_ENABLE;
	if(state)
		count = HEADSET_ATTACH_COUNT;
	else if(!state)
		count = HEADSET_DETACH_COUNT;
		
	SEC_HEADSET_DBG("Check attach state - headset_detect_timer_token is %d", headset_detect_timer_token);
	
	if(headset_detect_timer_token < count)
	{
		#if !defined(CONFIG_MACH_TICKERTAPE) && !defined(CONFIG_MACH_ZEUS)
		gpio_direction_output(EAR_ADC_SEL_GPIO , 0);
		#endif

		headset_detect_timer.expires = HEADSET_CHECK_TIME;
		add_timer(&headset_detect_timer);
		headset_detect_timer_token++;
	}
	else if(headset_detect_timer_token == count)
	{

		schedule_delayed_work(&ear_adc_cal_work, 20);
		SEC_HEADSET_DBG("add work queue - timer token is %d", count);
		headset_detect_timer_token = 0;
	}
	else
	{
		printk(KERN_ALERT "wrong headset_detect_timer_token count %d", headset_detect_timer_token);
		gpio_direction_output(EAR_MIC_BIAS_GPIO, 0);
	}

	

}
static void gpio_switch_work(struct work_struct *work)
{
	int state;
	SEC_HEADSET_DBG("");  
	//struct gpio_switch_data	*data =
		//container_of(work, struct gpio_switch_data, work);
	del_timer(&headset_detect_timer);
	cancel_delayed_work_sync(&ear_adc_cal_work);
	
	state = gpio_get_value(data->gpio) ^ EAR_DETECT_INVERT_ENABLE;

	if (state || !state)
	{
		//wake_lock(&headset_sendend_wake_lock);
		SEC_HEADSET_DBG("Headset attached timer start");
		headset_detect_timer_token = 0;
		headset_detect_timer.expires = HEADSET_CHECK_TIME;
		add_timer(&headset_detect_timer);
	}

	else
		SEC_HEADSET_DBG("Headset state does not valid. or send_end event");
}

static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
	struct gpio_switch_data *switch_data =
	    (struct gpio_switch_data *)dev_id;
	#ifdef CONFIG_INPUT_ZEUS_EAR_KEY
	int state;

	wake_lock_timeout(&headset_wake_lock, 3*HZ);

	SEC_HEADSET_DBG("");  
	state = gpio_get_value(data->gpio) ^ EAR_DETECT_INVERT_ENABLE;
	 if (!state)
	{
		printk("[Headset] disable earkey\n");        	

		ear_key_disable_irq();
	}
 	#endif
	gpio_direction_output(EAR_MIC_BIAS_GPIO, 0);
	schedule_work(&switch_data->work);
	return IRQ_HANDLED;
}

static ssize_t switch_gpio_print_state(struct switch_dev *sdev, char *buf)
{
	struct gpio_switch_data	*switch_data =
		container_of(sdev, struct gpio_switch_data, sdev);
	const char *state;
	SEC_HEADSET_DBG("");  
	if (switch_get_state(sdev))
		state = switch_data->state_on;
	else
		state = switch_data->state_off;

	if (state)
		return sprintf(buf, "%s\n", state);
	return -1;
}

static int gpio_switch_probe(struct platform_device *pdev)
{
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct gpio_switch_data *switch_data;
	int ret = 0;
	SEC_HEADSET_DBG("");  
	this_dev = &pdev->dev;
#if !defined(CONFIG_MACH_TICKERTAPE) && !defined(CONFIG_MACH_ZEUS)
	if (gpio_request(EAR_ADC_SEL_GPIO , "TVOUT_SEL") == 0) 
	{
		gpio_direction_output(EAR_ADC_SEL_GPIO , 0);
	}
#endif
	if (gpio_request(EAR_MIC_BIAS_GPIO, "EARMIC") == 1) 
	{
		gpio_direction_output(EAR_MIC_BIAS_GPIO, 0);
	}

	if (!pdata)
		return -EBUSY;

	switch_data = kzalloc(sizeof(struct gpio_switch_data), GFP_KERNEL);
	if (!switch_data)
		return -ENOMEM;

	switch_data->sdev.name = pdata->name;
	switch_data->gpio = pdata->gpio;
	switch_data->name_on = pdata->name_on;
	switch_data->name_off = pdata->name_off;
	switch_data->state_on = pdata->state_on;
	switch_data->state_off = pdata->state_off;
	switch_data->sdev.print_state = switch_gpio_print_state;

    ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0)
		goto err_switch_dev_register;

	//[2009.12.18 changoh.heo for oms
	#ifdef CONFIG_SEC_HEADSET_CLASS_FOR_OMS
	ret = accessory_register(&pdev->dev, &micco_headset_accessory);
	if (ret < 0)
		goto err_switch_dev_register;
	#endif
	//]

	ret = gpio_request(switch_data->gpio, pdev->name);
	if (ret < 0)
		goto err_request_gpio;

	ret = gpio_direction_input(switch_data->gpio);
	if (ret < 0)
		goto err_set_gpio_input;

	INIT_WORK(&switch_data->work, gpio_switch_work);

	switch_data->irq = gpio_to_irq(switch_data->gpio);
	if (switch_data->irq < 0) {
		ret = switch_data->irq;
		goto err_detect_irq_num_failed;
	}

	ret = request_irq(switch_data->irq, gpio_irq_handler,
			  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			  pdev->name, 
			  switch_data); //Iyyappan_HS
	if (ret < 0)
		goto err_request_irq;
	data = switch_data;	

	init_timer(&headset_detect_timer);
	headset_detect_timer.function = headset_detect_timer_handler;

	wake_lock_init(&headset_wake_lock, WAKE_LOCK_SUSPEND, "headset");

	enable_irq_wake(switch_data->irq);

	/* Perform initial detection */
	gpio_switch_work(&switch_data->work);

	return 0;

err_request_irq:
err_detect_irq_num_failed:
err_set_gpio_input:
	gpio_free(switch_data->gpio);
err_request_gpio:
    switch_dev_unregister(&switch_data->sdev);
	//[2009.12.18 changoh.heo for oms
	#ifdef CONFIG_SEC_HEADSET_CLASS_FOR_OMS
	accessory_unregister(&micco_headset_accessory);
	#endif
	//]
err_switch_dev_register:
	kfree(switch_data);

	return ret;
}

static int __devexit gpio_switch_remove(struct platform_device *pdev)
{
	struct gpio_switch_data *switch_data = platform_get_drvdata(pdev);
	SEC_HEADSET_DBG("");
	cancel_work_sync(&switch_data->work);
	gpio_free(switch_data->gpio);
    switch_dev_unregister(&switch_data->sdev);
	#ifdef CONFIG_SEC_HEADSET_CLASS_FOR_OMS
	accessory_unregister(&micco_headset_accessory);
	#endif
	kfree(switch_data);

	return 0;
}

static struct platform_driver gpio_switch_driver = {
	.probe		= gpio_switch_probe,
	.remove		= __devexit_p(gpio_switch_remove),
	.driver		= {
		.name	= "switch-gpio",
		.owner	= THIS_MODULE,
	},
};

static int __init gpio_switch_init(void)
{
#if !defined(CONFIG_MACH_TICKERTAPE) && !defined(CONFIG_MACH_ZEUS)
	gpio_free(EAR_ADC_SEL_GPIO );
#endif
	gpio_free(EAR_MIC_BIAS_GPIO);
	return platform_driver_register(&gpio_switch_driver);
}

static void __exit gpio_switch_exit(void)
{
	platform_driver_unregister(&gpio_switch_driver);
}

#ifndef MODULE
late_initcall(gpio_switch_init);
#else
module_init(gpio_switch_init);
#endif
module_exit(gpio_switch_exit);

MODULE_AUTHOR("Mike Lockwood <lockwood@android.com>");
MODULE_DESCRIPTION("GPIO Switch driver");
MODULE_LICENSE("GPL");
