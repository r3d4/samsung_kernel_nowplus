/*
 * main.c 
 *
 * Description: This file implements the Main module of the GP2AP002A00F driver 
 *
 * Author: Varun Mahajan <m.varun@samsung.com>
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/irq.h>
#include <linux/slab.h>

//#include <asm/arch/gpio.h>
//#include <asm/arch/nowplus.h>
#include <mach/gpio.h>
#include <plat/io.h>
//#include <mach/nowplus.h>
#include <mach/hardware.h>
#include <plat/mux.h>
#include <linux/i2c/twl.h>

#define I2C_M_WR 0
#define PHY_TO_OFF_PM_RECIEVER(p)	(p - 0x5b)

#include <asm/irq.h>
#include <linux/workqueue.h>
#include <linux/stat.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <asm/atomic.h>

#include "ioctls.h"
#include "i2c_drv.h"
#include "P_dev.h"
#include "L_dev.h"
#include "common.h"
#include "main.h"
#include "sysfs.h"

#ifdef CONFIG_MACH_OSCAR
#include <linux/sensors_core.h>
#include <linux/poll.h>
#include <linux/input.h>
#endif

#include <linux/wakelock.h>

/*****************************************************************************/
/****************           PROXIMITY SENSOR SPECIFIC          ***************/
/*****************************************************************************/
/*flag states*/
#define SET   1
#define RESET 0

/*Wait queue structure*/
typedef struct
{
    struct list_head list;
    struct mutex list_lock;
    wait_queue_head_t waitq;
}P_queue_t;

/*for user count*/
typedef struct
{
    struct mutex lock;
    int count;
}user_count_t;

/*file structure private data*/
typedef struct
{
    struct list_head node;
    atomic_t int_flag;
    wait_queue_t waitq_entry;
}P_fpriv_data_t;
/**********************************************************/

/**********************************************************/
/*P IRQ*/
#define P_IRQ OMAP_GPIO_IRQ(OMAP_GPIO_PS_OUT)         

/*extern functions*/
/**********************************************************/
int P_waitq_wkup_proc(void);

void P_enable_int(void);
void P_disable_int(void);

int pl_power_init( void );
void pl_power_exit( void );

/**********************************************************/

/*static functions*/
/**********************************************************/
/*file operatons*/
static int P_open (struct inode *, struct file *);
static int P_release (struct inode *, struct file *);
static int P_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
static ssize_t P_read(struct file *filp, char *buf, size_t count, loff_t *ofs);
static unsigned int P_poll(struct file *filp, struct poll_table_struct *pwait);

/*ISR*/
static irqreturn_t P_isr( int irq, void *unused );

/*waitq related functions*/
static void P_waitq_init(void);
static void P_waitq_list_insert_proc(P_fpriv_data_t *);
static void P_waitq_prepare_to_wait(P_fpriv_data_t *);
static void P_waitq_list_remove_proc(P_fpriv_data_t *);
static void P_waitq_finish_wait(P_fpriv_data_t *);
/**********************************************************/
/*module param*/
static u16 an_sleep_func = P_AN_SLEEP_OFF;
static u16 detec_cyc = P_CYC_8ms;

/*work struct*/
static struct work_struct P_ws;

static struct file_operations P_fops =
{
    .owner = THIS_MODULE,
    .open = P_open,
    .ioctl = P_ioctl,
    .release = P_release,
    .read = P_read,
    .poll = P_poll,
};

static struct miscdevice P_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "proximity_sensor",
    .fops = &P_fops,
};

#ifdef CONFIG_MACH_OSCAR
static struct sensors_dev P_sensor_dev = {
    .minor = 0,
    .dev = NULL,
    .fops = &P_fops,
};
#endif

static user_count_t P_user;

/*Wait queue*/
static P_queue_t P_waitq;

static struct wake_lock P_sensor_wake_lock;

/*****************************************************************************/
/*****************************************************************************/

