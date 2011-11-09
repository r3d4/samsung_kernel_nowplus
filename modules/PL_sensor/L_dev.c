/*
 * L_dev.c 
 *
 * Description: This file implements the Light sensor module
 *
 * Author: Varun Mahajan <m.varun@samsung.com>
 */

#include <linux/kernel.h>
#include <linux/mutex.h>
//#include <asm/arch/twl4030-madc.h>

//#include <asm/arch/twl4030.h>
//#include <asm/arch/resource.h>
#include <linux/i2c/twl.h>
#include <linux/i2c/twl4030-madc.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
//#define I2C_M_WR 0
//#define PHY_TO_OFF_PM_RECIEVER(p)	(p - 0x5b)

#include <linux/delay.h>
#include <linux/earlysuspend.h>

#include "main.h"
#include "L_dev.h"
#include "common.h"

/**************************************************************/
/*Light sensor device state, LDO state*/
#define  NOT_OPERATIONAL    0
#define  OPERATIONAL        1
/**************************************************************/

// ryun MADC CHANNEL
#define TWL4030_MADC_CHANNEL_LIGHT	    2
#define DEFAULT_POLLING_INTERVAL	    100 // ryun 20100124 set 500ms for test. // Archer_LSJ DB17 : 500 => 300 => 100

/*Data Structures*/
/**************************************************************/
/*
 * lock: mutex to serialize the access to this data structure 
 * t2_vintana2_ldo: res_handle structure for VINTANA 2 LDO in TWL4030
 * device_state: operational or not operational
 * ldo_state: operational or not operational
 * ch_request: madc_chrequest structure to get the converted value for a 
 * particular channel from MADC in TWL4030
 * table: pointer to light sensor's illuminance level table
 */
typedef struct 
{
	struct mutex lock;
	struct res_handle *t2_vintana2_ldo;
	u16 device_state;
	u16 ldo_state;
	struct twl4030_madc_request ch_request; // ryun
	u32 *table;
	int prev_adc_level;
	struct input_dev *inputdevice;	// ryun input
	struct timer_list *light_polling_timer; 	// ryun for polling
	unsigned long polling_time;
	int cur_polling_state;
	int saved_polling_state;
	u16 lcd_brightness;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	u16 op_state;
}L_device_t;

/**************************************************************/

/*extern functions*/
/**************************************************************/
void L_dev_sync_mech_init(void); 
int L_dev_init(void);
int L_dev_exit(void);
int L_dev_suspend(void);
int L_dev_resume(void);
int L_dev_get_adc_val(u32 *);
int L_dev_get_illum_lvl(u16 *);
int get_average_adc_value(unsigned int * data, int count);
int L_dev_polling_start( void );
int L_dev_polling_stop( void );
int L_dev_set_timer(u16);
/**************************************************************/

/*static functions*/
/**************************************************************/
static int get_illum_lvl(u32, u16 *);

/**************************************************************/

static struct workqueue_struct *light_wq;

/*Light sensor device structure*/
/**************************************************************/
static L_device_t L_dev =
{
    .t2_vintana2_ldo = NULL,
    .device_state = NOT_OPERATIONAL,
    .ldo_state = NOT_OPERATIONAL,
    .ch_request = 
    	{
    	    .channels = TWL4030_MADC_ADCIN2,
    	    .do_avg = 0,
    	    .method = TWL4030_MADC_SW1,	// ryun
    	    .active=0,
    	    .func_cb=NULL,
    	    .rbuf[TWL4030_MADC_CHANNEL_LIGHT] = 0,
//    	    .type=TWL4030_MADC_IRQ_ONESHOT, // ryun
    	},
    	/*
    	struct twl4030_madc_request {
	u16 channels;
	u16 do_avg;
	u16 method;
	u16 type;
	int active;
	int result_pending;
	int rbuf[TWL4030_MADC_MAX_CHANNELS];
	void (*func_cb)(int len, int channels, int *buf);
	};
    	*/
/*    .ch_request = 
    	{
    	    .channel = ADCIN2,
    	    .data = 0,	
    	    .is_average = 0,
    	},*/  //ryun
    .table = L_table,
    .prev_adc_level = -1,
    .inputdevice = NULL, // ryun
    .polling_time = DEFAULT_POLLING_INTERVAL,
    .cur_polling_state = L_SYSFS_POLLING_OFF,
    .saved_polling_state = L_SYSFS_POLLING_OFF,
    .lcd_brightness = 5,	// default lvl = 5
    .op_state = 0,
};    

