#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/workqueue.h>
#include <linux/slab.h>

//#include "m4mo.h"


#define TORCH_NAME          "torch-flash"  



extern u8 m4mo_is_init___;

int m4mo_enable_flashlight(u8 level);
int m4mo_disable_flashlight(void);

static int led_state =LED_OFF;



static void nowplus_led_set(struct led_classdev *led_cdev, enum led_brightness value)
{
printk("[TORCH] nowplus_led_set, val=%d\n", value);
#if 1
    if(!m4mo_is_init___)
    {
        printk("[TORCH] error m4mo is in not initialized\n");
        return;
    }
    
	// printk("[LED] %s ::: value=%d , led_state=%d\n", __func__, value, led_state);
	if ((value == LED_OFF)&&( led_state!=LED_OFF ))
	{
		printk("[TORCH] %s ::: OFF \n", __func__);
        m4mo_disable_flashlight();
        led_state = LED_OFF;
	}
	else if ((value != LED_OFF)&&(led_state==LED_OFF))
	{
		printk("[TORCH] %s ::: ON \n", __func__);
        m4mo_enable_flashlight(value);
        led_state = value;
	}
	else
	{ 
		printk("[TORCH] %s ::: same state ... \n", __func__);
		return ;
	}
#endif
}

static enum led_brightness nowplus_led_get(struct led_classdev *cdev)
{
	return led_state;
}

struct led_classdev nowplus_torch = {
	.name = TORCH_NAME,
	.brightness_set = nowplus_led_set,
	.brightness_get = nowplus_led_get,
//	.default_trigger = "nand-disk",
};


