/*
 * P_dev.c 
 *
 * Description: This file implements the Proximity sensor module
 *
 * Author: Varun Mahajan <m.varun@samsung.com>
 */

#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

//#include <asm/arch/power_companion.h>
//#include <asm/arch/prcm_34xx.h>

//#include <asm/arch/twl4030.h>
//#include <asm/arch/resource.h>

//[[ ryun 
#include <linux/i2c/twl.h>
#include <linux/input.h>

#define I2C_M_WR 0
#define PHY_TO_OFF_PM_RECIEVER(p)	(p - 0x5b)
//]] ryun

#include "P_regs.h"
#include "main.h"
#include "P_dev.h"
#include "common.h"
#include "sysfs.h"

#ifdef CONFIG_MACH_OSCAR
#include <linux/gpio.h>
#include <plat/mux.h>
#include <mach/hardware.h>
#include <mach/gpio.h>
#endif

/*wakeup source*/
// extern pm_wakeup_src_t waked_src; // ryun

/**************************************************************/
#define  NOT_DETECTED  1
#define  DETECTED      0

/*power state*/
#define  SHUTDOWN      P_SHUTDOWN
#define  OPERATIONAL   P_OPERATIONAL

/*detection distance*/
#define  D_40_mm       40
#define  D_50_mm       50

/*detection hysteresis*/
#define  HYST_0p       0  /*0% (polling mode)*/
#define  HYST_10p      10 /*10%*/
#define  HYST_20p      20 /*20%*/

/*interrupt state*/
#define  DISABLED      0
#define  ENABLED       1
/**************************************************************/

/*Data Structures*/
/**************************************************************/
/*
 * operation_mode: mode of operation
 * detection_dist: detection distance
 * detection_dist_hyst: detection distance hysteresis
 * detection_cyc: detection cycle
 * sleep_func_state: analog sleep function state
 * int_state: interrupt state
 */
typedef struct
{
   u16 operation_mode;
   u16 detection_dist;
   u16 detection_dist_hyst;
   u16 detection_cyc;
   u16 sleep_func_state;
   u16 int_status;
}P_dev_settings_t;

/*
 * lock: mutex to serialize the access to this data structure 
 * client: pointer to the i2c client struture for this device
 * t2_vaux1_ldo: res_handle structure for VAUX1 LDO in TWL4030
 * power_state: operational or shutdown
 * device_state: specifies whether the sensor is present in the system or not 
 * regs: sensor's registers
 */
typedef struct 
{
    struct mutex lock;
	   struct i2c_client const *client;
	   struct res_handle *t2_vaux1_ldo;
    u16 power_state;
    P_dev_settings_t settings;
    u16 device_state;
    u8 regs[NUM_OF_REGS];
	struct input_dev *inputdevice;	// ryun
}P_device_t;
/**************************************************************/

/*extern functions*/
/**************************************************************/
void P_dev_sync_mech_init(void); 
void P_dev_an_func_state_init(u16);
void P_dev_detec_cyc_init(u16);

int P_dev_init_detec_cyc(u16 *);

int P_dev_init(struct i2c_client *);
int P_dev_exit(void);

int P_dev_shutdown(void);
int P_dev_return_to_op(void);

int P_dev_reset(void);

void P_dev_check_wakeup_src(void);

int P_dev_get_mode(u16 *);
int P_dev_get_pwrstate_mode(u16 *, u16 *);

void P_dev_work_func(struct work_struct *);

int P_dev_powerup_set_op_mode(u16);

int P_dev_get_prox_output(u16 *);

int pl_sensor_power_on();
int pl_sensor_power_off();

/**************************************************************/

/*static functions*/
/**************************************************************/
static void enable_int(void);
static void disable_int(void);

static int vaux1_ldo_up(void);
static int vaux1_ldo_down(void);
static void vaux1_ldo_map_active(void);
static void vaux1_ldo_map_default(void);

static int make_operational(void);
static int shutdown(void);

static int set_mode(u16, u16, u16);

static int get_prox_output(u16 *);

static int i2c_read(u8);
static int i2c_write(u8);
/**************************************************************/

