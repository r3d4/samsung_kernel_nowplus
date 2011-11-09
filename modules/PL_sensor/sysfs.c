/*
 * sysfs.c 
 *
 * Description: This file implements the sysfs interface
 *
 * Author: Varun Mahajan <m.varun@samsung.com>
 */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
// ryun 20091012
#include <linux/input.h>

#include "sysfs.h"
#include "P_dev.h"
#include "L_dev.h"
#include "common.h"

#include <linux/autoconf.h>

#ifdef CONFIG_MACH_OSCAR	// ryun 20091212 for OSCAR
#include "linux/fs.h"	// ryun 20091203 
#include <linux/sensors_core.h>		// ryun 20091208 for OSCAR
#endif	// ryun 20091212 for OSCAR

/*****************************************************************************/
/****************           PROXIMITY SENSOR SPECIFIC          ***************/
/*****************************************************************************/
/*attr: P_operation*/
/*output values*/
#define P_SYSFS_DEV_SHUTDOWN        0
#define P_SYSFS_MODE_A_OPERATIONAL  1
#define P_SYSFS_MODE_B_OPERATIONAL  2
/*input values*/
#define P_SYSFS_PWRUP_MODE_A        3
#define P_SYSFS_PWRUP_MODE_B        4
#define P_SYSFS_SHUTDOWN            5
#define P_SYSFS_RETURN_TO_OP        6
#define P_SYSFS_RESET               7

/*attr: P_output*/
#define P_SYSFS_OBJ_NOT_DETECTED    0
#define P_SYSFS_OBJ_DETECTED        1

//[[  ryun 20091031
/*attr: L_operation*/ 
/*output values*/
#define L_SYSFS_POLLING_ON		0
#define L_SYSFS_POLLING_OFF		1

/*input values*/
#define L_SYSFS_START_POLLING	2
#define L_SYSFS_STOP_POLLING	3
//]]  ryun 20091031

/*error*/
#define P_SYSFS_ERROR              -1
#define L_SYSFS_ERROR              -1	// ryun 20091031


/*extern functions*/
#ifndef CONFIG_MACH_OSCAR
int P_sysfs_init(struct device *);
void P_sysfs_exit(struct device *);
#else
int P_sysfs_init(struct sensors_dev *);
void P_sysfs_exit(struct sensors_dev *);
#endif
void P_obj_state_genev(struct input_dev *, u16);

/*static functions*/
static ssize_t 
P_operation_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
P_operation_store(struct device *dev, struct device_attribute *attr, 
	                    const char *buf, size_t count);