/*****************************************************************************/
/******************           LIGHT SENSOR SPECIFIC          *****************/
/*****************************************************************************/
/*static functions*/
/**********************************************************/
/*file operatons*/
static int L_open (struct inode *, struct file *);
static int L_release (struct inode *, struct file *);
static int L_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
static ssize_t L_read(struct file *filp, char *buf, size_t count, loff_t *ofs);
static unsigned int L_poll(struct file *filp, struct poll_table_struct *pwait);

static struct file_operations L_fops =
{
    .owner = THIS_MODULE,
    .open = L_open,
    .ioctl = L_ioctl,
    .release = L_release,
    .read = L_read,
    .poll = L_poll,
};

static struct miscdevice L_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "light_sensor",
    .fops = &L_fops,
};

#ifdef CONFIG_MACH_OSCAR
static struct sensors_dev L_sensor_dev = {
    .minor = 0,
    .fops = &L_fops,
};
#endif

static u32 g_illum_lvl;

/*module param: light sensor's illuminance level table*/
u32 L_table [L_MAX_LVLS*5 + 1] = 
{
    /*total no of levels*/
    9,

    /*level no*/  /*lux (min)*/  /*lux(max)*/  /*mV(min)*/  /*mV(max)*/
   
    1,               1,           164,            0,          100,
    2,             165,           287,          101,         200,
    3,             288,           496,         201,         300,
    4,             497,           868,         301,         400,
    5,             869,          1531,         401,         500,
    6,            1532,          2691,         501,         600,
    7,            2692,          4691,         601,         700,
    8,            4692,          8279,         701,         800,
    9,            8280,        100000,         801,         2500,    
 /*	
    1,               1,           164,            0,          960,
    2,             165,           287,          961,         1035,
    3,             288,           496,         1036,         1150,
    4,             497,           868,         1151,         1320,
    5,             869,          1531,         1321,         1430,
    6,            1532,          2691,         1431,         1500,
    7,            2692,          4691,         1501,         1615,
    8,            4692,          8279,         1616,         1750,
    9,            8280,        100000,         1751,         2500,    
*/    
};
/*****************************************************************************/
/*****************************************************************************/


/*****************************************************************************/
/****************           PROXIMITY SENSOR SPECIFIC          ***************/
/*****************************************************************************/
static int P_open (struct inode *inode, struct file *filp)
{
    int ret = 0;
    P_fpriv_data_t *fpriv;
    unsigned short power_state = 0, op_mode = 0;
      
    trace_in();

    if( (fpriv = (P_fpriv_data_t *)kzalloc(sizeof(P_fpriv_data_t), GFP_KERNEL)) == NULL )
    {
        failed(1);
        ret = -ENOMEM;
    }    
    else
    {
        #ifdef CONFIG_MACH_OSCAR
        ret = P_dev_get_pwrstate_mode( &power_state, &op_mode );
        if(ret < 0)
        {
            printk("PSENSOR: %s: get power state failed! [%d]\n", __func__, ret);
            return ret;
        }
        
        if(power_state != P_OPERATIONAL)
        {
            ret = P_dev_powerup_set_op_mode(P_MODE_B);
            if(ret < 0)
            {
                printk("PSENSOR: %s: Can't turn on! [%d]\n", __func__, ret);
                return ret;
            }
        }
        #endif
        
        mutex_lock( &(P_user.lock) );
        #ifndef CONFIG_MACH_OSCAR
        if( P_user.count == 1 )
        {
            mutex_unlock( &(P_user.lock) ); 
            kfree(fpriv);
            ret = -EBUSY;
        }
        else
        {	
        #endif
            P_user.count++;
            mutex_unlock( &(P_user.lock) ); 

            atomic_set( &(fpriv->int_flag), RESET );
            filp->private_data = fpriv;
            
            #ifdef CONFIG_MACH_OSCAR
            P_waitq_list_insert_proc( fpriv );
            #endif
            nonseekable_open(inode, filp);
        }
    #ifndef CONFIG_MACH_OSCAR
    }
    #endif

    trace_out();
    return ret;
}