/*Proximity sensor device structure*/
/**************************************************************/
static P_device_t P_dev =
{
    .client = NULL,
    .power_state = SHUTDOWN,
    .settings = 
    	{
    	    .operation_mode = P_NOT_INITIALIZED,
    	    .detection_cyc = P_CYC_8ms, 
    	    .sleep_func_state = P_AN_SLEEP_OFF, 
    	    .int_status = ENABLED, 
    	},
    .device_state = NOT_DETECTED,
    .inputdevice = NULL, // ryun
};    
/**************************************************************/

/*for synchronization mechanism*/
/**************************************************************/
#define LOCK()    mutex_lock(&(P_dev.lock))
#define UNLOCK()  mutex_unlock(&(P_dev.lock))

void P_dev_sync_mech_init(void)
{
    mutex_init(&(P_dev.lock));
}
/**************************************************************/

/*extern functions*/
/**************************************************************/
void P_dev_an_func_state_init(u16 state)
{
    trace_in();
 
    LOCK();

    if( (state != (u16)P_AN_SLEEP_OFF) && (state != (u16)P_AN_SLEEP_ON) )
    {
        failed(1);
    }
    else
    {
        P_dev.settings.sleep_func_state = state;
    }

    UNLOCK(); 

    trace_out();
}

void P_dev_detec_cyc_init(u16 detection_cyc)
{
    u16 cyc_range = P_CYC_1024ms;
    
    trace_in();

    LOCK();

    if( detection_cyc > cyc_range )
    {
        failed(1);
    }
    else
    {
        P_dev.settings.sleep_func_state = detection_cyc;
    }

    UNLOCK(); 
 
    trace_out();
}

int P_dev_init(struct i2c_client *client)
{
    int ret = 0;

    trace_in();

    LOCK();   

    /*delay after Vcc is enabled*/
    mdelay(1);
//    P_dev.t2_vaux1_ldo = resource_get("proximity_sensor", "t2_vaux1"); //ryun

    /*ldo will remain in default state if we shut it down without having 
          powered it up, hence the following up->down sequence*/
// [[ ryun
    if( (ret = vaux1_ldo_up()) < 0 )
    {
//        resource_put(P_dev.t2_vaux1_ldo);  
        failed(1);
    }    
    else if( (ret = vaux1_ldo_down()) < 0 )
    {
//        resource_put(P_dev.t2_vaux1_ldo);  
        failed(2);
    }
	// [[ ryun / for input device
	else if( !(P_dev.inputdevice = input_allocate_device()) )
		{
		ret = -ENOMEM;
		input_free_device(P_dev.inputdevice);
		failed(3);
		}
	set_bit(EV_ABS, P_dev.inputdevice->evbit); // ryun
	set_bit(EV_SYN, P_dev.inputdevice->evbit); // ryun
	input_set_abs_params(P_dev.inputdevice, ABS_DISTANCE, -3, 3, 0, 0);
	P_dev.inputdevice->name = "proximity sensor"; // ryun
	//
	if((ret = input_register_device(P_dev.inputdevice))<0) 
		{
		failed(4);
		}
	// ]] ryun
    else
    {
        vaux1_ldo_map_default();
        P_dev.client = client;	
        P_dev.device_state = DETECTED;
    }
// ]] ryun
    
    UNLOCK();   	 

    trace_out();
	 
    return ret;
}

int P_dev_exit(void)
{
    int ret = 0;
	 
    trace_in();

    LOCK();
// [[ ryun
/*
    if( (ret = vaux1_ldo_down()) < 0 )
    {
        resource_put(P_dev.t2_vaux1_ldo);  
        failed(1);
    }    

    resource_put(P_dev.t2_vaux1_ldo);
    vaux1_ldo_map_default();
*/ 
// ]] ryun
    P_dev.client = NULL;	
    P_dev.device_state = NOT_DETECTED;
	
	// [[ ryun / for input device
	input_unregister_device(P_dev.inputdevice);	
	// ]] ryun

    UNLOCK(); 

    trace_out();

    return ret;
}

