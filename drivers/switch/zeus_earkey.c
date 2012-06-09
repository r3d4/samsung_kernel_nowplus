/* This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/switch.h>
#include <plat/gpio.h>
#include <linux/i2c/twl4030-madc.h>
#include <linux/i2c/twl.h>
#include <linux/delay.h>

#include <mach/hardware.h>
#include <plat/mux.h>

#include "switch_omap_gpio.h"
//#define CONFIG_DEBUG_SEC_HEADSET
#ifdef CONFIG_DEBUG_SEC_HEADSET
#define SEC_HEADSET_DBG(fmt, arg...) printk(KERN_INFO "[JACKKEY] %s" fmt "\r\n", __func__,## arg)
#else
#define SEC_HEADSET_DBG(fmt, arg...) do {} while(0)
#endif

#define KEYCODE_SENDEND 231
#define KEYCODE_HEADSETHOOK 226


#define SEND_END_CHECK_COUNT	3
#define SEND_END_CHECK_TIME get_jiffies_64() + 6 //30ms

struct switch_dev switch_sendend = {
        .name = "send_end",
};
static int __devinit ear_key_driver_probe(struct platform_device *plat_dev);
static irqreturn_t earkey_press_handler(int irq_num, void * dev);

static int get_adc_data( int ch )
{
	int ret = 0;
	struct twl4030_madc_request req;
	SEC_HEADSET_DBG("  ");
    
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


static struct timer_list send_end_key_event_timer;
static unsigned int send_end_key_timer_token;
static unsigned int send_end_irq_token;
static unsigned int check_adc;
static unsigned int adc_buffer[SEND_END_CHECK_COUNT];
static unsigned int count = 0;
static unsigned int fast_adc_val;
static unsigned int fast_adc_avg_val;

struct input_dev *ip_dev = NULL;

void ear_key_disable_irq(void)
{
	SEC_HEADSET_DBG("  ");
	if(send_end_irq_token > 0)
	{
		//disable_irq(EAR_KEY_GPIO);
		send_end_irq_token--;
	}else{
		printk("[EAR_KEY]Err!! send_end_irq_toke is < 0\n");
	}
}
EXPORT_SYMBOL_GPL(ear_key_disable_irq);

void ear_key_enable_irq(void)
{
	SEC_HEADSET_DBG("  ");
	//enable_irq(EAR_KEY_GPIO);
	send_end_irq_token++;
}
EXPORT_SYMBOL_GPL(ear_key_enable_irq);
static unsigned int earkey_stats = 0;
static void release_sysfs_event(struct work_struct *work)
{
    int i = 0;
    bool result = false;
  
	SEC_HEADSET_DBG(" %d ", earkey_stats);

    for(i = 0 ; i < SEND_END_CHECK_COUNT ; i++)
    {
        if(adc_buffer[i] >= 75)
        {
            result = false;
            break;
        }
        else
        {
            result = true;
        }
    }

    if((check_adc >= 2 ) || ((fast_adc_avg_val < 100) && (result == true)))
	{
		input_report_key(ip_dev,KEYCODE_HEADSETHOOK,1);
		input_sync(ip_dev);	
	}

	check_adc = 0;
    count = 0;

    for(i = 0 ; i < SEND_END_CHECK_COUNT ; i++)
        adc_buffer[i] = 0;
	
	if(earkey_stats)
		switch_set_state(&switch_sendend, 1);
	else
		switch_set_state(&switch_sendend, 0);

	//gpio_direction_output(EAR_ADC_SEL_GPIO , 1);

}
static DECLARE_DELAYED_WORK(release_sysfs_event_work, release_sysfs_event);


static void check_key_adc(struct work_struct *work)
{
	int adc_val;

	SEC_HEADSET_DBG("  ");
	adc_val = get_adc_data(3);
	//printk("- Earkey pressed ADC %d\n" , adc_val);
	
	if(adc_val <= 150 && adc_val >= 0)
	{
		printk("[Earkey]adc is within %d\n", adc_val);
		check_adc++;
	}
	else
	{
		printk("[Earkey]adc is NOT within %d\n", adc_val);
	}

	adc_buffer[count++] = adc_val;
}
static DECLARE_DELAYED_WORK(check_key_adc_work, check_key_adc);

static void fast_check_key_adc(struct work_struct *work)
{
	fast_adc_val = 0;
    fast_adc_avg_val = 0;
    
	SEC_HEADSET_DBG("  ");

    //gpio_direction_output(EAR_ADC_SEL_GPIO , 0);	
    
	fast_adc_val = get_adc_data(3);
	//printk("fast_check_key_adc() ADC value is = %d\n" , fast_adc_val);

    fast_adc_val += get_adc_data(3);
	//printk("fast_check_key_adc() ADC value is = %d\n" , fast_adc_val);

	fast_adc_avg_val = fast_adc_val / 2;
	printk("fast_key_adc() avg_val is = %d\n" , fast_adc_avg_val);
	
    //gpio_direction_output(EAR_ADC_SEL_GPIO , 1);		
}
static DECLARE_WORK(fast_check_key_adc_work, fast_check_key_adc);

void release_ear_key(void)
{
	SEC_HEADSET_DBG("  ");
	if(earkey_stats)
	{
		printk("Headset detached and earkey was pressed!\n");
		input_report_key(ip_dev,KEYCODE_HEADSETHOOK,0);
  		input_sync(ip_dev);
		printk("SEND/END %s.\n", "released");
		earkey_stats = 0;
		switch_set_state(&switch_sendend, 0);
		//gpio_direction_output(EAR_ADC_SEL_GPIO , 1);
	}
}
EXPORT_SYMBOL_GPL(release_ear_key);

static void send_end_key_event_timer_handler(unsigned long arg)
{
	int sendend_state = 0;
//	int adc_val = 0;	
  int i = 0;

	SEC_HEADSET_DBG("  ");
	sendend_state = gpio_get_value(EAR_KEY_GPIO) ^ EAR_KEY_INVERT_ENABLE;

	if((get_headset_status() == HEADSET_4POLE_WITH_MIC) && sendend_state)
	{
		if(send_end_key_timer_token < SEND_END_CHECK_COUNT)
		{	
			send_end_key_timer_token++;
			send_end_key_event_timer.expires = SEND_END_CHECK_TIME; 
			add_timer(&send_end_key_event_timer);

			schedule_delayed_work(&check_key_adc_work, 0);
	    
			SEC_HEADSET_DBG("SendEnd Timer Restart %d", send_end_key_timer_token);
		}
		else if(send_end_key_timer_token == SEND_END_CHECK_COUNT)
		{
			printk("SEND/END is pressed\n");
			earkey_stats = 1;
			schedule_delayed_work(&release_sysfs_event_work, 0);
			send_end_key_timer_token = 0;
		}
		else
		{
			printk(KERN_ALERT "[Headset]wrong timer counter %d\n", send_end_key_timer_token);
			//gpio_direction_output(EAR_ADC_SEL_GPIO , 1);
		}
	}
	else
	{		
		printk(KERN_ALERT "[Headset]GPIO Error\n %d, %d", get_headset_status(), sendend_state);
		check_adc = 0;
    count = 0;
    
  	for(i = 0 ; i < SEND_END_CHECK_COUNT ; i++)
  	  adc_buffer[i] = 0;

		//gpio_direction_output(EAR_ADC_SEL_GPIO , 1);
	}
}

static void ear_switch_change(struct work_struct *ignored)
{
	int ear_state = 0;

	SEC_HEADSET_DBG("");
	if(!ip_dev){
    		dev_err(ip_dev->dev.parent,"Input Device not allocated\n");
    		return;
  	}
  
  	ear_state = gpio_get_value(EAR_KEY_GPIO) ^ EAR_KEY_INVERT_ENABLE;
	
  	if( ear_state < 0 ){
    	dev_err(ip_dev->dev.parent,"Failed to read GPIO value\n");
    	return;
  	}

	del_timer(&send_end_key_event_timer);
	send_end_key_timer_token = 0;	

	//gpio_direction_output(EAR_ADC_SEL_GPIO , 0);

	if((get_headset_status() == HEADSET_4POLE_WITH_MIC) && send_end_irq_token)//  4 pole headset connected && send irq enable
	{
		if(ear_state)
		{
			send_end_key_event_timer.expires = SEND_END_CHECK_TIME; // 10ms ??
			add_timer(&send_end_key_event_timer);		
			SEC_HEADSET_DBG("SEND/END %s.timer start \n", "pressed");
			
		}else{
			SEC_HEADSET_DBG(KERN_ERR "SISO:sendend isr work queue\n");    			
		 	input_report_key(ip_dev,KEYCODE_HEADSETHOOK,0);
  			input_sync(ip_dev);
			printk("SEND/END %s.\n", "released");
			earkey_stats = 0;
			switch_set_state(&switch_sendend, 0);
			//gpio_direction_output(EAR_ADC_SEL_GPIO , 1);
		}

	}else{
		SEC_HEADSET_DBG("SEND/END Button is %s but headset disconnect or irq disable.\n", ear_state?"pressed":"released");
	}

  return;
}
static DECLARE_WORK(ear_switch_work, ear_switch_change);

static irqreturn_t earkey_press_handler(int irq_num, void * dev)
{
	SEC_HEADSET_DBG("earkey isr");
	if(send_end_irq_token)
	{
		if((gpio_get_value(EAR_KEY_GPIO) ^ EAR_KEY_INVERT_ENABLE))
			schedule_work(&fast_check_key_adc_work);

		schedule_work(&ear_switch_work);
	}
	else
	{
		printk("send end irq ries but token is null\n");
	}
	return IRQ_HANDLED;
}

static int __devinit ear_key_driver_probe(struct platform_device *plat_dev)
{
  struct input_dev *ear_key=NULL;
  int ear_key_irq=-1, err=0;

  SEC_HEADSET_DBG("");
  ear_key_irq = platform_get_irq(plat_dev, 0);
  if(ear_key_irq <= 0 ){
    dev_err(&plat_dev->dev,"failed to map the ear key to an IRQ %d\n",ear_key_irq);
    err = -ENXIO;
    return err;
  }
  ear_key = input_allocate_device();
  if(!ear_key)
  {
    dev_err(&plat_dev->dev,"failed to allocate an input devd %d \n",ear_key_irq);
    err = -ENOMEM;
    return err;
  }
  err = request_irq(ear_key_irq, &earkey_press_handler ,IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
                    "ear_key_driver",ear_key);
  if(err) {
    dev_err(&plat_dev->dev,"failed to request an IRQ handler for num %d\n",ear_key_irq);
    goto free_input_dev;
  }
  dev_dbg(&plat_dev->dev,"\n ear Key Drive:Assigned IRQ num %d SUCCESS \n",ear_key_irq);
  /* register the input device now */
  set_bit(EV_SYN,ear_key->evbit);
  set_bit(EV_KEY,ear_key->evbit);
  set_bit(KEYCODE_HEADSETHOOK, ear_key->keybit);

  ear_key->name = "ear_key_driver";
  ear_key->phys = "ear_key_driver/input0";
  ear_key->dev.parent = &plat_dev->dev;
  platform_set_drvdata(plat_dev, ear_key);

  err = input_register_device(ear_key);
  if (err) {
    dev_err(&plat_dev->dev, "ear key couldn't be registered: %d\n", err);
    goto release_irq_num;
  }
  	err = switch_dev_register(&switch_sendend);
	if (err < 0) {
		printk(KERN_ERR "SEC HEADSET: Failed to register switch sendend device\n");
		goto free_input_dev;
        }
	//disable_irq(EAR_KEY_GPIO);

	init_timer(&send_end_key_event_timer);
	send_end_key_event_timer.function = send_end_key_event_timer_handler;
	ip_dev = ear_key;

  return 0;

