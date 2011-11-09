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
#include <linux/input.h>    // joon.c.baek, for input_device_driver
#include <linux/poll.h>

#include <plat/mux.h>
#include <mach/hardware.h>
#include <mach/gpio.h>

#include <asm/irq.h>
#include <linux/workqueue.h>
#include <linux/wait.h>
#include <linux/stat.h>
#include <linux/ioctl.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>

#include "KXSD9_sysfs.h"
#include "KXSD9_i2c_drv.h"
#include "KXSD9_dev.h"
#include "common.h"

#ifdef CONFIG_MACH_OSCAR
#include <linux/sensors_core.h>
#endif

#ifdef CONFIG_MACH_OSCAR
    #define DEFAULT_POLLING_INTERVAL    (200)
#elif CONFIG_ARCHER_TARGET_SK
    #define DEFAULT_POLLING_INTERVAL    (200)  // Archer_LSJ DB22 : 250 => 200
#else
    #define DEFAULT_POLLING_INTERVAL    (250)
#endif 

/*KXSD9 IRQ*/
#define KXSD9_IRQ OMAP_GPIO_IRQ(OMAP3430_GPIO_ACC_INT)

/*****************IOCTLS******************/
/*magic no*/
#define KXSD9_IOC_MAGIC  0xF6
/*max seq no*/
#define KXSD9_IOC_NR_MAX 12 

#define KXSD9_IOC_GET_ACC           _IOR(KXSD9_IOC_MAGIC, 0, KXSD9_acc_t)
#define KXSD9_IOC_DISB_MWUP         _IO(KXSD9_IOC_MAGIC, 1)
#define KXSD9_IOC_ENB_MWUP          _IOW(KXSD9_IOC_MAGIC, 2, unsigned short)
#define KXSD9_IOC_MWUP_WAIT         _IO(KXSD9_IOC_MAGIC, 3)
#define KXSD9_IOC_GET_SENSITIVITY   _IOR(KXSD9_IOC_MAGIC, 4, KXSD9_sensitivity_t)
#define KXSD9_IOC_GET_ZERO_G_OFFSET _IOR(KXSD9_IOC_MAGIC, 5, KXSD9_zero_g_offset_t )  
#define KXSD9_IOC_GET_DEFAULT       _IOR(KXSD9_IOC_MAGIC, 6, KXSD9_acc_t)
#define KXSD9_IOC_SET_RANGE         _IOWR( KXSD9_IOC_MAGIC, 7, int )
#define KXSD9_IOC_SET_BANDWIDTH     _IOWR( KXSD9_IOC_MAGIC, 8, int )
#define KXSD9_IOC_SET_MODE          _IOWR( KXSD9_IOC_MAGIC, 9, int )
#define KXSD9_IOC_SET_MODE_AKMD2    _IOWR( KXSD9_IOC_MAGIC, 10, int )
#define KXSD9_IOC_GET_INITIAL_VALUE _IOWR( KXSD9_IOC_MAGIC, 11, kxsd9_convert_t )
#define KXSD9_IOC_SET_DELAY         _IOWR( KXSD9_IOC_MAGIC, 12, int )
/*****************************************/

KXSD9_status_t KXSD9_status;

int KXSD9_operation_mode;

KXSD9_acc_t latest_acc ;
void (*KXSD9_report_func)( KXSD9_acc_t* ) ;

KXSD9_module_param_t *KXSD9_main_getparam( void ) ;

static struct workqueue_struct* accel_wq ;

/*file operatons*/
static int KXSD9_open (struct inode *, struct file *);
static int KXSD9_release (struct inode *, struct file *);
static int KXSD9_ioctl(struct inode *, struct file *, unsigned int,  unsigned long);
static ssize_t KXSD9_read(struct file *filp, char *buf, size_t count, loff_t *ofs);
static unsigned int KXSD9_poll(struct file *filp, struct poll_table_struct *pwait);

/*work struct*/
static void work_function( struct work_struct* ) ;
static DECLARE_DELAYED_WORK( KXSD9_ws, work_function ) ;

/*input device, for input_report_abs()*/
static struct input_dev* input_dev ;

/*module param. default values*/
static unsigned short mwup = DISABLED;
static unsigned short mwup_rsp = DONT_CARE;
static unsigned short R_T = R2g_T1g;
static unsigned short BW = BW50;

/*Module param struct*/
static KXSD9_module_param_t KXSD9_module_param ;

static struct file_operations KXSD9_fops =
{
    .owner = THIS_MODULE,
    .open = KXSD9_open,
    .read = KXSD9_read,
    .poll = KXSD9_poll,
    .ioctl = KXSD9_ioctl,
    .release = KXSD9_release,
};

static struct miscdevice KXSD9_misc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "accel",
    .fops = &KXSD9_fops,
};

#ifdef CONFIG_MACH_OSCAR
static struct sensors_dev sensor_dev = {
    .minor = 0,
    .fops = &KXSD9_fops,
};
#endif