int P_dev_shutdown(void)
{
    int ret = 0;
     
    trace_in();
 
    LOCK();   
 
    if( P_dev.device_state == NOT_DETECTED )
    {
        failed(1);
        ret = -ENODEV;   
    }
    else if ( P_dev.power_state == SHUTDOWN )
    {
        debug("    already shutdown");
        ret = 0;
    }
    else if( (ret = shutdown()) < 0 )
    {
        failed(2);
    }
    else
    {
        /*wake up the waiting processes*/
        P_waitq_wkup_proc();
    }
 
    UNLOCK(); 
 
    trace_out();
     
    return ret;
}	

int P_dev_return_to_op(void)
{
    int ret = 0;
      
    trace_in();
  
    LOCK();   
  
    if( P_dev.device_state == NOT_DETECTED )
    {
        failed(1);
        ret = -ENODEV;   
    }
    else if ( P_dev.power_state == OPERATIONAL )
    {
        debug("    already operational");
        ret = 0;
    }
    else if( (ret = make_operational()) < 0 )
    {
        failed(4);
    }
  
    UNLOCK(); 
  
    trace_out();
      
    return ret;
} 

int P_dev_reset(void)
{
    int ret = 0;
     
    trace_in();
 
    LOCK();   
 
    if( P_dev.device_state == NOT_DETECTED )
    {
        failed(1);
        ret = -ENODEV;   
    }
    else if ( P_dev.power_state == SHUTDOWN )
    {
        failed(2);
        ret = -EPERM;
    }
    else if( (ret = shutdown()) < 0 )
    {
        failed(3);
    }
    else if( (ret = make_operational()) < 0 )
    {
        failed(4);
    }     
 
    UNLOCK(); 
 
    trace_out();
     
    return ret;
}

void P_dev_check_wakeup_src(void)
{
    trace_in();
    /* System has been woken up by the proximity sensor's interrupt.
        * The ISR will not be invoked so we'll call the work function here
        */
// [[ ryun 
/*
    if( waked_src.bits.proximity )
    {
        P_dev_work_func( (struct work_struct *)NULL );
    }
*/
// ]] ryun	
    trace_out();
}

int P_dev_get_mode(u16 *mode)
{
    int ret = 0;
 
    trace_in();
 
    LOCK();   
 	debug("[ryun] P_dev_get_mode() P_dev.device_state = %d \n", P_dev.device_state);
    if( P_dev.device_state == NOT_DETECTED )
    {
        failed(1);
        ret = -ENODEV; 
    }
    else if( P_dev.power_state == SHUTDOWN )
    {
        failed(2);
        ret = -EPERM;
    }      
    else 
    {
        *mode = P_dev.settings.operation_mode;
    } 
   
    UNLOCK(); 
 
    trace_out();
 
    return ret;
}

int P_dev_get_pwrstate_mode(u16 *power_state, u16 *mode)
{
    int ret = 0;
 
    trace_in();
 
    LOCK();   
 
    if( P_dev.device_state == NOT_DETECTED )
    {
        failed(1);
        ret = -ENODEV; 
    }
    else 
    {
        *power_state = P_dev.power_state;
        *mode = P_dev.settings.operation_mode;
    } 
   
    UNLOCK(); 
 
    trace_out();
 
    return ret;
}