static int P_release (struct inode *inode, struct file *filp)
{
    int ret = 0;
    P_fpriv_data_t *fpriv = (P_fpriv_data_t *)filp->private_data;
     
    trace_in();
    
    #ifdef CONFIG_MACH_OSCAR
    P_waitq_list_remove_proc( fpriv );
    #endif

    kfree(fpriv);

    mutex_lock( &(P_user.lock) );
    P_user.count--;
    mutex_unlock( &(P_user.lock) );      

    #ifdef CONFIG_MACH_OSCAR
    if(P_user.count < 1)
    {
        ret = P_dev_shutdown();
        if(ret < 0)
        {
            printk(KERN_ERR "PSENSOR: %s: power down failed![%d]\n", __func__, ret);
            return ret;
        }
    }
    #endif
    trace_out();
     
    return ret;
}

static ssize_t P_read(struct file *filp, char *buf, size_t count, loff_t *ofs)
{
    int ret = 0;
#ifdef CONFIG_MACH_OSCAR
    unsigned short power_state = 0,  opmode = 0, detected = 0;
    struct input_event event;
    P_fpriv_data_t *fpriv = (P_fpriv_data_t *)filp->private_data;

    trace_in();

    ret = P_dev_get_pwrstate_mode( &power_state, &opmode );
    if( ret < 0 )
    {
        printk("PSENSOR: %s: get power state failed![%d]\n", __func__, ret);
        goto error_return;
    }
    
    if( power_state == P_OPERATIONAL )
    {
        ret = P_dev_get_prox_output( &detected );
        
    	if( ret < 0 )
	    {
	        printk("PSENSOR: %s: get prox output failed![%d]\n", __func__, ret);
    	    goto error_return;
	    }
        
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV == CONFIG_OSCAR_REV00_EMUL) 
	detected = 1;
#else
        detected = detected ? 0 : 1;
#endif
        printk("PSENSOR: %s: proximity[%d]\n", __func__, detected);

        do_gettimeofday(&(event.time));
        event.type = EV_ABS;
        event.code = ABS_DISTANCE;
        event.value = detected;

        if( copy_to_user(buf, (const void*)&event, sizeof(struct input_event)) )
        {
            printk("PSENSOR: %s: copy_to_user failed\n", __func__);
            goto error_return;
        }
        ret = sizeof(struct input_event);
        *ofs += ret;
        atomic_set( &(fpriv->int_flag), RESET );
    }

error_return:	  
    trace_out();
#endif
    return ret;
}

static unsigned int P_poll(struct file *filp, struct poll_table_struct *pwait)
{
#ifdef CONFIG_MACH_OSCAR
    P_fpriv_data_t *fpriv = (P_fpriv_data_t *)filp->private_data;
    int int_flag;
    
    trace_in();

    mutex_lock( &(P_user.lock) );
    int_flag = atomic_read( &(fpriv->int_flag) );
    mutex_unlock( &(P_user.lock) );

    trace_out();
    return int_flag == SET ? POLLIN | POLLRDNORM : 0;
#else
    trace_in();
    trace_out();
    return 0;
#endif
}