KXSD9_timer_t   KXSD9_timer =
{
    .polling_timer= NULL,
    .polling_time=  DEFAULT_POLLING_INTERVAL,
    .current_timer_state= TIMER_OFF, 
    .saved_timer_state=   TIMER_OFF, 
};

#ifdef  CONFIG_ARCHER_TARGET_SK  // Archer_LSJ DA27
kxsd9_convert_t    kxsd9_convert =
{
    .zero_g_offset= 2048, 
    .divide_unit=   819,
    .axis= { 1, -1, 1 },
    .x_y_axis_changed= 1,
} ;
#else 
kxsd9_convert_t    kxsd9_convert =
{
//	.name= "bma023",
    .zero_g_offset= 2048, 
    .divide_unit=   819,
#if CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00
    .axis= { -1, -1, 1 },
    .x_y_axis_changed= 0,
#else
    .axis= { 1, 1, -1 },
    .x_y_axis_changed= 1,
#endif
} ;
#endif 

static pid_t KXSD9_thread_pid;
static KXSD9_acc_t g_acc_val;
static DECLARE_WAIT_QUEUE_HEAD(KXSD9_poll_wait);
static DECLARE_WAIT_QUEUE_HEAD(KXSD9_wait_queue);

static LIST_HEAD(process_list);
static DEFINE_SPINLOCK(list_lock);

static struct semaphore KXSD9_sem;

static atomic_t KXSD9_ref_count;
static struct early_suspend early_suspend ;
KXSD9_cal_t cal_data;

/***************************************************************/
KXSD9_module_param_t *KXSD9_main_getparam( void )
{
    return &KXSD9_module_param ;
}

static int KXSD9_new_data(KXSD9_acc_t *old, KXSD9_acc_t *new)
{
    int threshold = KXSD9_dev_get_threshold();
    if(abs(old->X_acc - new->X_acc) >= threshold || 
       abs(old->Y_acc - new->Y_acc) >= threshold)
        return 1;
    else
        return 0;
}

static int KXSD9_thread(void *arg)
{
    int ret;
    KXSD9_acc_t acc_val;
    int wait_ms;
    KXSD9_status_t *p;
    DEFINE_WAIT(wait);

    trace_in();
    
    printk("KXSD9: %s: interval[%lu], threshold[%d]\n", __func__, KXSD9_timer.polling_time, KXSD9_dev_get_threshold());

    daemonize("KXSD9 thread");

    while(1)
    {
        if(!test_bit(KXSD9_THREAD, &KXSD9_status.status))
            break;
        wait_ms = KXSD9_timer.polling_time;

        prepare_to_wait(&KXSD9_wait_queue, &wait, TASK_INTERRUPTIBLE);
        schedule_timeout(msecs_to_jiffies(wait_ms));
        finish_wait(&KXSD9_wait_queue, &wait);

        if(test_bit(KXSD9_MOVING, &KXSD9_status.status))
        {
            clear_bit(KXSD9_MOVING, &KXSD9_status.status);
            printk("KXSD9: %s: KXSD9_MOVING\n", __func__);
            continue;
        }

        if(!test_bit(KXSD9_ON, &KXSD9_status.status))
        {
            printk("KXSD9: %s: KXSD9 didn't turn on!\n", __func__);
            continue;
        }

        ret = KXSD9_dev_get_acc(&acc_val);
        if(ret < 0)
        {
            printk("KXSD9: %s: KXSD9_dev_get_acc failed[%d]!\n", __func__, ret);
            continue;
        }

        if(KXSD9_new_data(&g_acc_val, &acc_val))
        {
            debug("KXSD9: %d,%d,%d\n", acc_val.X_acc, acc_val.Y_acc, acc_val.Z_acc);
            g_acc_val = acc_val;
            spin_lock(&list_lock);
            list_for_each_entry(p, &process_list, list)
            {
                set_bit(KXSD9_NEWDATA, &p->status);
            }
            spin_unlock(&list_lock);

            wake_up(&KXSD9_poll_wait);
        }
    }
    memset(&g_acc_val, 0, sizeof(g_acc_val));

    trace_out();
    return 0;
}

void KXSD9_start_thread()
{
    int ret = 0;
    KXSD9_status_t *p;

    trace_in();

    if(down_trylock(&KXSD9_sem))
    {
        printk("KXSD9: %s: down_trylock failed!\n", __func__);
        trace_out();
        return;
    }

    if(!test_bit(KXSD9_ON, &KXSD9_status.status))
    {
        printk("KXSD9: %s: sensor didn't turn on\n", __func__);
        trace_out();
        return;
    }
    
    p = kzalloc(sizeof(*p), GFP_KERNEL);
    if(p == NULL)
    {
        printk(KERN_ERR "KSXD9: %s: kzalloc failed!\n", __func__);
        up(&KXSD9_sem);
        trace_out();
        return;
    }

    set_bit(KXSD9_NEWDATA, &p->status);

    spin_lock(&list_lock);
    list_add(&p->list, &process_list);
    spin_unlock(&list_lock);

    if(!test_bit(KXSD9_THREAD, &KXSD9_status.status))
    {
        set_bit(KXSD9_THREAD, &KXSD9_status.status);

        ret = KXSD9_dev_get_acc(&g_acc_val);
        if(ret < 0)
            printk(KERN_ERR "KXSD9: %s: set initial value failed!\n", __func__);

        KXSD9_thread_pid = kernel_thread(KXSD9_thread, &KXSD9_thread_pid, 0);
        if(KXSD9_thread_pid < 0)
        {
            printk(KERN_ERR "KXSD9: %s: start kernel_thread failed!\n", __func__);
            clear_bit(KXSD9_THREAD, &KXSD9_status.status);
            up(&KXSD9_sem);
            trace_out();
            return;
        }
    }
    atomic_inc(&KXSD9_ref_count);
    up(&KXSD9_sem);

    trace_out();
}