void P_dev_work_func(struct work_struct *work)
{
    u16 prox_op=P_OBJ_NOT_DETECTED;
    trace_in();
 
    LOCK(); 

    if( P_dev.device_state == NOT_DETECTED )
    {
        failed(1);
    }
    else if( P_dev.power_state == SHUTDOWN )
    {
        failed(2);
    }    
    else
    {
        disable_int();
        
        /*clear the interrupt and wake up the processes*/
        if( get_prox_output(&prox_op) < 0 )
        {
            failed(3);
        }
        else
        {
            debug("   prox output: 0x%x", (0x01 & P_dev.regs[PROX])); 
        }

        P_obj_state_genev(P_dev.inputdevice, prox_op);
//	debug("[ryun] kobject_uevent(&P_misc_device->this_device->kobj, KOBJ_CHANGE);  \n");
//	struct miscdevice *P_m_device;
//	P_m_device = container_of(work, struct miscdevice, this_device);
//	kobject_uevent(&P_m_device->this_device->kobj, KOBJ_CHANGE);
		// [[ ryun
//	input_report_key(P_dev.inputdevice, BTN_0,  0x01 & P_dev.regs[PROX]);
	printk("[PROXIMITY] prox output = 0x%x  \n", (0x01 & P_dev.regs[PROX])? DETECTED : NOT_DETECTED);
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV == CONFIG_OSCAR_REV00_EMUL)
	input_report_abs(P_dev.inputdevice, ABS_DISTANCE,  (0x01 & P_dev.regs[PROX])? NOT_DETECTED : NOT_DETECTED);
#else
	input_report_abs(P_dev.inputdevice, ABS_DISTANCE,  (0x01 & P_dev.regs[PROX])? DETECTED : NOT_DETECTED);
#endif
	input_sync(P_dev.inputdevice);
		// ]] ryun
        P_waitq_wkup_proc();
    } 

    UNLOCK(); 
 
    trace_out();
}

int P_dev_get_prox_output(u16 *prox_op)
{
    int ret = 0;
   
    trace_in();
  
    LOCK();   
  
    if( P_dev.device_state == NOT_DETECTED )
    {
        failed(1);
        ret = -ENODEV; 
    }
    else if( P_dev.power_state == SHUTDOWN )
    {
        failed(2);
        ret = -1;
    }
    else if( (ret = get_prox_output(prox_op)) < 0 )
    {
        failed(3);
    }

    UNLOCK(); 
  
    trace_out();

    return ret;
}

int P_dev_powerup_set_op_mode(u16 mode)
{
    int ret = 0;
    
    trace_in();
   
    LOCK();

    if( P_dev.device_state == NOT_DETECTED )
    {
        failed(1);
        ret = -ENODEV; 
    }
    else if( (ret = set_mode(mode, P_dev.settings.detection_cyc, 
    	    P_dev.settings.sleep_func_state)) < 0 )
    {
        failed(2);
    }

    UNLOCK(); 
   
    trace_out();

    return ret;
}

/**************************************************************/

/*static functons*/
/**************************************************************/
static void enable_int(void)
{
    trace_in();
    
    if( P_dev.settings.int_status == DISABLED )
    {
        P_enable_int();
        P_dev.settings.int_status = ENABLED;
    }
    else
    {
        debug("   IMBALANCE (this is not an error)");
    }

    trace_out();
}

static void disable_int(void)
{
    trace_in();
    
    if( P_dev.settings.int_status == ENABLED )
    {
        P_disable_int();
        P_dev.settings.int_status = DISABLED;
    }  
    else
    {
        debug("   IMBALANCE (this is not an error)");
    }    

    trace_out();
}

static int vaux1_ldo_up(void)
{
    int ret = 0;
    
    trace_in();

// [[ ryun 
// add power up code.
	debug("[ryun] power up Proximity/Light chip....\n");
	pl_sensor_power_on();
#ifdef CONFIG_MACH_OSCAR
    gpio_direction_output(OMAP_GPIO_ALS_EN, 1);
#endif

/*
    if ( (ret = resource_request(P_dev.t2_vaux1_ldo, 
    	          T2_VAUX1_3V00)) < 0 )
    {
        failed(2);
        ret = -1;
    }    
    else
    {
        mdelay(1);
    }
*/
// ]] ryun
    trace_out();

    return ret;
}

static int vaux1_ldo_down(void)
{
    int ret = 0;
    
    trace_in();

#ifdef CONFIG_MACH_OSCAR
    gpio_direction_output(OMAP_GPIO_ALS_EN, 0);
#endif

// [[ ryun
// add power down code.
	debug("[ryun] power down Proximity/Light chip....\n");

	pl_sensor_power_off();
/*
    if ( (ret = resource_request(P_dev.t2_vaux1_ldo, 
    	          T2_VAUX1_OFF)) < 0 )
    {
        failed(1);
        ret = -1;
    }    
    else
    {
        mdelay(1);
    }
*/
// ]] ryun

    trace_out();

    return ret;
}