static int adc_table[] = { 100000, 500, 400, 100, 30, 0};

/**************************************************************/

// ryun 20091030 for calculate LCD brightness
static unsigned int sumADCval = 0;
static int lcdBrightnessCount = 0;

/*for synchronization mechanism*/
/**************************************************************/
#define LOCK()    mutex_lock(&(L_dev.lock))
#define UNLOCK()  mutex_unlock(&(L_dev.lock))

// static DECLARE_WAIT_QUEUE_HEAD(lightadc_work);

/*work struct*/
//static struct work_struct L_ws;
static void L_dev_work_func (struct work_struct *);
static DECLARE_DELAYED_WORK(L_ws, L_dev_work_func);
#ifdef CONFIG_HAS_EARLYSUSPEND
static int L_dev_early_suspend(struct early_suspend* handler);
static int L_dev_early_resume(struct early_suspend* handler);
#endif

void L_dev_sync_mech_init(void)
{
    mutex_init(&(L_dev.lock));
}
/**************************************************************/

/*extern functions*/
/**************************************************************/
int L_dev_init(void)
{
    int ret = 0;

    trace_in();

    LOCK();   

    //L_dev.t2_vintana2_ldo = resource_get("light_sensor", "t2_vintana2");  // ryun
    // add power up code.
    debug("[light] power up VINTANA2....\n");
    if (ret != twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,0x41, TWL4030_VINTANA2_DEDICATED)) // ryun. 0x46 : VINTANA2_DEDICATED
        return -5; // #define EIO  5 /* I/O error *  // ryun
    if (ret != twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,DEV_GRP_BELONG_P1, TWL4030_VINTANA2_DEV_GRP)) // ryun. 0x43 : VINTANA2_DEV_GRP 
        return -5; // #define EIO  5 /* I/O error *  // ryun
    L_dev.device_state = OPERATIONAL;
    L_dev.ldo_state = OPERATIONAL;
    /*
    if ( (ret = resource_request(L_dev.t2_vintana2_ldo, T2_VINTANA2_2V75)) < 0 )
    {
        failed(1);
    }
    else
    {
        L_dev.device_state = OPERATIONAL;
        L_dev.ldo_state = OPERATIONAL;
    }
    */ // ]]  ryun
    UNLOCK();	 

    if( !(L_dev.inputdevice = input_allocate_device()) )
    {
        ret = -ENOMEM;
        input_free_device(L_dev.inputdevice);
        failed(1);
    }
    set_bit(EV_ABS, L_dev.inputdevice->evbit); 
    set_bit(EV_SYN, L_dev.inputdevice->evbit); 
    input_set_abs_params(L_dev.inputdevice, ABS_MISC, 0, 100, 0, 0);
    L_dev.inputdevice->name = "light sensor"; 

    if((ret = input_register_device(L_dev.inputdevice))<0) 
    {
        failed(2);
    }

    // ryun 20091028 init workqueue
    INIT_DELAYED_WORK(&L_ws, (void (*) (struct work_struct *))L_dev_work_func);
    light_wq = create_singlethread_workqueue("light_wq");
    if (!light_wq)
        return -ENOMEM;

#ifdef CONFIG_HAS_EARLYSUSPEND
    L_dev.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1 ;
    L_dev.early_suspend.suspend = (void *)L_dev_early_suspend;
    L_dev.early_suspend.resume = (void *)L_dev_early_resume;
    register_early_suspend(&L_dev.early_suspend);
#endif

    trace_out();

    return ret;
}

int L_dev_exit(void)
{
    int ret = 0;

    trace_in();

    LOCK();

    // [[ ryun
    //	debug("[light] power down VINTIANA2....\n");
    	if (ret != twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,0x00, TWL4030_VINTANA2_DEDICATED)) // ryun. 0x46 : VINTIANA2_DEDICATED
    			return -5; // #define EIO  5 /* I/O error *  // ryun
    	if (ret != twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,DEV_GRP_BELONG_NONE, TWL4030_VINTANA2_DEV_GRP)) // ryun. 0x43 : VINTANA2_DEV_GRP 
    			return -5; // #define EIO  5 /* I/O error *  // ryun
    /*
    resource_release(L_dev.t2_vintana2_ldo);
    resource_put(L_dev.t2_vintana2_ldo);    
    */ // ]] ryun

    L_dev.device_state = NOT_OPERATIONAL;
    L_dev.ldo_state = NOT_OPERATIONAL;

    input_unregister_device(L_dev.inputdevice);	

    if (light_wq)
        destroy_workqueue(light_wq);

    UNLOCK();

    trace_out();

    return ret;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static int L_dev_early_suspend(struct early_suspend* handler)
{
    int ret = 0;
    trace_in() ;

    if(L_dev.saved_polling_state == L_SYSFS_POLLING_ON)
    {
        debug("[light] L_dev_suspend(void)\n");
        flush_work(&L_ws);
        cancel_delayed_work_sync(&L_ws);
        L_dev_polling_stop();
        L_dev.saved_polling_state = L_SYSFS_POLLING_ON;
        L_dev.cur_polling_state = L_SYSFS_POLLING_OFF;
    }

    printk( "%s: Early suspend sleep success!!!\n", __FUNCTION__ ) ;
    trace_out() ; 
    return ret;
}