#ifdef CONFIG_MACH_OSCAR	// ryun 20091212 for OSCAR
/*static functions for OSCAR*/
static ssize_t P_name_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t P_switch_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t P_switch_store(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t P_threshold_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t P_threshold_store(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t P_interval_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t P_interval_store(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t P_vendor_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t P_type_show(struct device *dev, struct device_attribute *attr, char *buf);
#endif	// ryun 20091212 for OSCAR
static ssize_t P_output_show(struct device *dev, struct device_attribute *attr, char *buf);

/*attributes*/
static DEVICE_ATTR(P_operation, S_IRUGO | S_IWUSR, 
                        P_operation_show, P_operation_store);

static DEVICE_ATTR(P_output, S_IRUGO, P_output_show, NULL);

static struct class *P_obj_state;
/*****************************************************************************/
/*****************************************************************************/


/*****************************************************************************/
/******************           LIGHT SENSOR SPECIFIC          *****************/
/*****************************************************************************/
#define L_SYSFS_ERROR               -1

/*extern functions*/
#ifndef CONFIG_MACH_OSCAR
int L_sysfs_init(struct device *);
void L_sysfs_exit(struct device *);
#else
int L_sysfs_init(struct sensors_dev *);
void L_sysfs_exit(struct sensors_dev *);
#endif

/*static functions*/
static ssize_t 
L_adc_val_show(struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t 
L_illum_lvl_show(struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t 
L_op_show(struct device *dev, struct device_attribute *attr, char *buf, size_t size);

static ssize_t 
L_op_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size);

#ifdef CONFIG_MACH_OSCAR	// ryun 20091212 for OSCAR
/*static functions for OSCAR*/
static ssize_t 
L_name_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
L_switch_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
L_switch_store(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
L_threshold_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
L_threshold_store(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
L_interval_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
L_interval_store(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
L_vendor_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
L_type_show(struct device *dev, struct device_attribute *attr, char *buf);
#endif	// ryun 20091212 for OSCAR

/*attributes*/
static DEVICE_ATTR(L_adc_val, S_IRUGO, L_adc_val_show, NULL);

static DEVICE_ATTR(L_illum_lvl, S_IRUGO, L_illum_lvl_show, NULL);
static DEVICE_ATTR(L_op, S_IRUGO | S_IWUGO | S_IRUSR | S_IWUSR, L_op_show, L_op_store);

/*****************************************************************************/
/*****************************************************************************/
#ifndef CONFIG_MACH_OSCAR
int P_sysfs_init(struct device *dev)
#else
int P_sysfs_init(struct sensors_dev *sdev)
#endif
{
    int ret = 0;

    trace_in();


#ifdef CONFIG_MACH_OSCAR	// ryun 20091212 for OSCAR
	static struct device_attribute sd[6];
	sd[0].attr.name = "name";
	sd[0].attr.mode = 0444 ;
	sd[0].show = P_name_show;
	sd[0].store = NULL;
	sd[1].attr.name = "switch";
	sd[1].attr.mode = 0666;
	sd[1].show = P_switch_show ;
	sd[1].store = P_switch_store;
	sd[2].attr.name = "threshold";
	sd[2].attr.mode = 0666 ;
	sd[2].show = P_threshold_show;
	sd[2].store = P_threshold_store;;
	sd[3].attr.name = "interval";
	sd[3].attr.mode = 0666 ;
	sd[3].show = P_interval_show;
	sd[3].store = P_interval_store;  
	sd[4].attr.name = "vendor";
	sd[4].attr.mode = 0444 ;
	sd[4].show = P_vendor_show;
	sd[4].store = NULL;
	sd[5].attr.name = "type";
	sd[5].attr.mode = 0444 ;
	sd[5].show = P_type_show;
	sd[5].store = NULL;
	
	sensors_register(NULL, sdev->dev, sd, 6, sdev);
#else	// ryun 20091212 for OSCAR
	
    if( (ret = device_create_file( dev, &dev_attr_P_operation )) < 0 )
    {
        failed(1);
    }
    else if( (ret = device_create_file( dev, &dev_attr_P_output )) < 0 )
    {
        device_remove_file( dev, &dev_attr_P_operation );
        failed(2);
    }
    else if( IS_ERR(P_obj_state = class_create( THIS_MODULE, "P_obj_state" )) )
    {
        device_remove_file( dev, &dev_attr_P_output );
        device_remove_file( dev, &dev_attr_P_operation );
        ret = -1;
        failed(3);
    } 
#endif
    trace_out();

    return ret;
}

#ifndef CONFIG_MACH_OSCAR
void P_sysfs_exit(struct device *dev)
#else
void P_sysfs_exit(struct sensors_dev *sdev)
#endif
{
    trace_in();

#ifdef CONFIG_MACH_OSCAR
    sensors_unregister(sdev);
#else
    class_destroy(P_obj_state);
    device_remove_file( dev, &dev_attr_P_output );
    device_remove_file( dev, &dev_attr_P_operation );
#endif
    trace_out();
}

#ifndef CONFIG_MACH_OSCAR
int L_sysfs_init(struct device *dev)
#else
int L_sysfs_init(struct sensors_dev *sdev)
#endif
{
    int ret = 0;

    trace_in();

#ifdef CONFIG_MACH_OSCAR	// ryun 20091212 for OSCAR
	static struct device_attribute sd[6];
        struct device *dev = NULL;
	sd[0].attr.name = "name";
	sd[0].attr.mode =  0444;
	sd[0].show = L_name_show;
	sd[0].store = NULL;
	sd[1].attr.name = "switch";
	sd[1].attr.mode = 0666;
	sd[1].show = L_switch_show;
	sd[1].store = L_switch_store;
	sd[2].attr.name = "threshold";
	sd[2].attr.mode = 0666 ;
	sd[2].show = L_threshold_show;
	sd[2].store = L_threshold_store;
	sd[3].attr.name = "interval";
	sd[3].attr.mode = 0666;
	sd[3].show = L_interval_show;
	sd[3].store = L_interval_store;
	sd[4].attr.name = "vendor";
	sd[4].attr.mode = 0444 ;
	sd[4].show = L_vendor_show;
	sd[4].store = NULL;
	sd[5].attr.name = "type";
	sd[5].attr.mode = 0444 ;
	sd[5].show = L_type_show;
	sd[5].store = NULL;
		
	sensors_register(dev, sdev->dev, sd, 6, sdev);
        dev = sdev->dev;
#endif	// ryun 20091212 for OSCAR

    if( (ret = device_create_file( dev, &dev_attr_L_adc_val )) < 0 )
    {
        failed(1);
    }
    else if( (ret = device_create_file( dev, &dev_attr_L_illum_lvl )) < 0 )
    {
        device_remove_file( dev, &dev_attr_L_adc_val );
        failed(2);
    }
    else if( (ret = device_create_file( dev, &dev_attr_L_op )) < 0 )
    {
        device_remove_file( dev, &dev_attr_L_adc_val );
        device_remove_file( dev, &dev_attr_L_illum_lvl );
        failed(3);
    }

    trace_out();

    return ret;
}	

#ifndef CONFIG_MACH_OSCAR
void L_sysfs_exit(struct device *dev)
#else
void L_sysfs_exit(struct sensors_dev *sdev)
#endif
{
    trace_in();

#ifdef CONFIG_MACH_OSCAR
    struct device *dev = sdev->dev;
    sensors_unregister(sdev);
#endif
    device_remove_file( dev, &dev_attr_L_adc_val );
    device_remove_file( dev, &dev_attr_L_illum_lvl );

    trace_out();
}	

void P_obj_state_genev(struct input_dev *inputdevice, u16 obj_state)
{
    trace_in();
    
    if( obj_state == P_OBJ_DETECTED )
    {
    debug("[ryun] kobject_uevent(&inputdevice->dev.kobj, KOBJ_ADD);");
//        kobject_uevent(&inputdevice->dev.kobj, KOBJ_ADD);
//        kobject_uevent(&P_obj_state->dev_uevent, KOBJ_ADD);
    }
    else if( obj_state == P_OBJ_NOT_DETECTED )
    {
    debug("[ryun] kobject_uevent(&inputdevice->dev.kobj, KOBJ_REMOVE);");
//	    kobject_uevent(&inputdevice->dev.kobj, KOBJ_REMOVE);
//    kobject_uevent(&P_obj_state->dev_uevent, KOBJ_REMOVE);
//ryun        kobject_uevent(&P_obj_state->subsys.kobj, KOBJ_REMOVE);
    }

    trace_out();
}

static ssize_t 
P_operation_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    int P_operation = P_SYSFS_ERROR;
    u16 power_state, mode;
   
    trace_in();

    if( P_dev_get_pwrstate_mode(&power_state, &mode) < 0 )
    {
        P_operation = P_SYSFS_ERROR;
        failed(1);
    }
    else
    {
        if( power_state == P_SHUTDOWN )
        {
            P_operation = P_SYSFS_DEV_SHUTDOWN;
        }
        else if ( (mode == P_MODE_A) && (power_state == P_OPERATIONAL) )
        {
            P_operation = P_SYSFS_MODE_A_OPERATIONAL;
        }
        else if ( (mode == P_MODE_B) && (power_state == P_OPERATIONAL) )
        {
            P_operation = P_SYSFS_MODE_B_OPERATIONAL;
        }        
    }
    
    debug("   P_operation: %d", P_operation);

    ret = snprintf(buf, PAGE_SIZE, "%d\n",P_operation);

    trace_out();

    return ret;
}

static ssize_t 
P_operation_store(struct device *dev, struct device_attribute *attr, 
	                    const char *buf, size_t count)
{
    ssize_t ret;
    int P_operation;
    
    trace_in();

    sscanf(buf, "%d", &P_operation);
    ret = strnlen(buf, sizeof(P_operation));

    debug("   P_operation: %d", P_operation);

    switch (P_operation)
    {
        case P_SYSFS_PWRUP_MODE_A:
        	
        	   if( P_dev_powerup_set_op_mode(P_MODE_A) < 0 )
        	   {
        	       failed(1);
        	   }

            break;	   

        case P_SYSFS_PWRUP_MODE_B:

        	   if( P_dev_powerup_set_op_mode(P_MODE_B) < 0 )
        	   {
        	       failed(2);
        	   }

            break;

        case P_SYSFS_SHUTDOWN:

        	   if( P_dev_shutdown() < 0 )
        	   {
        	       failed(3);
        	   }

            break;

        case P_SYSFS_RETURN_TO_OP:

        	   if( P_dev_return_to_op() < 0 )
        	   {
        	       failed(4);
        	   }

            break;
            
        case P_SYSFS_RESET:

            if( P_dev_reset() < 0 )
        	   {
        	       failed(5);
        	   }
            
            break;
        	
    }

    trace_out();

    return ret;
}

static ssize_t 
P_output_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    int P_output = P_SYSFS_ERROR;
    u16 prox_op;

    trace_in();

    if( P_dev_get_prox_output(&prox_op) < 0 )
    {
        P_output = P_SYSFS_ERROR;
        failed(1);
    }
    else
    {
         if( prox_op == P_OBJ_DETECTED )
         {
             P_output = P_SYSFS_OBJ_DETECTED;
         }
         else if( prox_op == P_OBJ_NOT_DETECTED )
         {
             P_output = P_SYSFS_OBJ_NOT_DETECTED;
         }
    }

    debug("   P_output: %d", P_output);
    
    ret = snprintf(buf, PAGE_SIZE, "%d\n",P_output);

    trace_out();

    return ret;
}

static ssize_t 
L_adc_val_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    int L_adc_val = L_SYSFS_ERROR;
    u32 adc_val;
 
    trace_in();

    if( L_dev_get_adc_val(&adc_val) < 0 )
    {
        L_adc_val = L_SYSFS_ERROR;
        failed(1);
    }
    else
    {
        L_adc_val = adc_val;
    }                
  
    debug("   L_adc_val: %d", L_adc_val);
     
    ret = snprintf(buf, PAGE_SIZE, "%d\n",L_adc_val);
 
    trace_out();
 
    return ret;
}


static ssize_t 
L_illum_lvl_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    int L_illum_lvl = L_SYSFS_ERROR;
    u16 illum_lvl;
  
    trace_in();
 
    if( L_dev_get_illum_lvl(&illum_lvl) < 0 )
    {
        L_illum_lvl = L_SYSFS_ERROR;
        failed(1);
    }
    else
    {
        L_illum_lvl = illum_lvl;
    }                
  
    debug("   L_illum_lvl: %d", L_illum_lvl);
     
    ret = snprintf(buf, PAGE_SIZE, "%d\n",L_illum_lvl);
 
    trace_out();
 
    return ret;
}

static ssize_t
L_op_show(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	u16 op_state;

	op_state = L_dev_get_op_state();;

	return snprintf(buf, PAGE_SIZE, "%hu\n", op_state);
}

static ssize_t
L_op_store(struct device *dev, struct device_attribute *attr, char *buf, size_t size)
{
	unsigned short value;

	if (sscanf(buf, "%hu", &value) != 1)
		return -EINVAL;

	L_dev_set_op_state(value);

	return size;
}

#ifdef CONFIG_MACH_OSCAR	// ryun 20091212 for OSCAR
/*static functions for OSCAR*/
static ssize_t 
P_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();

	debug("   sensor name: psensor");
	ret = snprintf(buf, PAGE_SIZE, "psensor\n");

	trace_out();
	return ret;
}
static ssize_t 
P_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    char *P_operation = "off";
    u16 power_state, mode;
   
    trace_in();

    if( P_dev_get_pwrstate_mode(&power_state, &mode) < 0 )
    {
        P_operation = "off";
        failed(1);
    }
    else
    {
        if( power_state == P_SHUTDOWN )
        {
            P_operation = "off";
        }
        else if ( (mode == P_MODE_A) && (power_state == P_OPERATIONAL) )
        {
            P_operation = "on";
        }
        else if ( (mode == P_MODE_B) && (power_state == P_OPERATIONAL) )
        {
            P_operation = "on";
        }        
    }
    
    debug("   P_operation: %s", P_operation);

    ret = snprintf(buf, PAGE_SIZE, "%s\n",P_operation);

    trace_out();

    return ret;
}
static ssize_t 
P_switch_store(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	char *P_operation = buf;
    
	trace_in();
	
	if (strncmp(P_operation, "off", 3) == 0 ) 
		{
			ret = 3;
			printk("%s", P_operation);
			if( P_dev_shutdown() < 0 )
			{
				printk("P_dev_powerup_set_op_mode() : fail!! \n");
				ret = -1;
			}
		}
	else if(strncmp(P_operation, "on", 2) == 0 ) 
		{
			ret = 2;
			printk("%s", P_operation);
			if( P_dev_powerup_set_op_mode(P_MODE_B) < 0 )
			{
				printk("P_dev_powerup_set_op_mode() : fail!! \n");
				ret = -1;
			}
		}
	trace_out();
	return ret;
}
static ssize_t 
P_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();

	debug("   P_threshold_show()");
	ret = snprintf(buf, PAGE_SIZE, "0\n");

	trace_out();
	return ret;
}