static void vaux1_ldo_map_active(void)
{
    trace_in();
// ryun
//    t2_out(PM_RECEIVER, 0x99, PHY_TO_OFF_PM_RECIEVER(0x74));
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0x99, PHY_TO_OFF_PM_RECIEVER(0x74));
    trace_out();
}

static void vaux1_ldo_map_default(void)
{
    trace_in();
// ryun
//    t2_out(PM_RECEIVER, 0x00, PHY_TO_OFF_PM_RECIEVER(0x74));
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, 0x00, PHY_TO_OFF_PM_RECIEVER(0x74));
    trace_out();
}

static int set_mode(u16 mode, u16 detection_cyc, u16 sleep_func_state)
{
    int ret = 0;
    int i;
    u16 cyc_range = P_CYC_1024ms;
    
    trace_in();

    if( P_dev.power_state == OPERATIONAL )
    {
        shutdown();
    }

    if( (ret = vaux1_ldo_up()) < 0 )
    {
        failed(12);
        goto OUT;
    }

    /*initialize all the registers to 0*/
    for( i = 0; i < NUM_OF_REGS; i++ )
    {
        P_dev.regs[i] = 0;
    }

    if( detection_cyc > cyc_range ) detection_cyc = P_CYC_8ms; /*default*/

    if( sleep_func_state != P_AN_SLEEP_ON ) 
    	   sleep_func_state = P_AN_SLEEP_OFF; /*default*/

    if( mode == P_MODE_A )
    {
        /*disable the host interrupt*/
        disable_int();

        /*The previous mode might be B and some processes 
                  might be sleeping, so before changing the mode to A 
                  we need to wake up those processes*/
        P_waitq_wkup_proc();

        P_dev.regs[GAIN] |= GAIN_LED0;

        P_dev.regs[HYS] |= (HYS_HYSC1|HYS_HYSF1|HYS_HYSD);

        P_dev.regs[CYCLE] |= ((detection_cyc << 3) & CYCLE_CYCL_MASK);
        P_dev.regs[CYCLE] |= CYCLE_OSC;

        /*disable the device VOUT*/
        P_dev.regs[CON] |= (CON_OCON1|CON_OCON0);
 
        if( sleep_func_state == P_AN_SLEEP_ON )
        {
            P_dev.regs[OPMOD] |= OPMOD_ASD;
        }
        P_dev.regs[OPMOD] |= OPMOD_SSD;

        if( (ret = i2c_write(GAIN)) < 0 )
        {
            failed(1);
        }
        else if( (ret = i2c_write(HYS)) < 0 )
        {
            failed(2);
        }        
        else if( (ret = i2c_write(CYCLE)) < 0 )
        {
            failed(3);
        }
        else if( (ret = i2c_write(CON)) < 0 )
        {
            failed(4);
        }          
        else if( (ret = i2c_write(OPMOD)) < 0 )
        {
            failed(5);
        }
        else
        {
            P_dev.settings.operation_mode = P_MODE_A;
            P_dev.settings.sleep_func_state = sleep_func_state;
            P_dev.settings.detection_cyc = detection_cyc;
            P_dev.settings.detection_dist = D_40_mm;
            P_dev.settings.detection_dist_hyst = HYST_0p;
             
            P_dev.power_state = OPERATIONAL;

            /*delay for the device to get stabilized*/
            mdelay(1);
        }
    }
    else if( mode == P_MODE_B )
    {
        P_dev.regs[GAIN] |= GAIN_LED0;

        P_dev.regs[HYS] |= HYS_HYSC1;
     
        P_dev.regs[CYCLE] |= ((detection_cyc << 3) & CYCLE_CYCL_MASK);
        P_dev.regs[CYCLE] |= CYCLE_OSC;

        P_dev.regs[CON] &= ~(CON_OCON1|CON_OCON0);
 
        if( sleep_func_state == P_AN_SLEEP_ON )
        {
            P_dev.regs[OPMOD] |= OPMOD_ASD;
        }
        P_dev.regs[OPMOD] |= (OPMOD_SSD|OPMOD_VCON);
     
        if( (ret = i2c_write(GAIN)) < 0 )
        {
            failed(6);
        }
        if( (ret = i2c_write(CON)) < 0 )
        {
            failed(7);
        }        
        else if( (ret = i2c_write(HYS)) < 0 )
        {
            failed(8);
        }        
        else if( (ret = i2c_write(CYCLE)) < 0 )
        {
            failed(9);
        }
        else if( (ret = i2c_write(OPMOD)) < 0 )
        {
            failed(10);
        }
        else
        {
            vaux1_ldo_map_active();
            
            P_dev.settings.operation_mode = P_MODE_B;
            P_dev.settings.sleep_func_state = sleep_func_state;
            P_dev.settings.detection_cyc = detection_cyc;
            P_dev.settings.detection_dist = D_50_mm;

            /*If hyst value needs to be changed, change it here*/
            P_dev.settings.detection_dist_hyst = HYST_10p;
         
            P_dev.power_state = OPERATIONAL;

            /*delay for the device to get stabilized: this is done before 
                        enabling the interrupt because it has been observed 
                        that the device sends an interrupt once it is powered
                        up in mode B, the first interrupt will be ignored*/ 
            mdelay(1);

		/* Ryunkyun.Park 20091109 for init value for proximity sensor. */
		debug("[PROXIMITY] input_report_abs(P_dev.inputdevice, ABS_DISTANCE,  NOT_DETECTED);\n");
		input_report_abs(P_dev.inputdevice, ABS_DISTANCE,  NOT_DETECTED);	// ryun 20091109 dummy value.
		input_sync(P_dev.inputdevice);

            enable_int();            
        }
    }
    else 
    {
        failed(11);
        ret = -EINVAL;
    }

OUT:
    trace_out();

    return ret;
}