void KXSD9_stop_thread()
{
    KXSD9_status_t *p;

    trace_in() ;
    
    if(down_trylock(&KXSD9_sem))
    {
        trace_out();
        return;
    }
    
    spin_lock(&list_lock);
    list_for_each_entry(p, &process_list, list)
    {
        list_del(&p->list);
        kfree(p);
    }
    spin_unlock(&list_lock);

    if(atomic_dec_and_test(&KXSD9_ref_count))
    {
        clear_bit(KXSD9_THREAD, &KXSD9_status.status);
    }

    up(&KXSD9_sem);
    trace_out() ;
}

static int KXSD9_open (struct inode *inode, struct file *filp)
{
    int ret = 0;
#ifdef CONFIG_MACH_OSCAR
    trace_in() ; 
    
    if(down_trylock(&KXSD9_sem))
    {
        printk(KERN_ERR "KXSD9: %s: down_trylock failed!\n", __func__);
        trace_out();
        return -ERESTARTSYS;
    }
    
    set_bit(KXSD9_NEWDATA, &KXSD9_status.status);
    atomic_inc(&KXSD9_ref_count);
    up(&KXSD9_sem);
        
    trace_out() ;
#else
    trace_in();
    nonseekable_open(inode, filp);
    trace_out();
#endif
    return ret;
}

static ssize_t KXSD9_read (struct file *filp, char *buf, size_t count, loff_t *ofs)
{
    struct input_event data[3];
    ssize_t size = sizeof(data);
    int ret;

    trace_in();

    if(down_trylock(&KXSD9_sem))
        return -ERESTARTSYS;

    if(count < size)
    {
        ret = -EINVAL;
        goto err_out;
    }

    do_gettimeofday(&(data[0].time));
    data[0].type = EV_ABS;
    data[0].code = ABS_X;
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
    data[0].value = (__s32)g_acc_val.X_acc - 2048;
#else
    data[0].value = (__s32)g_acc_val.Y_acc - 2048;
#endif
    
    data[1].time = data[0].time;
    data[1].type = EV_ABS;
    data[1].code = ABS_Y;

#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
    data[1].value = (__s32)g_acc_val.Y_acc - 2048;
#else
    data[1].value = (__s32)g_acc_val.X_acc - 2048;
#endif

    data[2].time = data[0].time;
    data[2].type = EV_ABS;
    data[2].code = ABS_Z;
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
    data[2].value = -((__s32)g_acc_val.Z_acc - 2048);
#else
    data[2].value = (__s32)g_acc_val.Z_acc - 2048;
#endif

    if(copy_to_user(buf, &data, size)) 
    {
        printk(KERN_ERR "%s: copy_to_user error!\n", __func__);
        trace_out();
        return -EIO;
    }

    *ofs += size;
    
    clear_bit(KXSD9_NEWDATA, &KXSD9_status.status);
    up(&KXSD9_sem);
    trace_out();
   
    debug("x[%d], y[%d], z[%d]\n", g_acc_val.X_acc, g_acc_val.Y_acc, g_acc_val.Z_acc);
    return size;

err_out:
    up(&KXSD9_sem);
    trace_out();
    return ret;
}


static unsigned int KXSD9_poll(struct file *filp, struct poll_table_struct *pwait)
{
    unsigned int mask = 0;

    trace_in();

    if (down_trylock(&KXSD9_sem))
    {
        trace_out();
        return -ERESTARTSYS;
    }

    poll_wait(filp, &KXSD9_poll_wait, pwait);

    if (!test_bit(KXSD9_FIRST_TIME, &KXSD9_status.status)) {
        set_bit(KXSD9_FIRST_TIME, &KXSD9_status.status);
        mask |= (POLLIN | POLLRDNORM);
        goto out;
    }

    if (test_bit(KXSD9_MOVING, &KXSD9_status.status))
        goto out;		

    if (test_bit(KXSD9_NEWDATA, &KXSD9_status.status))  
        mask |= (POLLIN | POLLRDNORM);

     trace_out();
out:
     up(&KXSD9_sem);
     trace_out();
     return mask;
}