static int L_dev_early_resume(struct early_suspend* handler)
{
    int ret = 0;
    trace_in();

    if(L_dev.cur_polling_state == L_SYSFS_POLLING_OFF
    && L_dev.saved_polling_state == L_SYSFS_POLLING_ON)
    {
        debug("[light] L_dev_resume() : L_dev_polling_start() L_dev.polling_time = %d ms \n", L_dev.polling_time);
        L_dev.saved_polling_state = L_SYSFS_POLLING_OFF;
        L_dev_polling_start();
        L_dev.cur_polling_state = L_SYSFS_POLLING_ON;
    }

    printk( "%s: Early suspend wakeup success!!!\n", __FUNCTION__ ) ;
    trace_out();
    return ret;
}
#endif

int L_dev_suspend(void)
{
    int ret = 0;

    trace_in();

    LOCK();   

    if( L_dev.device_state == NOT_OPERATIONAL )
    {
        failed(1);
        ret = -1;
    }
    else if( L_dev.ldo_state == OPERATIONAL )
    {
#ifndef CONFIG_HAS_EARLYSUSPEND
        if(L_dev.saved_polling_state == L_SYSFS_POLLING_ON)
        {
            debug("[light] L_dev_suspend(void)\n");
            flush_work(&L_ws);
            cancel_delayed_work_sync(&L_ws);
            L_dev_polling_stop();	
            L_dev.saved_polling_state = L_SYSFS_POLLING_ON;
            L_dev.cur_polling_state = L_SYSFS_POLLING_OFF;
        }
#endif
#if 0
        // [[ ryun
        //resource_release(L_dev.t2_vintana2_ldo);
        debug("[light] power down VINTIANA2....\n");
        if (ret != twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,0x00, TWL4030_VINTANA2_DEDICATED)) // ryun. 0x46 : VINTIANA2_DEDICATED
            return -5; // #define EIO  5 /* I/O error *  // ryun
        if (ret != twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,DEV_GRP_BELONG_NONE, TWL4030_VINTANA2_DEV_GRP)) // ryun. 0x43 : VINTANA2_DEV_GRP 
            return -5; // #define EIO  5 /* I/O error *  // ryun
        // ]] ryun
#endif
        L_dev.ldo_state = NOT_OPERATIONAL;
    }

    UNLOCK(); 

    trace_out();

    return ret;
}

int L_dev_resume(void)
{
    int ret = 0;

    trace_in();
    LOCK();   

    if( L_dev.device_state == NOT_OPERATIONAL )
    {
        failed(1);
        ret = -1;
    }
    else if( L_dev.ldo_state == NOT_OPERATIONAL )
    {
        // add power up code.
        debug("[light] power up VINTIANA2....\n");
        if (ret != twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER,0x41, TWL4030_VINTANA2_DEDICATED)) // ryun. 0x46 : VINTIANA2_DEDICATED
            return -5; // #define EIO  5 /* I/O error *  // ryun
        if (ret != twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, DEV_GRP_BELONG_P1, TWL4030_VINTANA2_DEV_GRP)) // ryun. 0x43 : VINTANA2_DEV_GRP 
            return -5; // #define EIO  5 /* I/O error *  // ryun
        L_dev.ldo_state = OPERATIONAL;

        /*
        if ( (ret = resource_request(L_dev.t2_vintana2_ldo, T2_VINTANA2_2V75)) < 0 )
        {
            failed(2);
            ret = -1;
        }
        else
        {
            L_dev.ldo_state = OPERATIONAL;
        }
        */