static int get_prox_output(u16 *prox_op)
{
    int ret = 0;

    trace_in();
    
    if( (ret = i2c_read(PROX)) < 0 )
    {
        failed(1);
    }
    else
    {
        *prox_op = (P_dev.regs[PROX] & PROX_VO) ?
                   P_OBJ_DETECTED : P_OBJ_NOT_DETECTED;
    }
 
    if( (P_dev.settings.operation_mode == P_MODE_B) &&
        (P_dev.settings.int_status == DISABLED) )
    /*The mode is B and this read is the first read after the status change
           interrupt*/
    {
        P_dev.regs[HYS] = 0;

        if( *prox_op == P_OBJ_NOT_DETECTED )
        {           
            P_dev.regs[HYS] |= HYS_HYSC1;
        }
        else 
        {
        	if(P_dev.settings.detection_dist_hyst == HYST_10p)
        	{
        		P_dev.regs[HYS] |= (HYS_HYSC0|HYS_HYSF1|HYS_HYSF0);
        	}
        	else 
        	{
            	P_dev.regs[HYS] |= HYS_HYSC0;
        	}
        }
 
        if( (ret = i2c_write(HYS)) < 0 )
        {
            failed(2);
            goto OUT;
        }

        P_dev.regs[CON] = 0;
        P_dev.regs[CON] |= (CON_OCON1|CON_OCON0);
 
        if( (ret = i2c_write(CON)) < 0 )
        {
            failed(3);
            goto OUT;
        }       

        enable_int();
  
        P_dev.regs[CON] = 0;
        P_dev.regs[CON] &= ~(CON_OCON1|CON_OCON0);
 
        if( (ret = i2c_write(INT_CLR(CON))) < 0 )
        {
            failed(4);
            goto OUT;
        } 
    }

OUT:
    trace_out();
    
    return ret;
}