release_irq_num:
	free_irq(ear_key_irq,NULL); //pass devID as NULL as device registration failed 

free_input_dev:
	input_free_device(ear_key);

return err;

}

static int __devexit ear_key_driver_remove(struct platform_device *plat_dev)
{
  //struct input_dev *ip_dev= platform_get_drvdata(plat_dev);
	int ear_key_irq=0;

	SEC_HEADSET_DBG("");
	ear_key_irq = platform_get_irq(plat_dev,0);
  
	free_irq(ear_key_irq,ip_dev);
	switch_dev_unregister(&switch_sendend);
	input_unregister_device(ip_dev);  

	 return 0;
}
struct platform_driver ear_key_driver_t = {
	.probe          = &ear_key_driver_probe,
	.remove         = __devexit_p(ear_key_driver_remove),
	.driver         = {
		.name   = "ear_key_device", 
		.owner  = THIS_MODULE,
	},
};

static int __init ear_key_driver_init(void)
{
        return platform_driver_register(&ear_key_driver_t);
}
module_init(ear_key_driver_init);

static void __exit ear_key_driver_exit(void)
{
        platform_driver_unregister(&ear_key_driver_t);
}
module_exit(ear_key_driver_exit);

MODULE_ALIAS("platform:ear key driver");
MODULE_DESCRIPTION("board zeus ear key");
MODULE_LICENSE("GPL");

