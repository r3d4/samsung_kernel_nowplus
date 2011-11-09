/*
 * rtc-virtual.c -- Virtual Real Time Clock interface
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/rtc.h>
#include <linux/bcd.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>

/*----------------------------------------------------------------------*/

static int virtual_rtc_alarm_irq_enable(struct device *dev, unsigned enabled)
{
	return 0;
}

static int virtual_rtc_update_irq_enable(struct device *dev, unsigned enabled)
{
	return 0;
}

static int virtual_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	return 0;
}

static int virtual_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	char date_string[50]; 
	char time_string[50]; 
	char mode_string[50];
	char *envp[3];
	int env_offset = 0;
	int ret = 0;

	printk("%s, %04d-%02d-%02d %02d:%02d:%02d\n", __func__, 
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	snprintf(mode_string, sizeof(mode_string), "TIME_MODE=R");
	envp[env_offset++] = mode_string;
	snprintf(date_string, sizeof(date_string), "RTC_DATE=%04d-%02d-%02d", 
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	envp[env_offset++] = date_string;
	snprintf(time_string, sizeof(date_string), "RTC_TIME=%02d:%02d:%02d", 
		tm->tm_hour, tm->tm_min, tm->tm_sec);
	envp[env_offset++] = time_string;
	
//	printk("%s, change uevent \n", __func__);
//	kobject_uevent(&dev->kobj, KOBJ_CHANGE);
	
	kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);

	return ret;
}

/*
 * Gets current virtual RTC alarm time.
 */
static int virtual_rtc_read_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
	return 0;
}

static int virtual_rtc_set_alarm(struct device *dev, struct rtc_wkalrm *alm)
{
//	char date_string[50]; 
//	char time_string[50]; 
//	char clear_string[50];
	char alarm_data[50];
	char mode_string[50];
//	char *envp[3];
	char *envp[2];
	int env_offset = 0;
	int ret = 0;

	if(alm->enabled == 1)
	{
		printk("%s, alarm is enabled.\n", __func__);
	}
	else
	{
		printk("%s, alarm is disabled.\n", __func__);
	}

//	printk("%s, %04d-%02d-%02d %02d:%02d:%02d\n", __func__, 
//		alm->time.tm_year + 1900, alm->time.tm_mon + 1, alm->time.tm_mday, alm->time.tm_hour, alm->time.tm_min, alm->time.tm_sec);

//	snprintf(clear_string, sizeof(clear_string), "ALARM_ENABLED=%c", alm->enabled?'Y':'N');
//	envp[env_offset++] = clear_string;
//	snprintf(date_string, sizeof(date_string), "ALARM_DATE=%04d-%02d-%02d", 
//		alm->time.tm_year + 1900, alm->time.tm_mon + 1, alm->time.tm_mday);
//	envp[env_offset++] = date_string;
//	snprintf(time_string, sizeof(date_string), "ALARM_TIME=%02d:%02d:%02d", 
//		alm->time.tm_hour, alm->time.tm_min, alm->time.tm_sec);
//	envp[env_offset++] = time_string;
	snprintf(mode_string, sizeof(mode_string), "TIME_MODE=A");
	envp[env_offset++] = mode_string;
	snprintf(alarm_data, sizeof(alarm_data), "ALARM_DATA=%c%04d-%02d-%02d+%02d:%02d:%02d", 
		alm->enabled?'Y':'N', alm->time.tm_year + 1900, alm->time.tm_mon + 1, alm->time.tm_mday,
		alm->time.tm_hour, alm->time.tm_min, alm->time.tm_sec);
	envp[env_offset++] = alarm_data;

	printk("%s, Set kobj env : %s, %s, %s, env_offset=%d\n", __func__, envp[0], envp[1], envp[2], env_offset);

//	kobject_uevent(&dev->kobj, KOBJ_CHANGE);
	kobject_uevent_env(&dev->kobj, KOBJ_CHANGE, envp);

	return ret;
}

static struct rtc_class_ops virtual_rtc_ops = {
	.read_time	= virtual_rtc_read_time,
	.set_time	= virtual_rtc_set_time,
	.read_alarm	= virtual_rtc_read_alarm,
	.set_alarm	= virtual_rtc_set_alarm,
	.alarm_irq_enable = virtual_rtc_alarm_irq_enable,
	.update_irq_enable = virtual_rtc_update_irq_enable,
};

/*----------------------------------------------------------------------*/

static int __devinit virtual_rtc_probe(struct platform_device *pdev)
{
	struct rtc_device *rtc;
	int ret = 0;

	printk("%s, Virtual RTC driver\n", __func__);
	rtc = rtc_device_register(pdev->name,
				  &pdev->dev, &virtual_rtc_ops, THIS_MODULE);
	if (IS_ERR(rtc)) {
		ret = PTR_ERR(rtc);
		dev_err(&pdev->dev, "can't register RTC device, err %ld\n",
			PTR_ERR(rtc));
		goto out0;

	}

	platform_set_drvdata(pdev, rtc);

	return ret;

//out1:
//	rtc_device_unregister(rtc);
out0:
	return ret;
}

static int __devexit virtual_rtc_remove(struct platform_device *pdev)
{
	/* leave rtc running, but disable irqs */
	struct rtc_device *rtc = platform_get_drvdata(pdev);

	rtc_device_unregister(rtc);
	platform_set_drvdata(pdev, NULL);
	return 0;
}

static void virtual_rtc_shutdown(struct platform_device *pdev)
{
}

#ifdef CONFIG_PM

//static unsigned char irqstat;

static int virtual_rtc_suspend(struct platform_device *pdev, pm_message_t state)
{
	return 0;
}

static int virtual_rtc_resume(struct platform_device *pdev)
{
	return 0;
}

#else
#define virtual_rtc_suspend NULL
#define virtual_rtc_resume NULL
#endif

MODULE_ALIAS("platform:virtual_rtc");

static struct platform_driver virtual_rtc_driver = {
	.probe		= virtual_rtc_probe,
	.remove		= __devexit_p(virtual_rtc_remove),
	.shutdown	= virtual_rtc_shutdown,
	.suspend	= virtual_rtc_suspend,
	.resume		= virtual_rtc_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "virtual_rtc",
	},
};

static int __init virtual_rtc_init(void)
{
        printk("virtual_rtc_init\n");
	return platform_driver_register(&virtual_rtc_driver);
}
module_init(virtual_rtc_init);

static void __exit virtual_rtc_exit(void)
{
	platform_driver_unregister(&virtual_rtc_driver);
}
module_exit(virtual_rtc_exit);

MODULE_AUTHOR( "TAESOON IM <ts.im@samsung.com>" );
MODULE_LICENSE("GPL");