static int shutdown(void)
{
    int ret = 0;

    trace_in();
    
    if( P_dev.settings.operation_mode == P_MODE_B )
    {
        disable_int();
        P_dev.regs[OPMOD] |= OPMOD_VCON;
    }
    else if( P_dev.settings.operation_mode == P_MODE_A )
    {
        P_dev.regs[OPMOD] &= ~OPMOD_VCON;
    }
 
    P_dev.regs[OPMOD] &= ~OPMOD_SSD;
 
    if( (ret = i2c_write(OPMOD)) < 0 )
    {
        failed(1);
    }
    else
    {
        if( P_dev.settings.operation_mode == P_MODE_B )
        {
            vaux1_ldo_map_default();
        }
        mdelay(1);
        if( (ret = vaux1_ldo_down()) < 0 )
        {
            failed(2);
        }
        P_dev.power_state = SHUTDOWN;
    }

    trace_out();

    return ret;
}

static int make_operational(void)
{
    int ret = 0;

    trace_in();

    if( (ret = vaux1_ldo_up()) < 0 )
    {
        failed(6);
    }
    else if( P_dev.settings.operation_mode == P_MODE_A )
    {
        P_dev.regs[OPMOD] |= OPMOD_SSD;
        P_dev.regs[OPMOD] &= ~OPMOD_VCON;
     
        if( (ret = i2c_write(OPMOD)) < 0 )
        {
            failed(1);
        }
        else
        {
        
            P_dev.power_state = OPERATIONAL;
        }         
    }
    else if( P_dev.settings.operation_mode == P_MODE_B )
    {
        P_dev.regs[CON] = 0;
        P_dev.regs[CON] |= (CON_OCON1|CON_OCON0);
 
        if( (ret = i2c_write(CON)) < 0 )
        {
            failed(2);
            goto OUT;
        }
 
        P_dev.regs[HYS] = 0;
       
        P_dev.regs[HYS] |= HYS_HYSC1;
 
        if( (ret = i2c_write(HYS)) < 0 )
        {
            failed(3);
            goto OUT;
        }
 
        P_dev.regs[OPMOD] |= (OPMOD_SSD|OPMOD_VCON);
 
        if( (ret = i2c_write(OPMOD)) < 0 )
        {
            failed(4);
            goto OUT;
        }
 
        enable_int();
         
        P_dev.regs[CON] = 0;
        P_dev.regs[CON] &= ~(CON_OCON1|CON_OCON0);
 
        if( (ret = i2c_write(CON)) < 0 )
        {
            failed(5);
            goto OUT;
        }
        
        vaux1_ldo_map_active();
        P_dev.power_state = OPERATIONAL;
    }

    mdelay(1);

OUT:
	   trace_out();
	   
    return ret;
}

static int i2c_read(u8 reg)
{
    int ret = 0;
    struct i2c_msg msg[1];
    
    u8 aux[2] = {0.0};

    if( reg != PROX )
    {
        return -1;
    }

    msg[0].addr	= P_dev.client->addr;
    msg[0].flags = I2C_M_RD;
    msg[0].len   = 2;
    msg[0].buf = aux;

    ret = i2c_transfer(P_dev.client->adapter, msg, 1);

    if( ret < 0 )
    {
        failed(1);
        error("i2c_transfer failed %d", ret);
    }
    else if( ret >= 0 )
    {
        P_dev.regs[reg] = aux[1];
        debug("   read 0x%x: 0x%x", reg, P_dev.regs[reg]);
        ret = 0;
    }

    return ret;
}

static int i2c_write( u8 reg )
{
    int ret = 0;
    struct i2c_msg msg[1];
    u8 buf[2];

    if( ! ((NOT_INT_CLR(reg) > PROX) && (NOT_INT_CLR(reg) <= CON)) )
    {
        return -1;
    }
    
    debug("   write 0x%x: 0x%x", NOT_INT_CLR(reg), 
    	      P_dev.regs[NOT_INT_CLR(reg)]);

    msg[0].addr	= P_dev.client->addr;
    msg[0].flags = I2C_M_WR;
    msg[0].len = 2;

    buf[0] = reg;
    buf[1] = P_dev.regs[NOT_INT_CLR(reg)];
    msg[0].buf = buf;

    ret = i2c_transfer(P_dev.client->adapter, msg, 1);

    if( ret < 0 )
    {
        failed(1);
        error("i2c_transfer failed %d", ret);
    }

    if( ret >= 0 )
    {
        ret = 0;
    }

    return ret;
}
/**************************************************************/


