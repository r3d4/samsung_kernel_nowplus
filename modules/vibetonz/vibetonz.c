/*****************************************************************************
  **
  ** COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2015 ALL RIGHTS RESERVED
  **
  *****************************************************************************/

#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/gpio.h>
#include <linux/irq.h>

//#include <linux/mutex.h>
//#include <linux/semaphore.h>

#include <asm/irq.h>
#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/leds.h>
#include <asm/mach/irq.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/timed_output.h>

#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <asm/delay.h>
#include <plat/board.h>
#include <plat/dmtimer.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <asm/mach/time.h>
#include <mach/io.h>
#include <asm/mach-types.h>
#include <plat/mux.h>

#include "vibrator.h"
#define DRIVER_NAME "secVibrator"

/*********** vibrator debug ****************************************************/
#if 0
#define gprintk(fmt, x... ) printk( "%s(%d): " fmt, __FUNCTION__ ,__LINE__, ## x)
#else
#define gprintk(x...) do { } while (0)
#endif
/*******************************************************************************/

#define PWM_DUTY_MAX	580  /*For 100%Duty Cycle*/
#define PWM_DUTY_VALUE	580  /*For 100%Duty Cycle*/ // for max cycle   500->580

/* Error and Return value codes */
#define VIBE_S_SUCCESS	0	/*!< Success */
#define VIBE_E_FAIL		-4	/*!< Generic error */

#define GPIO_LEVEL_LOW				0
#define GPIO_LEVEL_HIGH				1

#define TIMER_DISABLED				0
#define TIMER_ENABLED				1

static bool g_bAmpEnabled = false;
static int max_timeout = 5000;
static int vibrator_value = 0;

struct vibrator_device_info 
{
    struct device  *dev;
};
static int __devinit vibrator_probe( struct platform_device * );
static int __devexit vibrator_remove( struct platform_device * );
static int vibrator_suspend( struct platform_device *, pm_message_t );
static int vibrator_resume( struct platform_device * );

typedef struct
{
	int  state;
	//struct  mutex lock;
	spinlock_t vib_lock;
}timer_state_t;
timer_state_t timer_state;

/*
 * variable For High Resolution Timer "hrtimer"
 */
static struct hrtimer h_timer;

/*
 * For OMAP3430 "GPtimer9"
 */
static struct omap_dm_timer *gptimer;	/*For OMAP3430 "gptimer9"*/

static VibeStatus ImmVibeSPI_ForceOut_Set(VibeInt8 nForce);

static irqreturn_t omap2_gp_timer_interrupt(int irq,void *dev_id)
{
	unsigned long flags;
	struct omap_dm_timer *gpt = NULL;
	
	spin_lock_irqsave(&(timer_state.vib_lock), flags);
    
	gpt = (struct omap_dm_timer *)dev_id;	
	if(omap_dm_timer_get_irq(gpt)) {
		omap_dm_timer_write_status(gpt,OMAP_TIMER_INT_OVERFLOW);
	}

	spin_unlock_irqrestore(&(timer_state.vib_lock), flags);

	spin_lock_irqsave(&(timer_state.vib_lock), flags);

	if(omap_dm_timer_get_irq(gpt)) {
		omap_dm_timer_write_status(gpt,OMAP_TIMER_INT_MATCH);
	}

	spin_unlock_irqrestore(&(timer_state.vib_lock), flags);

	return IRQ_HANDLED;
}

static struct irqaction omap2_gp_timer_irq={
	.name="gptimer9",
	.flags=IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler=omap2_gp_timer_interrupt,
};

/*
 *  For requesting the "GPtimer9" also for setting the default value in registers
 */
static int ReqGPTimer(void)
{
	int ret;

//	printk("[VIBRATOR] %s \n",__func__);
	
	gptimer=omap_dm_timer_request_specific(9);

	if (gptimer == NULL) {
//		printk("failed to request pwm timer\n");
		ret = -ENODEV;
	}

	omap_dm_timer_enable(gptimer);
	omap_dm_timer_set_source(gptimer, OMAP_TIMER_SRC_SYS_CLK);
	omap_dm_timer_set_load(gptimer, 1, 0xffffff00);

	/*
	   Change for Interrupt
	 */
	omap2_gp_timer_irq.dev_id=(void *)gptimer;
	setup_irq(omap_dm_timer_get_irq(gptimer),&omap2_gp_timer_irq);		/* Request for interrupt Number */
	omap_dm_timer_set_int_enable(gptimer,OMAP_TIMER_INT_OVERFLOW);
	omap_dm_timer_set_int_enable(gptimer,OMAP_TIMER_INT_MATCH);
	omap_dm_timer_stop(gptimer);
	omap_dm_timer_disable(gptimer);
	return 0;
}

/*
 * For setting the values in registers for varying the duty cycle and time period
 */