static int KXSD9_release (struct inode *inode, struct file *filp)
{
#ifdef CONFIG_MACH_OSCAR
    trace_in() ;
    
    if(down_trylock(&KXSD9_sem))
    {
        trace_out();
        return -ERESTARTSYS;
    }
    
    if(atomic_dec_and_test(&KXSD9_ref_count))
    {
        clear_bit(KXSD9_THREAD, &KXSD9_status.status);
    }

    up(&KXSD9_sem);
    trace_out() ;
#else
    trace_in();
    trace_out();
#endif
    return 0;
}

void work_function( struct work_struct* unused )
{
    KXSD9_acc_t *acc = NULL;
    
    trace_in() ;

    if( (acc = (KXSD9_acc_t *)kzalloc(sizeof(KXSD9_acc_t), GFP_KERNEL)) == NULL )
    {
        printk("%s: KXSD9_ioctl: no memory\n", __FUNCTION__ );
    }
    else if( KXSD9_dev_get_acc(acc) < 0 )
    {
        printk("%s: KXSD9_dev_get_acc failed\n", __FUNCTION__ );	
    }     
    
    if( acc == NULL ) 
    {
        printk("%s: work_function: acc is NULL\n", __FUNCTION__ );	
    }

    KXSD9_report_func( acc ) ;

    if( acc != NULL )
    {
        kfree(acc);
    }

    queue_delayed_work( accel_wq, &KXSD9_ws, msecs_to_jiffies(KXSD9_timer.polling_time) ) ;

    trace_out() ;
}