static int P_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_cmd, unsigned long arg)
{
    int ret = 0;
    void __user *argp = (void __user *)arg;   

    trace_in(); 
    //	ioctl_cmd = P_IOC_GET_PROX_OP; // ryun for test
    
	debug("\n[ryun] P_IOC_GET_PROX_OP=0x%x , ioctl_cmd=0x%x \n", P_IOC_GET_PROX_OP, ioctl_cmd);
	   
    if( _IOC_TYPE(ioctl_cmd) != P_IOC_MAGIC )
    {
        debug("%s Inappropriate ioctl 1 0x%x", __FUNCTION__, ioctl_cmd);
        return -ENOTTY;
    }
    
    if( (_IOC_NR(ioctl_cmd) > P_IOC_NR_MAX) || (_IOC_NR(ioctl_cmd) < P_IOC_NR_MIN) )
    {
        debug("%s Inappropriate ioctl 2 0x%x", __FUNCTION__, ioctl_cmd);
        return -ENOTTY;
    }
    
    switch (ioctl_cmd)
    {
        case P_IOC_POWERUP_SET_MODE:
            {
                u16 mode;

                if( copy_from_user((void*) &mode, argp, sizeof(u16)) )
                {
                    debug("\n[ryun] mode =  %d , argp=%d, P_MODE_B=%d \n", mode, (int)argp, P_MODE_B);
                    ret = -EFAULT;
                    failed(1);
                }
                else if( (ret = P_dev_powerup_set_op_mode(mode)) < 0 )
                {
                    failed(2);
                }
                printk("[PROXIMITY] P_ioctl() case P_IOC_POWERUP_SET_MODE mode=%d \n", mode);
            }   
            break;

        case P_IOC_GET_PROX_OP:
            {
                u16 prox_op;

                if( (ret = P_dev_get_prox_output(&prox_op)) < 0 )
                {
                	debug("\n[ryun] P_ioctl() : failed(5) ret=%d \n", ret);
                    failed(5);
                }
                else if( copy_to_user(argp, (void*) &prox_op, sizeof(u16)) )
                {
                    ret = -EFAULT;
                	debug("\n[ryun] P_ioctl() : failed(5) ret=%d \n", ret);
                    failed(6);
                }                
                debug("\n[ryun] P_ioctl() : ret=%d \n", ret);
            }   
            break;

        case P_IOC_WAIT_ST_CHG:
            {
                u16 mode;
                P_fpriv_data_t *fpriv = (P_fpriv_data_t *) filp->private_data;

                if( (ret = P_dev_get_mode(&mode)) < 0 )
                {
                    failed(7);
                    break;
                }

                /*mode B is the interrupt mode*/
                debug("\n[ryun] [P_IOC_WAIT_ST_CHG] mode=%d, P_MODE_B=%d \n", mode, P_MODE_B);
           	    if( mode != P_MODE_B )
           	    {
           	        failed(8);
           	        ret = -EPERM;
           	        break;
           	    }
                debug("\n[ryun] [P_IOC_WAIT_ST_CHG] init_wait( &(fpriv->waitq_entry) ); \n");
                init_wait( &(fpriv->waitq_entry) );    
                P_waitq_list_insert_proc(fpriv);
                P_waitq_prepare_to_wait(fpriv);

                /*mandatory condition check*/
                if( atomic_read( &(fpriv->int_flag) ) == RESET )
                {
                    debug("\n[ryun] [P_IOC_WAIT_ST_CHG] schedule(); +  \n");
                    schedule();
                    debug("\n[ryun] [P_IOC_WAIT_ST_CHG] schedule(); +  \n");
                }
                
                P_waitq_list_remove_proc(fpriv);
                P_waitq_finish_wait(fpriv);

                atomic_set(&(fpriv->int_flag), RESET);
            }
	          break;            

        case P_IOC_RESET:
            {
                if( (ret = P_dev_reset()) < 0 )
                {
                    failed(9);
                }
            }   
            break;

        case P_IOC_SHUTDOWN:
            {
                if( (ret = P_dev_shutdown()) < 0 )
                {
                    failed(10);
                }
                printk("[PROXIMITY] P_ioctl() case P_IOC_SHUTDOWN  \n" );
            }   
            break;

        case P_IOC_RETURN_TO_OP:
            {
                if( (ret = P_dev_return_to_op()) < 0 )
                {
                    failed(11);
                }
            }   
            break;            

        default:
            debug("%s Inappropriate ioctl default 0x%x", __FUNCTION__, 
            	      ioctl_cmd);
            ret = -ENOTTY;
            break;
    }

    trace_out();   
    return ret;
}

static irqreturn_t P_isr( int irq, void *unused )
{
	int ret = 0; // ryun

	wake_lock_timeout(&P_sensor_wake_lock, 3*HZ);

	debug("[ryun] Proximity interrupt!! \n");
    trace_in(); 

    // schedule_work(&P_ws);
	ret = schedule_work(&P_ws);
	debug("[ryun] schedule_work(&P_ws) ret(1:succes, 0:fail) = %d \n", ret );

    trace_out();
    return IRQ_HANDLED;
}

