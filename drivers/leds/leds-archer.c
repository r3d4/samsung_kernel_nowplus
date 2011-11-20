
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>

#include <linux/workqueue.h>

#include <linux/i2c/twl.h>
#include <linux/slab.h>

#include <plat/mux.h>
#include <plat/mux_archer_rev_r03.h>
#include <mach/gpio.h>
#include <mach/archer.h>

#define DRIVER_NAME "secLedDriver"

struct archer_led_data {
	struct led_classdev cdev;
	unsigned gpio;
//	struct work_struct work;
	u8 new_level;
	u8 can_sleep;
//	u8 active_low;
//	int (*platform_gpio_blink_set)(unsigned gpio,
//			unsigned long *delay_on, unsigned long *delay_off);
};

struct regulator *vmmc2;
static int led_state =0;
extern u32 hw_revision;

static void archer_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct archer_led_data *led_dat =
		container_of(led_cdev, struct archer_led_data, cdev);
	int ret = 0;
    
	// printk("[LED] %s ::: value=%d , led_state=%d\n", __func__, value, led_state);
	if ((value == LED_OFF)&&( led_state==1 ))
	{
		if(hw_revision < 3 ) 
			ret = regulator_disable( vmmc2 );
		else
                   gpio_direction_output(led_dat->gpio, 0);
		led_state = 0;
		printk("[LED] %s ::: OFF \n", __func__);
	}
	else if ((value != LED_OFF)&&(led_state==0))
	{
		if(hw_revision < 3 ) 
			ret = regulator_enable( vmmc2 );
		else
		gpio_direction_output(led_dat->gpio, 1);
		led_state = 1;
		printk("[LED] %s ::: ON \n", __func__);
	}
	else
	{ 
		// printk("[LED] %s ::: same state ... \n", __func__);
		return ;
	}
	if( ret && (hw_revision < 3))
			printk("Regulator vmmc2 error!!\n");

}

static int archer_led_probe(struct platform_device *pdev)
{	

	struct led_platform_data *pdata = pdev->dev.platform_data;
	struct led_info *cur_led;
	struct archer_led_data *leds_data, *led_dat;
	
	int i, ret = 0;

	printk("[LED] %s +\n", __func__);

	if (!pdata)
		return -EBUSY;

	leds_data = kzalloc(sizeof(struct archer_led_data) * pdata->num_leds,
				GFP_KERNEL);
	if (!leds_data)
		return -ENOMEM;

	for (i = 0; i < pdata->num_leds; i++) {
		cur_led = &pdata->leds[i];
		led_dat = &leds_data[i];
		printk("[LED] %s hw_revision=%d \n", __func__, hw_revision);
		if(hw_revision < 3 ) 
		{
			vmmc2 = regulator_get( &pdev->dev, "vmmc2" );
			if( IS_ERR( vmmc2 ) )
			{
				printk("PSENSOR: %s: regulator_get failed to get vmmc2!\n", __func__);
				goto err;
			}
		}
		else 
		{
			ret = gpio_request(OMAP_GPIO_LED_EN, cur_led->name);
			if (ret < 0)
				goto err;
			led_dat->gpio = OMAP_GPIO_LED_EN;
			led_dat->can_sleep = gpio_cansleep(OMAP_GPIO_LED_EN);
			gpio_direction_output(OMAP_GPIO_LED_EN, 1);

			ret = twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0x00, TWL4030_VMMC2_DEV_GRP);
			if (ret)
				printk(" leds-archer.c fail to set reousrce off TWL4030_VMMC2_DEV_GRP\n");
		}


		led_dat->cdev.name = cur_led->name;
		led_dat->cdev.default_trigger = cur_led->default_trigger;
		led_dat->cdev.brightness_set = archer_led_set;
		led_dat->cdev.brightness = LED_OFF;
		led_dat->cdev.flags |= LED_CORE_SUSPENDRESUME;

//		gpio_direction_output(led_dat->gpio, led_dat->active_low);

//		INIT_WORK(&led_dat->work, gpio_led_work);

		ret = led_classdev_register(&pdev->dev, &led_dat->cdev);
		if (ret < 0) {
//			gpio_free(led_dat->gpio);
			regulator_put( vmmc2 );
			goto err;
		}
	}

	platform_set_drvdata(pdev, leds_data);

	return 0;

err:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			led_classdev_unregister(&leds_data[i].cdev);
//			cancel_work_sync(&leds_data[i].work);
//			gpio_free(leds_data[i].gpio);
		}
	}

	kfree(leds_data);

	return ret;
}

static int __devexit archer_led_remove(struct platform_device *pdev)
{
	int i;
//	struct gpio_led_platform_data *pdata = pdev->dev.platform_data;
//	struct gpio_led_data *leds_data;
	struct led_platform_data *pdata = pdev->dev.platform_data;
	struct archer_led_data *leds_data;

	regulator_put( vmmc2 );

	leds_data = platform_get_drvdata(pdev);

	for (i = 0; i < pdata->num_leds; i++) {
		led_classdev_unregister(&leds_data[i].cdev);
//		cancel_work_sync(&leds_data[i].work);
//		gpio_free(leds_data[i].gpio);
	}	

	kfree(leds_data);

	return 0;
}

#if 0
static int archer_led_suspend(struct platform_device *dev, pm_message_t state)
{
	struct led_platform_data *pdata = dev->dev.platform_data;
	struct archer_led_data *leds_data;
	int i;
  
	printk("[LED] %s +\n", __func__);  
	printk("[LED] %s nuim_leds=%d \n", __func__, pdata->num_leds);
	for (i = 0; i < pdata->num_leds; i++) 
		led_classdev_suspend(&leds_data[i].cdev);

	return 0;
}

static int archer_led_resume(struct platform_device *dev)
{
	struct led_platform_data *pdata = dev->dev.platform_data;
	struct archer_led_data *leds_data;
	int i;

	printk("[LED] %s +\n", __func__);

	for (i = 0; i < pdata->num_leds; i++) 
		led_classdev_resume(&leds_data[i].cdev);

	return 0;
}
#endif

static struct platform_driver archer_led_driver = {
	.probe		= archer_led_probe,
	.remove		= __devexit_p(archer_led_remove),
//	.suspend		= archer_led_suspend,
//	.resume		= archer_led_resume,
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},	
};

static int __init archer_led_init(void)
{
	printk("[LED] %s +\n", __func__);
	return platform_driver_register(&archer_led_driver);
}

static void __exit archer_led_exit(void)
{
	platform_driver_unregister(&archer_led_driver);
}

module_init(archer_led_init);
module_exit(archer_led_exit);

MODULE_DESCRIPTION("VMMC LED driver");
MODULE_LICENSE("GPL");