static int KXSD9_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_cmd,  unsigned long arg)
{
    int ret = 0;
    int range = 0 , bandwidth = 0 , mode = 0 ;
    unsigned short mwup_rsp;
    void __user *argp = (void __user *)arg;   
    int delay ;
    trace_in() ;

    if( _IOC_TYPE(ioctl_cmd) != KXSD9_IOC_MAGIC )
    {
        printk("%s: Inappropriate kxsd9 ioctl 1 0x%x", __FUNCTION__, ioctl_cmd);
        trace_out() ;
        return -ENOTTY;
    }
    if( _IOC_NR(ioctl_cmd) > KXSD9_IOC_NR_MAX )
    {
        printk("%s: Inappropriate kxsd9 ioctl 2 0x%x", __FUNCTION__, ioctl_cmd);	
        trace_out() ;
        return -ENOTTY;
    }

    switch (ioctl_cmd)
    {
        case KXSD9_IOC_SET_DELAY:
            {                
                ret= copy_from_user( (void*)&delay, 
                                        argp, 
                                     sizeof(int) ) ;
                if( ret < 0 )
                {
                    printk( "KXSD9_IOC_SET_DELAY failed, ret= %d\n", ret ) ;
                    ret= -EFAULT ;
                }
                else
                {
                	KXSD9_timer_set_polling_time( delay ) ;
                }
            }
    	break; 
        case KXSD9_IOC_GET_INITIAL_VALUE:  
            {
            	debug( "divide_unit= %d\n", kxsd9_convert.divide_unit ) ;
            	debug( "kxsd9_convert_t size= %d\n", sizeof(
            	kxsd9_convert_t ) ) ;
                if( copy_to_user( (kxsd9_convert_t*)arg, 
                            &kxsd9_convert, 
                            sizeof(kxsd9_convert_t) ) )
                {
                    ret = -EFAULT;
                }   
                debug( "kxsd9 initialize value transferred to akmd2 or sensorhal, %d", ret ) ;
            }
            break ;

        case KXSD9_IOC_GET_ACC:
            {
                if( copy_to_user( argp, (const void *)&latest_acc, sizeof(KXSD9_acc_t) ) )
                {
                    ret = -EFAULT;
                }

                /*
                debug_value( "X-Axis: %d\t, Y-Axis: %d\t, Z-Axis: %d",
                        latest_acc.X_acc, latest_acc.Y_acc, latest_acc.Z_acc ) ;
                */
            }
            break;

        case KXSD9_IOC_GET_DEFAULT:
            {
                KXSD9_default_t *acc = NULL;

                if( (acc = (KXSD9_default_t *)kzalloc(sizeof(KXSD9_default_t), GFP_KERNEL)) == NULL )
                {
                    printk("%s: KXSD9_ioctl: no memory", __FUNCTION__ );
                    ret = -ENOMEM;
                }
                else if( (ret = KXSD9_dev_get_default(acc)) < 0 )
                {
                    printk("%s: KXSD9_dev_get_default failed", __FUNCTION__ );	
                }     
                else if( copy_to_user( argp, (const void *) acc, sizeof(KXSD9_default_t) ) )
                {
                    ret = -EFAULT;
                }

                /*
                debug( "X-Axis: %d %d\t, Y-Axis: %d %d\t, Z-Axis: %d \
                        %d\t\n", acc->x1, acc->x2, acc->y1, acc->y2, acc->z1, acc->z2 ) ;
                */;
                kfree(acc);
            }
            break;

        case KXSD9_IOC_GET_SENSITIVITY:
            {
                KXSD9_sensitivity_t *sensitivity = NULL;

                if( (sensitivity = (KXSD9_sensitivity_t *)
                            kzalloc(sizeof(KXSD9_sensitivity_t), GFP_KERNEL)) == NULL )
                {
                    printk("%s: KXSD9_ioctl: no memory", __FUNCTION__ );
                    ret = -ENOMEM;
                }
                else if( (ret = KXSD9_dev_get_sensitivity(sensitivity)) < 0 )
                {
                    printk("%s: KXSD9_dev_get_sensitivity failed", __FUNCTION__ );	
                }     
                else if( copy_to_user( argp, (const void *) sensitivity, sizeof(KXSD9_sensitivity_t) ) )
                {
                    ret = -EFAULT;
                }

                printk( "Sensitivity: %d, error_RT: %d\n", \
                        sensitivity->counts_per_g, sensitivity->error_RT ) ;
                kfree(sensitivity);
            }
            break;

        case KXSD9_IOC_GET_ZERO_G_OFFSET:
            {
                KXSD9_zero_g_offset_t *offset = NULL;

                if( (offset= (KXSD9_zero_g_offset_t *)
                            kzalloc(sizeof(KXSD9_zero_g_offset_t), GFP_KERNEL)) == NULL )
                {
                    printk("%s: KXSD9_ioctl: no memory", __FUNCTION__ );
                    ret = -ENOMEM;
                }
                else if( (ret = KXSD9_dev_get_zero_g_offset(offset)) < 0 )
                {
                    printk("%s: KXSD9_dev_get_zero_g_offset failed", __FUNCTION__ );	
                }     
                else if( copy_to_user( argp, (const void *) offset, sizeof(KXSD9_zero_g_offset_t) ) )
                {
                    ret = -EFAULT;
                }

                kfree(offset);
            }
            break;            

        case KXSD9_IOC_DISB_MWUP:
            {
                ret = KXSD9_dev_mwup_disb();
            }

            break;

        case KXSD9_IOC_ENB_MWUP:
            {
                if( copy_from_user( (void*) &mwup_rsp, argp, sizeof(int) ) )
                {
                    ret = -EFAULT;
                }
                else
                {
                    ret = KXSD9_dev_mwup_enb(mwup_rsp);
                }           	    
            }
            break;

            /* Add for AKMD2 */
        case KXSD9_IOC_SET_RANGE:
            {
                if( copy_from_user( (void*)&range, argp, sizeof(int) ) )
                {
                    ret = -EFAULT;
                }
                else
                {
                    ret = KXSD9_dev_set_range( range );
                }           	    
            }
            break ;

        case KXSD9_IOC_SET_BANDWIDTH:
            {
                if( copy_from_user( (void*)&bandwidth, argp, sizeof(int) ) )
                {
                    ret= -EFAULT ;
                }
                else
                {
                    ret = KXSD9_dev_set_bandwidth( bandwidth ) ;
                }
            }
            break ;

        case KXSD9_IOC_SET_MODE:
            {
                if( copy_from_user( (void*)&mode, argp, sizeof(int) ) )
                {
                    ret= -EFAULT ;
                }
                else 
                {
                    ret = KXSD9_dev_set_operation_mode( ACCEL, mode ) ;
                }
            }
            break ;

        case KXSD9_IOC_SET_MODE_AKMD2:
            {
                if( copy_from_user( (void*)&mode, argp, sizeof(int) ) )
                {
                    ret= -EFAULT ;
                }
                else 
                {
                    ret = KXSD9_dev_set_operation_mode( COMPASS, mode ) ;
                }
            }
            break ;

        default:
            printk( "%s: ERROR!!! default!!!\n", __FUNCTION__ ) ;
            ret = -ENOTTY;
            break;
    }

    trace_out() ;
    return ret;
}

/************************************************************/

int KXSD9_timer_init( void )
{
    return 0 ;
}

int KXSD9_timer_exit( void )
{
    return 0 ;
}

int KXSD9_timer_enable( unsigned long timeout ) 
{
    trace_in() ;

    if( KXSD9_timer.current_timer_state == TIMER_ON )
    {
        printk( "Timer ON already!!!\n" ) ;
        trace_out();
        return KXSD9_timer.current_timer_state ;
    }   

    KXSD9_timer.polling_time= timeout ;

    cancel_delayed_work_sync( &KXSD9_ws ) ;
    queue_delayed_work( accel_wq, &KXSD9_ws, msecs_to_jiffies(KXSD9_timer.polling_time) ) ;
    
    KXSD9_timer.current_timer_state= TIMER_ON ;

    printk( "[ACCEL] KXSD9_timer_enable() Success!!!\n" ) ;

    trace_out() ;
    return KXSD9_timer.current_timer_state ;
}