/**********************************************************/
static void P_waitq_init(void)
{
    trace_in();
    
    mutex_init( &(P_waitq.list_lock) );
    init_waitqueue_head( &(P_waitq.waitq) );
    INIT_LIST_HEAD( &(P_waitq.list) );

    trace_out();
}

static void P_waitq_list_insert_proc( P_fpriv_data_t *fpriv )
{
    trace_in();
    mutex_lock( &(P_waitq.list_lock) );
    list_add_tail(&(fpriv->node), &(P_waitq.list));
    mutex_unlock( &(P_waitq.list_lock) );
    trace_out();
}

static void P_waitq_prepare_to_wait( P_fpriv_data_t *fpriv )
{
    trace_in();
    prepare_to_wait(&(P_waitq.waitq), &(fpriv->waitq_entry), 
    	                 TASK_INTERRUPTIBLE);
	trace_out();
}

static void P_waitq_list_remove_proc( P_fpriv_data_t *fpriv )
{
    trace_in();
    mutex_lock( &(P_waitq.list_lock) );
    list_del_init( &(fpriv->node) );
    mutex_unlock( &(P_waitq.list_lock) );
	trace_out();
}

static void P_waitq_finish_wait( P_fpriv_data_t *fpriv )
{
    trace_in();
    finish_wait(&(P_waitq.waitq), &(fpriv->waitq_entry));
	trace_out();
}

int P_waitq_wkup_proc(void)
{
    int no_of_processes = 0;
    struct list_head *node;
    P_fpriv_data_t *fpriv;

    trace_in();

    mutex_lock( &(P_waitq.list_lock) );

    list_for_each( node, &(P_waitq.list) )
    {
        fpriv = list_entry(node, P_fpriv_data_t, node);
        atomic_set( &(fpriv->int_flag), SET );

        no_of_processes++;
    }

    mutex_unlock( &(P_waitq.list_lock) );

    if( no_of_processes == 0 )
    {
        debug("   no process is waiting");
    }
    else
    {
        wake_up_interruptible( &(P_waitq.waitq) );
        debug("   no of processes woken up: %d", no_of_processes);
    }
    
    trace_out();

    return no_of_processes;
}

void P_enable_int(void)
{
    debug("   INTERRUPT ENABLED");
    enable_irq(P_IRQ);
}