static void GPTimerSetValue(unsigned long load,unsigned long cmp)
{
	unsigned long load_reg, cmp_reg;
//	printk("[VIBRATOR] %s \n",__func__);

	load_reg =  load;	/* For setting the frequency=22.2Khz */
	cmp_reg =  cmp;		/* For varying the duty cycle */

	omap_dm_timer_set_load(gptimer, 1, -load_reg);
	omap_dm_timer_set_match(gptimer, 1, -cmp_reg);
	omap_dm_timer_set_pwm(gptimer, 0, 1,OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);
	omap_dm_timer_write_counter(gptimer, -2);
}


/*
 *  Called to disable amp (disable output force)
 */
static VibeStatus ImmVibeSPI_ForceOut_AmpDisable(void)
{
//	printk("[VIBRATOR] %s \n",__func__);

	if (g_bAmpEnabled) {
//		printk(KERN_DEBUG "ImmVibeSPI_ForceOut_AmpDisable.\n");
		g_bAmpEnabled = false;
		gpio_set_value(OMAP_GPIO_MOTOR_EN, GPIO_LEVEL_LOW);
	}
	return VIBE_S_SUCCESS;
}

/*
 * Called to enable amp (enable output force)
 */
static VibeStatus ImmVibeSPI_ForceOut_AmpEnable(void)
{
//	printk("[VIBRATOR] %s \n",__func__);

	if (!g_bAmpEnabled) {
//		printk(KERN_DEBUG "ImmVibeSPI_ForceOut_AmpEnable.\n");
		g_bAmpEnabled = true;
		gpio_set_value(OMAP_GPIO_MOTOR_EN, GPIO_LEVEL_HIGH);
	}
	return VIBE_S_SUCCESS;
}

/*
 * Called at initialization time to set PWM freq, disable amp, etc...
 */
static VibeStatus ImmVibeSPI_ForceOut_Initialize(void)
{
//	printk("[VIBRATOR] %s \n",__func__);

	/* Disable amp */
	g_bAmpEnabled = true;   /* to force ImmVibeSPI_ForceOut_AmpDisable disabling the amp */
	ImmVibeSPI_ForceOut_AmpDisable();
	ReqGPTimer();
	return VIBE_S_SUCCESS;
}

/*
 * Called by the real-time loop to set PWM duty cycle, and enable amp if required
 */
static VibeStatus ImmVibeSPI_ForceOut_Set(VibeInt8 nForce)
{
//	printk("[VIBRATOR] %s \n",__func__);
	if (nForce == 0) {  
		ImmVibeSPI_ForceOut_AmpDisable();
		omap_dm_timer_stop(gptimer);
	}
	else {
		ImmVibeSPI_ForceOut_AmpEnable();
		/*
		 * Formula for matching the user space force (-127 to +127) to Duty cycle.
		 * Duty cycle will vary from 0 to 45('0' means 0% duty cycle,'45' means
		 * 100% duty cycle.Also if user space force equals to -127 then duty cycle will be 0
		 * (0%),if force equals to 0 duty cycle will be 22.5(50%),if +127 then duty cycle will
		 * be 45(100%)
		 */
		//uint32_t nTmp=(PWM_DUTY_MAX*nForce)/254 +PWM_DUTY_MAX/2;
		GPTimerSetValue(PWM_DUTY_MAX, PWM_DUTY_VALUE); /* set the duty according to the modify value later */
		omap_dm_timer_start(gptimer);  /* start the GPtimer9 */ 
	}
	return VIBE_S_SUCCESS;
}

void GPtimer_enable()
{
	omap_dm_timer_enable(gptimer);
}

static int VibeOSKernelTimerProc(struct hrtimer * local_timer)
{
	unsigned long flags;
//	printk("[VIBRATOR] %s \n",__func__);
	//mutex_lock(&(timer_state.lock));
	spin_lock_irqsave(&(timer_state.vib_lock), flags);
	ImmVibeSPI_ForceOut_Set(0);
	spin_unlock_irqrestore(&(timer_state.vib_lock), flags);
	//mutex_unlock(&(timer_state.lock));	
	return HRTIMER_NORESTART;
}

static void VibeOSKernelLinuxInitTimer(void)
{
//	printk("[VIBRATOR] %s \n",__func__);
	
	/*
	 * Initialising the High Resolution Timer
	 */
	hrtimer_init(&h_timer,CLOCK_MONOTONIC,HRTIMER_MODE_REL);	
	h_timer.function  = VibeOSKernelTimerProc;
}