int KXSD9_timer_disable( void )
{
    trace_in() ;

    if( KXSD9_timer.current_timer_state == TIMER_OFF )
    {
        printk( "Timer OFF already!!!\n" ) ;
        trace_out();
        return KXSD9_timer.current_timer_state ;
    }   

    flush_work( (struct work_struct *)&KXSD9_ws ) ;
    cancel_delayed_work_sync( &KXSD9_ws ) ;

    KXSD9_timer.current_timer_state= TIMER_OFF ;

    printk( "[ACCEL] KXSD9_timer_disable() Success!!!\n" ) ;

    trace_out() ;
    return KXSD9_timer.current_timer_state ;
}

void KXSD9_timer_timeout( unsigned long dataAddr )
{
    trace_in() ;

    schedule_delayed_work( &KXSD9_ws, 0 ) ;

    trace_out() ;
}

unsigned long KXSD9_timer_get_polling_time( void )
{
    return KXSD9_timer.polling_time ;
}

void KXSD9_timer_set_polling_time( int timeout )
{
    trace_in() ;

    if( timeout > 0 )
    {
        KXSD9_timer.polling_time= timeout ;
    }
    else
    {
        KXSD9_timer.polling_time= DEFAULT_POLLING_INTERVAL ;
    }
    printk("%s: delay= %d\n", __FUNCTION__, (int)KXSD9_timer.polling_time );

    trace_out() ;
}

void KXSD9_report_sensorhal( KXSD9_acc_t* acc )
{
    trace_in() ;

    input_report_abs( input_dev, ABS_X, acc->X_acc ) ;
    input_report_abs( input_dev, ABS_Y, acc->Y_acc ) ;
    input_report_abs( input_dev, ABS_Z, acc->Z_acc ) ;
    input_sync( input_dev ) ;

    if(KXSD9_new_data(&g_acc_val, acc))	
    {
        g_acc_val = *acc;
        set_bit(KXSD9_NEWDATA, &KXSD9_status.status);
    }
    // twisted axis, x <-> y, for OSCAR rev0.1
    /*
    input_report_abs( input_dev, ABS_X, acc->Y_acc ) ;
    input_report_abs( input_dev, ABS_Y, acc->X_acc ) ;
    input_report_abs( input_dev, ABS_Z, acc->Z_acc ) ;
    input_sync( input_dev ) ;
    */
    
    trace_out() ;
}

void KXSD9_report_akmd2( KXSD9_acc_t* acc )
{
    trace_in() ;

    latest_acc.X_acc= acc->X_acc ;
    latest_acc.Y_acc= acc->Y_acc ;
    latest_acc.Z_acc= acc->Z_acc ;

    if(KXSD9_new_data(&g_acc_val, acc))	
    {
        g_acc_val = *acc;
        set_bit(KXSD9_NEWDATA, &KXSD9_status.status);
    }
    // twisted axis, x <->y, for OSCAR rev0.1
    /*
    latest_acc.X_acc= acc->Y_acc ;
    latest_acc.Y_acc= acc->X_acc ;
    latest_acc.Z_acc= acc->Z_acc ;
    */

    trace_out() ;
}

void KXSD9_report_sensorhal_akmd2( KXSD9_acc_t* acc )
{
    trace_in() ;

    latest_acc.X_acc= acc->X_acc ;
    latest_acc.Y_acc= acc->Y_acc ;
    latest_acc.Z_acc= acc->Z_acc ;

    input_report_abs( input_dev, ABS_X, acc->X_acc ) ;
    input_report_abs( input_dev, ABS_Y, acc->Y_acc ) ;
    input_report_abs( input_dev, ABS_Z, acc->Z_acc ) ;
    input_sync( input_dev ) ;

    if(KXSD9_new_data(&g_acc_val, acc))	
    {
        g_acc_val = *acc;
        set_bit(KXSD9_NEWDATA, &KXSD9_status.status);
    }
    // twisted axis, x <->y, for OSCAR rev0.1
    /*
    latest_acc.X_acc= acc->Y_acc ;
    latest_acc.Y_acc= acc->X_acc ;
    latest_acc.Z_acc= acc->Z_acc ;

    // twisted axis, x <-> y, for OSCAR rev0.1
    input_report_abs( input_dev, ABS_X, acc->Y_acc ) ;
    input_report_abs( input_dev, ABS_Y, acc->X_acc ) ;
    input_report_abs( input_dev, ABS_Z, acc->Z_acc ) ;
    input_sync( input_dev ) ;
    */

    trace_out() ;
}

static int KXSD9_early_suspend( struct early_suspend* handler )
{
    int ret = 0;
    trace_in() ;
  
    if( (ret = KXSD9_dev_disable()) < 0 )
    {
        printk("%s: KXSD9_dev_disable failed", __FUNCTION__ );
    }

    if( (ret= KXSD9_timer_disable()) != TIMER_OFF ) 
    {
        printk( "%s: KXSD9_timer_ON, this is fail", __FUNCTION__ ) ;
        trace_out();
        return ret ;
    }

    printk( "%s: Early suspend sleep success!!!\n", __FUNCTION__ ) ;
    trace_out() ; 
    return ret;
}