void P_disable_int(void)
{
    debug("   INTERRUPT DISABLED");
    disable_irq(P_IRQ);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/******************           LIGHT SENSOR SPECIFIC                         *****************/
/*****************************************************************************/
static int L_open (struct inode *inode, struct file *filp)
{
    trace_in();
    return nonseekable_open(inode, filp);
}

static int L_release (struct inode *inode, struct file *filp)
{
    trace_in();
    return 0;
}

static ssize_t L_read(struct file *filp, char *buf, size_t count, loff_t *ofs)
{
    int ret = 0;
    
    #ifdef CONFIG_MACH_OSCAR
    struct input_event event;

    trace_in();
    do_gettimeofday(&(event.time));
    event.type = EV_ABS;
    event.code = ABS_MISC;
    event.value = g_illum_lvl;
    printk("LSENSOR : READ(%d)\n", g_illum_lvl);
    if( copy_to_user(buf, (const void*)&event, sizeof(struct input_event)) < 0)
    {
        printk(KERN_ERR "LSENSOR: %s copy_to_user failed!\n", __func__);
        return 0;
    }

    ret = sizeof(struct input_event);
    *ofs += ret;
   
    trace_out();
    #endif
    
    return ret;
}

static unsigned int L_poll(struct file *filp, struct poll_table_struct *pwait)
{
    #ifdef CONFIG_MACH_OSCAR
    int ret;
    u32 illum_lvl;

    trace_in();

    ret  = L_dev_get_adc_val(&illum_lvl);
    if(ret < 0)
    {
        printk(KERN_ERR "LSENSOR: %s: L_dev_get_illum_lvl failed![%d]\n", __func__, ret);
        return 0;
    }
    
    if( g_illum_lvl != illum_lvl )
    {
        g_illum_lvl = illum_lvl;
        return POLLIN | POLLRDNORM;
    }
    #endif    
    
    return 0;
}

static int L_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_cmd, unsigned long arg)
{
    int ret = 0;
    void __user *argp = (void __user *)arg;   

    trace_in(); 

	// ryun 20091031 for test 
	debug("\n[ryun] L_IOC_MAGIC=0x%x\n", L_IOC_MAGIC);
	debug("\n[ryun] L_IOC_NR_MAX=0x%x, L_IOC_NR_MIN=0x%x\n", L_IOC_NR_MAX, L_IOC_NR_MIN);
		   
    if( _IOC_TYPE(ioctl_cmd) != L_IOC_MAGIC )
    {
        debug("%s Inappropriate ioctl 1 0x%x\n", __FUNCTION__, ioctl_cmd);
        return -ENOTTY;
    }
    if( (_IOC_NR(ioctl_cmd) > L_IOC_NR_MAX) || 
    	   (_IOC_NR(ioctl_cmd) < L_IOC_NR_MIN) )
    {
        debug("%s Inappropriate ioctl 2 0x%x\n", __FUNCTION__, ioctl_cmd);
        return -ENOTTY;
    }
	
    switch (ioctl_cmd)
    {
        case L_IOC_GET_ADC_VAL:
            {
                u32 adc_val;

                if( (ret = L_dev_get_adc_val(&adc_val)) < 0 )
                {
                    failed(1);
                }
                else if( copy_to_user(argp, (void*) &adc_val, 
                	        sizeof(unsigned int)) )
                {
                    ret = -EFAULT;
                    failed(2);
                }                
            }   
            break;

        case L_IOC_GET_ILLUM_LVL:
            {
                u16 illum_lvl;

                if( (ret = L_dev_get_illum_lvl(&illum_lvl)) < 0 )
                {
                    failed(3);
                }
                else if( copy_to_user(argp, (void*) &illum_lvl, 
                	        sizeof(unsigned short)) )
                {
                    ret = -EFAULT;
                    failed(4);
                }                
            }   
            break;            
            
	    // [[ ryun 20091028 for light sensor 
    	case L_IOC_POLLING_TIMER_SET:	// ryun 20091028 set timer and start polling
    		{
    			if( L_dev_polling_start() == 0 ) // ryun 20091212 
    				printk("[LIGHT] L_ioctl() case L_IOC_POLLING_TIMER_SET \n");			
    		}
    		break;
            
    	case L_IOC_POLLING_TIMER_CANCEL:	// ryun 20091028 kill timer and stop polling 
    		{
    			if(L_dev_polling_stop() == 1 )
    				printk("[LIGHT] L_ioctl() case L_IOC_POLLING_TIMER_CANCEL \n");
    		}
    		break;
        
        default:
            debug("%s Inappropriate ioctl default 0x%x", __FUNCTION__, 
            	        ioctl_cmd);
            ret = -ENOTTY;
            break;
    }

    trace_out();   
    return ret;
}
/*****************************************************************************/
/*****************************************************************************/