static int get_time_for_vibrator(struct timed_output_dev *dev)
{

	int remaining;
//	printk("[VIBRATOR] %s \n",__func__);

	if (hrtimer_active(&h_timer)) {
		ktime_t r = hrtimer_get_remaining(&h_timer);
		remaining = r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else 
		remaining = 0;
	
	if (vibrator_value == -1)
		remaining = -1;

	return remaining;

}

static void enable_vibrator_from_user(struct timed_output_dev *dev,int value)
{

	unsigned long flags;
//	printk("[VIBRATOR] %s : time = %d msec \n",__func__,value);

	hrtimer_cancel(&h_timer);
	//mutex_lock(&(timer_state.lock));

    if(timer_state.state==TIMER_DISABLED) {
		spin_lock_irqsave(&(timer_state.vib_lock), flags);
		GPtimer_enable();
		spin_unlock_irqrestore(&(timer_state.vib_lock), flags);
		timer_state.state = TIMER_ENABLED;
	}
	spin_lock_irqsave(&(timer_state.vib_lock), flags);
	ImmVibeSPI_ForceOut_Set(value);
	spin_unlock_irqrestore(&(timer_state.vib_lock), flags);
	
	vibrator_value = value;	
	//mutex_unlock(&(timer_state.lock));
	//spin_unlock_irqrestore(&vib_lock, flags);

	if (value > 0) {
		if (value > max_timeout & value !=0xFFFF)
			value = max_timeout;

		hrtimer_start(&h_timer,
				ktime_set(value / 1000, (value % 1000) * 1000000),
				HRTIMER_MODE_REL);
		vibrator_value = 0;
	}
}


static struct timed_output_dev timed_output_vt = {
	.name     = "vibrator",
	.get_time = get_time_for_vibrator,
	.enable   = enable_vibrator_from_user,
};

static int vibrator_start(void)
{
	int ret = 0;

//	printk("[VIBRATOR] %s \n",__func__);
	
	//mutex_init(&(timer_state.lock));
	spin_lock_init(&(timer_state.vib_lock));
	timer_state.state = TIMER_DISABLED;

	if (gpio_is_valid(OMAP_GPIO_MOTOR_EN)){
		if (gpio_request(OMAP_GPIO_MOTOR_EN, "OMAP_GPIO_MOTOR_EN"))
			printk(KERN_ERR "Failed to request GPIO_VIB_EN!\n");
		mdelay(10);
		gpio_direction_output(OMAP_GPIO_MOTOR_EN,GPIO_LEVEL_LOW);
		gpio_set_value(OMAP_GPIO_MOTOR_EN, GPIO_LEVEL_LOW);
	}

	ImmVibeSPI_ForceOut_Initialize();
	VibeOSKernelLinuxInitTimer();

	/*
	 * timed_output_device settings 
	 */
	ret = timed_output_dev_register(&timed_output_vt);
	
	if(ret)
		printk(KERN_ERR "[VIBETONZ] timed_output_dev_register is fail \n");

	return ret;
}

static void vibrator_end(void)
{
//	printk("[VIBRATOR] %s \n",__func__);
}

static int __devinit vibrator_probe( struct platform_device *pdev )
{
    int ret = 0;
    int i = 0;

//printk("[VIBRATOR] %s \n",__func__);

    struct vibrator_device_info *di;
  
    di = kzalloc( sizeof(*di), GFP_KERNEL );
    if(!di)
        return -ENOMEM;
    
    platform_set_drvdata( pdev, di );


    vibrator_start();
    return 0;
}

static int __devexit vibrator_remove( struct platform_device *pdev )
{
    struct vibrator_device_info *di=platform_get_drvdata(pdev);
    vibrator_end();
//printk("[VIBRATOR] %s \n",__func__);
    platform_set_drvdata( pdev, NULL );
    kfree(di);

    return 0;
}

static int vibrator_suspend( struct platform_device *pdev ,pm_message_t state)
{
	//printk("[VIBRATOR] %s \n",__func__);
	gpio_set_value(OMAP_GPIO_MOTOR_EN, GPIO_LEVEL_LOW);
	omap_dm_timer_stop(gptimer);
    omap_dm_timer_disable(gptimer);

	//FIXME : Temporary fix to hold the PWM pin low in sleep. Hoon.
//	omap_dm_timer_enable(gptimer);
//	omap_dm_timer_stop(gptimer);
//	omap_dm_timer_disable(gptimer);
	return 0;
}

static int vibrator_resume( struct platform_device *pdev )
{
    //printk("[VIBRATOR] %s \n",__func__);
    omap_dm_timer_enable(gptimer);
    return 0;
}

struct platform_driver vibrator_platform_driver = {
    .probe      = vibrator_probe,
    .remove     = __devexit_p( vibrator_remove ),
    .suspend    = &vibrator_suspend,
    .resume     = &vibrator_resume,
    .driver     = {
        .name = DRIVER_NAME,
    },
};

static int __init vibrator_init(void)
{
	int ret = 0;
//	printk("[VIBETONZ] %s \n",__func__);
    ret = platform_driver_register( &vibrator_platform_driver );

    return ret;
}

static void __exit vibrator_exit(void)
{
//	printk("[VIBETONZ] %s \n",__func__);
    platform_driver_unregister( &vibrator_platform_driver );
}

module_init(vibrator_init);
module_exit(vibrator_exit);

MODULE_AUTHOR("SAMSUNG");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("vibrator control interface");