static int KXSD9_early_resume( struct early_suspend* handler )
{
    int ret = 0;
    trace_in() ;
    
    if( (ret = KXSD9_dev_enable()) < 0 )
    {
        printk("%s: KXSD9_dev_enable failed", __FUNCTION__ );
    }

    if( KXSD9_timer.saved_timer_state == TIMER_ON ) 
    {
        if( (ret= KXSD9_timer_enable( POLLING_INTERVAL )) != TIMER_ON ) 
        {
            printk( "%s: KXSD9_timer_OFF, this is fail", __FUNCTION__ ) ;
            trace_out();
            return ret ;
        }
    }
 
    printk( "%s: Early suspend wakeup success!!!\n", __FUNCTION__ ) ;
    trace_out() ; 
    return ret;
}

static ssize_t KXSD9_calibration_store(struct device *dev,
                                       struct device_attribute *attr,
                                       const char *buf, size_t count)
{
    KXSD9_calibrate();
    return count;
}

static ssize_t KXSD9_calibration_show(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
    KXSD9_init_calibration();
    return sprintf(buf, "%d %d %d\n", cal_data.x, cal_data.y, cal_data.z);
}

static DEVICE_ATTR(calibration, 0666, KXSD9_calibration_show, KXSD9_calibration_store);
static struct attribute *KXSD9_attributes[] = {
    &dev_attr_calibration.attr,
    NULL
};
static struct attribute_group KXSD9_attribute_group = {
    .attrs = KXSD9_attributes
};

/************************************************************/

int __init KXSD9_driver_init(void)
{
    int ret = 0;
    trace_in() ;

    KXSD9_module_param.mwup = mwup;
    KXSD9_module_param.mwup_rsp = mwup_rsp;
    KXSD9_module_param.R_T = R_T;
    KXSD9_module_param.BW = BW;	

    /*Initilize the KXSD9 dev mutex*/
    KXSD9_dev_mutex_init();

    /*Add the i2c driver*/
    if ( (ret = KXSD9_i2c_drv_init() < 0) ) 
    {
         goto MISC_IRQ_DREG;
    }

    /*Add the input device driver*/
    input_dev= input_allocate_device() ;

    if( !input_dev ) {
    	error( "Input_allocate_deivce failed!" ) ;
      trace_out();
    	return -ENODEV ;
    }

    set_bit( EV_ABS, input_dev->evbit ) ;
    set_bit( EV_SYN, input_dev->evbit ) ;

    input_set_abs_params( input_dev, ABS_X, 0, 4095, 0, 0 ) ;
    input_set_abs_params( input_dev, ABS_Y, 0, 4095, 0, 0 ) ;
    input_set_abs_params( input_dev, ABS_Z, 0, 4095, 0, 0 ) ;

    input_dev->name= "accel" ;

    ret= input_register_device( input_dev ) ;
    if( ret ) {
      	error( "Unable to register Input device: %s\n", input_dev->name ) ;
        trace_out();
      	return ret ;
    }

    ret = sysfs_create_group(&input_dev->dev.kobj, &KXSD9_attribute_group);
    if(ret < 0)
    {
        error("KXSD9_driver_init sysfs_create_group failed[%d!\n", ret);
    }

    /*misc device registration*/
    if( (ret = misc_register(&KXSD9_misc_device)) < 0 )
    {
        error("KXSD9_driver_init misc_register failed: %s\n", __FUNCTION__ );
        goto MISC_DREG;
    }

#ifdef CONFIG_MACH_OSCAR
    init_MUTEX(&KXSD9_sem);
#endif

    /*
    set_irq_type ( KXSD9_IRQ, IRQF_TRIGGER_RISING );
    */

    /*
    if( (ret = request_irq(KXSD9_IRQ,KXSD9_isr,0 ,"accel", (void *)NULL)) < 0 )
    {
        error("KXSD9_driver_init request_irq failed %d",KXSD9_IRQ);
    }
    */

    INIT_DELAYED_WORK( &KXSD9_ws, (void(*)(struct work_struct*))work_function ) ;
    accel_wq= create_singlethread_workqueue( "accel_wq" ) ;
    if( accel_wq < 0 )
    {
      	printk( "%s, Can't create_singlethread_workqueue, ret= %d\n", __FUNCTION__, (int)accel_wq ) ;
      	goto MISC_DREG ;
    }

#ifndef CONFIG_MACH_OSCAR
    if( (ret= KXSD9_sysfs_init( KXSD9_misc_device.this_device )) < 0 )
#else
    if( (ret= KXSD9_sysfs_init( &sensor_dev )) < 0 )
#endif
    {
      	error( "Unable to sysfs init!!!" ) ;
      	goto SYSFS_DREG ;
    }
 
#ifdef  CONFIG_HAS_EARLYSUSPEND
    early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1 ;
    early_suspend.suspend = (void *)KXSD9_early_suspend;
    early_suspend.resume = (void *)KXSD9_early_resume;
    register_early_suspend(&early_suspend);
    debug("kxsd9: early_suspend registered");
#endif // CONFIG_HAS_EARLYSUSPEND
   
    KXSD9_init_calibration();

    trace_out() ;
    return ret;

SYSFS_DREG:
#ifndef CONFIG_MACH_OSCAR
    KXSD9_sysfs_exit( KXSD9_misc_device.this_device ) ;
#else
    KXSD9_sysfs_exit( &sensor_dev );
#endif

MISC_IRQ_DREG:
/*
    free_irq(KXSD9_IRQ, (void *)NULL);
    */

MISC_DREG:
    misc_deregister(&KXSD9_misc_device);

    trace_out() ;
    return ret; 
}