#ifndef CONFIG_HAS_EARLYSUSPEND
        debug("[light] resume!! L_dev.cur_polling_state=%d, L_dev.saved_polling_state=%d \n", L_dev.cur_polling_state, L_dev.saved_polling_state);
        if(L_dev.cur_polling_state == L_SYSFS_POLLING_OFF
        && L_dev.saved_polling_state == L_SYSFS_POLLING_ON)
        {
            debug("[light] L_dev_resume() : L_dev_polling_start() L_dev.polling_time = %d ms \n", L_dev.polling_time);
            L_dev.saved_polling_state = L_SYSFS_POLLING_OFF;
            L_dev_polling_start();
            L_dev.cur_polling_state = L_SYSFS_POLLING_ON;
        }
#endif
    }
    UNLOCK(); 
    trace_out();
    return ret;
}

int L_dev_get_adc_val(u32 *adc_val)
{
    int ret = 0;
	// int i, temp, data[6];
    // trace_in();
 
    LOCK();   
 
    if( L_dev.device_state == NOT_OPERATIONAL )
    {
        failed(1);
        ret = -1;
    }
    else if ( L_dev.ldo_state == NOT_OPERATIONAL )
    {
        failed(2);
        ret = -1;
    }
#if 1
    //else if( (ret = twl4030madc_convert(&(L_dev.ch_request), 1)) < 0 )
    else if((ret = twl4030_madc_conversion(&(L_dev.ch_request))) < 0 ) // ryun
    {
        failed(3);
    }    
    else
    {
        // *adc_val = L_dev.ch_request.data;
        // debug("   ADC data: %u", L_dev.ch_request.data);
        *adc_val = L_dev.ch_request.rbuf[TWL4030_MADC_CHANNEL_LIGHT];
        debug("ADC data: %u", L_dev.ch_request.rbuf[TWL4030_MADC_CHANNEL_LIGHT]);
    }
#else
    else
    {
	temp = 0;

	for(i = 0; i < 6; i++) {
		if( (ret = twl4030madc_convert(&(L_dev.ch_request), 1)) < 0 )
		    {
		        failed(3);
		    }   
		data[i] = L_dev.ch_request.data;
		sleep_on_timeout(&lightadc_work, 2);
	}
	
        *adc_val = get_average_adc_value(data, 6);
        debug("   ADC data: %u", *adc_val);
    }
#endif
 
    UNLOCK(); 
 
    // trace_out();
     return ret;
}

int L_dev_get_illum_lvl(u16 *illum_lvl)
{
    int ret = 0;
    u32 mV;
	//int i, temp, data[6];
   
    trace_in();
 
    LOCK();   
 
    if( L_dev.device_state == NOT_OPERATIONAL )
    {
        failed(1);
        ret = -1;
    }
    else if ( L_dev.ldo_state == NOT_OPERATIONAL )
    {
        failed(2);
        ret = -1;
    }
#if 1
    //else if( (ret = twl4030madc_convert(&(L_dev.ch_request), 1)) < 0 )
    else if((ret = twl4030_madc_conversion(&(L_dev.ch_request))) < 0 ) // ryun
    {
        failed(3);
    } 
    else
    {
         mV = L_dev.ch_request.rbuf[TWL4030_MADC_CHANNEL_LIGHT];
        debug("   ADC data: %u", L_dev.ch_request.rbuf[TWL4030_MADC_CHANNEL_LIGHT]);
        if( get_illum_lvl(mV, illum_lvl) < 0 )
        {
            failed(4);
            ret = -1;
        }
    }
#else
    else
    {

	temp = 0;

	for(i = 0; i < 6; i++) {
		if( (ret = twl4030madc_convert(&(L_dev.ch_request), 1)) < 0 )
		    {
		        failed(3);
		    }   
		data[i] = L_dev.ch_request.data;
		sleep_on_timeout(&lightadc_work, 2);
	}
	
        //mV = L_dev.ch_request.data;
        mV = get_average_adc_value(data, 6);
		
        debug("   ADC data: %u", mV);
        if( get_illum_lvl(mV, illum_lvl) < 0 )
        {
            failed(4);
            ret = -1;
        }
    }
#endif

    UNLOCK(); 
    trace_out();
    return ret;
}	

static int get_illum_lvl(u32 mV, u16 *illum_lvl)
{
    int ret = -1;
    int i = L_LVL1_mV_IDX, j;

    if( L_dev.table[L_NO_OF_LVLS_IDX] > L_MAX_LVLS )
    {
        failed(1);
        return -1;
    }

    for( i = L_LVL1_mV_IDX, j = 1; j <= L_dev.table[L_NO_OF_LVLS_IDX]; 
         i += L_LVL_mV_INCR, j++ )
    {
        if( (mV >= L_dev.table[i]) && (mV <= L_dev.table[i+1]) )
        {
            *illum_lvl = L_dev.table[i-3];
            ret = 0;
            break;
        }
    }
    return ret;
}