static ssize_t P_threshold_store(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = strlen(buf);
 	trace_in();

	return ret;
}

static ssize_t 
P_interval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();

	debug("   P_interval_show()");
	ret = snprintf(buf, PAGE_SIZE, "0\n");

	trace_out();
	return ret;
}

static ssize_t P_interval_store(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret = strlen(buf);
	trace_in();

	return ret;
}

static ssize_t 
P_vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();

	debug("   P_vendor_show : samsung");
	ret = snprintf(buf, PAGE_SIZE, "samsung\n");

	trace_out();
	return ret;
}
static ssize_t 
P_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();

	debug("   P_type_show : 8");
	ret = snprintf(buf, PAGE_SIZE, "8\n");

	trace_out();
	return ret;
}
static ssize_t 
L_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();

	debug("   sensor name: lsensor");
	ret = snprintf(buf, PAGE_SIZE, "lsensor\n");

	trace_out();
	return ret;
}
static ssize_t 
L_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    char *L_operation = "off";
    int power_state;
   
    trace_in();
	power_state = L_dev_get_polling_state();
	if( power_state == L_SYSFS_POLLING_OFF)
	{
		L_operation = "off";
	}
    	else if(power_state == L_SYSFS_POLLING_ON)
	{
		L_operation = "on";
	}        
        
    debug("   L_operation: %s", L_operation);

    ret = snprintf(buf, PAGE_SIZE, "%s\n",L_operation);

    trace_out();
    return ret;
}
static ssize_t 
L_switch_store(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t ret;
	char *L_operation = buf;
    
	trace_in();
	
	if (strncmp(L_operation, "off", 3) == 0 ) 
		{
			printk("%s", L_operation);
			if( L_dev_polling_stop() != 1 )
			{
				printk("L_dev_polling_stop() : fail!! \n");
				ret = -1;
			}
		}
	else if(strncmp(L_operation, "on", 2) == 0 ) 
		{
			printk("%s", L_operation);
			if( L_dev_polling_start() != 0 )
			{
				printk("L_dev_polling_start() : fail!! \n");
				ret = -1;
			}
		}
	trace_out();
	return ret;
}
static ssize_t 
L_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();

	debug("   L_threshold_show()");
	ret = snprintf(buf, PAGE_SIZE, "0");

	trace_out();
	return ret;
}

static ssize_t
L_threshold_store(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = strlen(buf);
    trace_in();

    return ret;
}

static ssize_t 
L_interval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();
	unsigned long interval = L_dev_get_polling_interval();

	debug("   sensor L_interval_show() : %lu", interval );
	ret = snprintf(buf, PAGE_SIZE, "%lu\n", interval);

	trace_out();
	return ret;
}
static ssize_t 
L_interval_store(struct device *dev, struct device_attribute *attr, char *buf)
{

    ssize_t ret;
    unsigned long L_operation;
    
    trace_in();

    sscanf(buf, "%lu", &L_operation);
    ret = strnlen(buf, sizeof(L_operation));
	L_dev_set_polling_interval(L_operation);
	debug("   sensor L_interval_store() : %lu sec", L_operation );
    trace_out();

    return ret;

}
static ssize_t 
L_vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();

	debug("   L_vendor_show : samsung");
	ret = snprintf(buf, PAGE_SIZE, "samsung\n");

	trace_out();
	return ret;
}
static ssize_t 
L_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	size_t ret;
	trace_in();

	debug("   L_type_show : 5");
	ret = snprintf(buf, PAGE_SIZE, "5\n");

	trace_out();
	return ret;
}
#endif	// ryun 20091212 for OSCAR