void __exit KXSD9_driver_exit(void)
{
    trace_in() ;

    /*Delete the i2c driver*/
    KXSD9_i2c_drv_exit();

    sysfs_remove_group(&input_dev->dev.kobj, &KXSD9_attribute_group);

    /*
    free_irq(KXSD9_IRQ, (void *)NULL);
    */
    
    input_unregister_device( input_dev ) ;

    destroy_workqueue( accel_wq ) ;

    /*misc device deregistration*/
    misc_deregister(&KXSD9_misc_device);

#ifndef CONFIG_MACH_OSCAR
    KXSD9_sysfs_exit( KXSD9_misc_device.this_device ) ;
#else
    KXSD9_sysfs_exit( &sensor_dev );
#endif

    trace_out() ;
}

void KXSD9_calibrate()
{
	int sum_x = 0;
	int sum_y = 0;
	int sum_z = 0;
	KXSD9_acc_t cur_val = {0,0,0};
	int i = 0;
	struct file *filp_calibrate = NULL;
	int len = 0;
	mm_segment_t old_fs;

	cal_data.x = cal_data.y = cal_data.z = 0;

	for(; i < 20; i++)
	{
		KXSD9_dev_get_acc(&cur_val);
		sum_x += cur_val.X_acc;
		sum_y += cur_val.Y_acc;
		sum_z += cur_val.Z_acc;
	}

	cal_data.x = (sum_x / 20) - 2048;
	cal_data.y = (sum_y / 20) - 2048;
	cal_data.z = (sum_z / 20) - 2867;

    #if 0 
	if(cal_data.x < 0) cal_data.x = 0;
	if(cal_data.y < 0) cal_data.y = 0;
	if(cal_data.z < 0) cal_data.z = 0;
    #endif 

	printk(KERN_ERR "%s (%d,%d,%d)\n", __func__, cal_data.x, cal_data.y, cal_data.z);
	
	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp_calibrate = filp_open("/data/system/calibration_data", O_CREAT|O_TRUNC|O_WRONLY,0666);
	if(IS_ERR(filp_calibrate))
	{
		printk(KERN_ERR "%s: Can't open the file for calibration\n", __func__);
		return;
	}

	len = filp_calibrate->f_op->write(filp_calibrate, (char*)&cal_data.x, sizeof(short), &filp_calibrate->f_pos);
	if(len < 1)
	{
		printk(KERN_ERR "%s: Can't write the calibration data into the file\n", __func__);
		return;
	}
	len = filp_calibrate->f_op->write(filp_calibrate, (char*)&cal_data.y, sizeof(short), &filp_calibrate->f_pos);
	len = filp_calibrate->f_op->write(filp_calibrate, (char*)&cal_data.z, sizeof(short), &filp_calibrate->f_pos);

	filp_close(filp_calibrate, current->files);
	set_fs(old_fs);
}

void KXSD9_init_calibration()
{
	struct file *filp_calibrate = NULL;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	filp_calibrate = filp_open("/data/system/calibration_data", O_RDONLY, 0666);
	if(IS_ERR(filp_calibrate))
	{
		printk(KERN_ERR "%s: Can't open the file for calibration\n", __func__);
		return;
	}

	filp_calibrate->f_op->read(filp_calibrate, (char*)&cal_data.x, sizeof(short), &filp_calibrate->f_pos);	
	filp_calibrate->f_op->read(filp_calibrate, (char*)&cal_data.y, sizeof(short), &filp_calibrate->f_pos);	
	filp_calibrate->f_op->read(filp_calibrate, (char*)&cal_data.z, sizeof(short), &filp_calibrate->f_pos);	

	printk(KERN_ERR "%s (%u,%u,%u)\n", __func__, cal_data.x, cal_data.y, cal_data.z);

	filp_close(filp_calibrate, current->files);
	set_fs(old_fs);
}

module_param(mwup, ushort, S_IRUGO|S_IWUSR);
module_param(mwup_rsp, ushort, S_IRUGO|S_IWUSR);
module_param(R_T, ushort, S_IRUGO|S_IWUSR);
module_param(BW, ushort, S_IRUGO|S_IWUSR);

module_init(KXSD9_driver_init);
module_exit(KXSD9_driver_exit);

MODULE_AUTHOR("joon.c.baek <joon.c.baek@samsung.com>");
MODULE_DESCRIPTION("KXSD9 accel driver");
MODULE_LICENSE("GPL");