int get_average_adc_value(unsigned int * data, int count)
{

	int i=0, average, min=0xFFFFFFFF, max=0, total=0;
	for(i=0 ; i<count ; i++)
	{
		if(data[i] < min)
			min=data[i];
		if(data[i] > max)
			max=data[i];

		total+=data[i];
	}
	average = (total - min -max)/(count -2);
	return average;
}

int L_dev_get_op_state(void)
{
	return (int)(L_dev.op_state);
}

void L_dev_set_op_state(u16 op_state)
{
	switch(op_state)
	{
		case 0: // OFF
		if(L_dev.op_state == 1) // if ON state
		{
			L_dev.op_state = 0;
		}
		break;

		case 1: // ON
		L_dev.op_state = 1;
		break;

		case 2: // ON LOCK for factory test
		L_dev.op_state = 2;
		break;
	}
}

// [[ ryun 20091028 for timer
int L_dev_polling_start(void)
{	
	printk("[light] L_dev_polling_start() L_dev.polling_time = %d ms\n", L_dev.polling_time);
	if(L_dev.saved_polling_state == L_SYSFS_POLLING_ON)
	{
		debug("[light] L_dev_polling_start() ... already POLLING ON!! \n");
		return L_dev.saved_polling_state;
	}
    
//	schedule_delayed_work(&L_ws, L_dev.polling_time);
	cancel_delayed_work_sync(&L_ws);
	queue_delayed_work(light_wq, &L_ws, L_dev.polling_time);

 	 L_dev.saved_polling_state = L_SYSFS_POLLING_ON;
	 return L_dev.saved_polling_state;
}
int L_dev_polling_stop(void)
{
	printk("[light] L_dev_polling_stop() \n");
	
	if(L_dev.saved_polling_state == L_SYSFS_POLLING_OFF)
	{
		debug("[light] L_dev_polling_stop() ... already POLLING OFF!! \n");
		return L_dev.saved_polling_state;
	}

	flush_work(&L_ws);
	cancel_delayed_work_sync(&L_ws);

	L_dev.saved_polling_state = L_SYSFS_POLLING_OFF;
	return L_dev.saved_polling_state;
}

#ifdef CONFIG_MACH_OSCAR	// ryun 20091212 for OSCAR
int L_dev_get_polling_state(void)
{
	    if( L_dev.device_state == NOT_OPERATIONAL )
	    {
	        return L_SYSFS_POLLING_OFF;
	    }
	    else if ( L_dev.ldo_state == NOT_OPERATIONAL )
	    {
	        return L_SYSFS_POLLING_OFF;
	    }
	return L_dev.saved_polling_state;
}
unsigned long L_dev_get_polling_interval( void )
{
	return L_dev.polling_time;
}
void L_dev_set_polling_interval(unsigned long interval)
{
	L_dev.polling_time = interval;
}
#endif	// ryun 20091212 for OSCAR


static void L_dev_work_func (struct work_struct *unused)
{
	u32 adc_val;

	trace_in() ;

	debug("[light] L_dev_work_func(), L_dev.saved_polling_state= %d\n", L_dev.saved_polling_state);
    if( !(L_dev.saved_polling_state) )
//	if(L_dev.op_state)
	{
		int adc_level = 0;
		if( L_dev_get_adc_val(&adc_val) < 0 )
		{
			failed(1);
			debug( "[light] Failed!!!\n" );
		}
		for(; adc_level < sizeof(adc_table)/sizeof(int); adc_level++)
		{
			if(adc_val > adc_table[adc_level])
				break;
		}

		if(L_dev.prev_adc_level != adc_level)
		{
			input_report_abs(L_dev.inputdevice, ABS_MISC, adc_val);
			input_sync(L_dev.inputdevice);
			L_dev.prev_adc_level = adc_level;
			debug("[light] L_dev_work_func() light output...adc_val = %d ", adc_val);
		}
	}

	// debug("[light] schedule_delayed_work(&L_ws, DEFAULT_POLLING_INTERVAL=%d ms);", DEFAULT_POLLING_INTERVAL);
	// cancel_delayed_work_sync(&L_ws);
	// schedule_delayed_work(&L_ws, L_dev.polling_time);	// ryun 20091030 

	queue_delayed_work(light_wq, &L_ws, L_dev.polling_time);

	trace_out() ;
}