int __init PL_driver_init(void)
{
    int ret = 0;

    trace_in(); 

    /*Proximity sensor: initialize user count*/
    mutex_init(&(P_user.lock));
    P_user.count = 0;

    /*Proximity sensor: Initilize the P dev synchronization mechanism*/
    P_dev_sync_mech_init();

    debug("module param an_sleep_func = %d, detec_cyc = %d", an_sleep_func,
    	     detec_cyc);

    /*Proximity sensor: initialize analog sleep function state and detection cycle*/
    P_dev_an_func_state_init(an_sleep_func);
    P_dev_detec_cyc_init(detec_cyc);

    /*Light sensor: Initilize the L dev synchronization mechanism*/
    L_dev_sync_mech_init();    

    /*Proximity sensor: Initialize the waitq*/
    P_waitq_init();

    if( (ret = pl_power_init()) < 0 )
    {
        error(" pl_power_init failed");
        failed(0);
        return ret; 	  	
    }

    /*Proximity sensor: misc device registration*/
    if( (ret = misc_register(&P_misc_device)) < 0 )
    {
        error(" misc_register failed");
        failed(1);
        return ret; 	  	
    }

    /*Light sensor: misc device registration*/
    if( (ret = misc_register(&L_misc_device)) < 0 )
    {
        error(" misc_register failed");
        failed(2);
        goto MISC_DREG1; 	  	
    }    

    /*Proximity sensor: ISR's bottom half*/
    INIT_WORK(&P_ws, (void (*) (struct work_struct *))P_dev_work_func);


    set_irq_type ( P_IRQ, IRQF_TRIGGER_FALLING);

    if( (ret = request_irq(P_IRQ,P_isr,0 ,DEVICE_NAME, (void *)NULL)) < 0 )
    {
        error("request_irq failed %d",P_IRQ);
        failed(3);
        goto MISC_DREG2;
    }
    /**********************************************************/
    enable_irq_wake(P_IRQ); // keep wakeup attr in sleep state

    /*Add the i2c driver*/
    if ( (ret = PL_i2c_drv_init() < 0) ) 
    {
         failed(4);
         goto MISC_IRQ_DREG;
    }

    /*Proximity sensor: sysfs interface*/
    #ifndef CONFIG_MACH_OSCAR
    if( (ret = P_sysfs_init(P_misc_device.this_device)) < 0 )
    #else
    if( (ret = P_sysfs_init(&P_sensor_dev)) < 0 )
    #endif
    {
        error(" sysfs error[%d]", ret);
        failed(5);
        goto MISC_IRQ_I2CDRV_DREG;
    }

    /*Light sensor: sysfs interface*/
    #ifndef CONFIG_MACH_OSCAR
    if( (ret = L_sysfs_init(L_misc_device.this_device)) < 0 )
    {
        error(" sysfs error");
        failed(6);
        goto MISC_IRQ_I2CDRV_PSYS_DREG;
    }
    #endif

    wake_lock_init(&P_sensor_wake_lock, WAKE_LOCK_SUSPEND, "P_sensor");

    trace_out();
    return ret;

MISC_IRQ_I2CDRV_PSYS_DREG:
#ifndef CONFIG_MACH_OSCAR
    P_sysfs_exit(P_misc_device.this_device);
#else
    P_sysfs_exit(&P_sensor_dev);
#endif
    
MISC_IRQ_I2CDRV_DREG:

    PL_i2c_drv_exit();

MISC_IRQ_DREG:
    free_irq(P_IRQ, (void *)NULL);

MISC_DREG2:
    misc_deregister(&L_misc_device);
MISC_DREG1:
    misc_deregister(&P_misc_device);
    return ret; 
}

void __exit PL_driver_exit(void)
{
    trace_in(); 

#ifndef CONFIG_MACH_OSCAR
    L_sysfs_exit(L_misc_device.this_device);
    P_sysfs_exit(P_misc_device.this_device);    
#else
    P_sysfs_exit(&P_sensor_dev);    
#endif
		  
    /*Delete the i2c driver*/
    PL_i2c_drv_exit();

    /*Proximity sensor: IRQ*/
    free_irq(P_IRQ, (void *)NULL);

    /*Light sensor: misc device deregistration*/
    misc_deregister(&L_misc_device);    
    
    /*Proximity sensor: misc device deregistration*/
    misc_deregister(&P_misc_device);

    pl_power_exit();
    trace_out();
}

module_param(an_sleep_func, ushort, S_IRUGO|S_IWUSR);
module_param(detec_cyc, ushort, S_IRUGO|S_IWUSR);
module_param_array(L_table, uint, NULL, S_IRUGO|S_IWUSR);

module_init(PL_driver_init);
module_exit(PL_driver_exit);
MODULE_AUTHOR("Varun Mahajan <m.varun@samsung.com>");
MODULE_DESCRIPTION("GP2AP002A00F light/proximity sensor driver");
MODULE_LICENSE("GPL");
