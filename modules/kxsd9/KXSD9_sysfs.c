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
#include <linux/input.h>

#ifdef CONFIG_MACH_OSCAR
#include <linux/sensors_core.h>
#endif

#include "KXSD9_sysfs.h"
#include "KXSD9_dev.h"
#include "common.h"

/* KXSD9 driver status */
#define ACCEL           0
#define COMPASS         1

#define STANDBY         0
#define NORMAL          1

#define STANDBY         0
#define ONLYACCEL       1
#define ONLYCOMPASS     2
#define ACCELCOMPASS    3

/*output values*/
#define KXSD9_SYSFS_POLLING_ON		0
#define KXSD9_SYSFS_POLLING_OFF		1

/*input values*/
#define KXSD9_SYSFS_START_POLLING	2
#define KXSD9_SYSFS_STOP_POLLING	3

/*error*/
#define KXSD9_SYSFS_ERROR              -1	// ryun 20091031


/*****************************************************************************/
/******************           LIGHT SENSOR SPECIFIC          *****************/
/*****************************************************************************/

/*extern functions*/
#ifndef CONFIG_MACH_OSCAR
int KXSD9_sysfs_init(struct device *);
void KXSD9_sysfs_exit(struct device *);
#else
int KXSD9_sysfs_init(struct sensors_dev *);
void KXSD9_sysfs_exit(struct sensors_dev *);
#endif

#if 0
/*static functions*/
static ssize_t 
KXSD9_name_show(struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t 
KXSD9_vendor_show(struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t 
KXSD9_type_show(struct device *dev, struct device_attribute *attr, char *buf);

static ssize_t 
KXSD9_switch_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
KXSD9_switch_store(struct device *dev, struct device_attribute *attr, 
                    const char *buf, size_t count );

static ssize_t 
KXSD9_threshold_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
KXSD9_threshold_store(struct device *dev, struct device_attribute *attr, 
                    const char *buf, size_t count );

static ssize_t 
KXSD9_interval_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t 
KXSD9_interval_store(struct device *dev, struct device_attribute *attr, 
                    const char *buf, size_t count );

/*attributes*/
static struct device_attribute attrs[6] = {
    {
        .attr.name = "name",
        .attr.mode = 0444,
        .show      = KXSD9_name_show,
        .store     = NULL,
    },
    {
        .attr.name = "vendor",
        .attr.mode = 0444,
        .show      = KXSD9_vendor_show,
        .store     = NULL,
    },
    {
        .attr.name = "type",
        .attr.mode = 0444,
        .show      = KXSD9_type_show,
        .store     = NULL,
    },
    {
        .attr.name = "switch",
        .attr.mode = 0666,
        .show      = KXSD9_switch_show,
        .store     = KXSD9_switch_store,
    },
    {
        .attr.name = "threshold",
        .attr.mode = 0666,
        .show      = KXSD9_threshold_show,
        .store     = KXSD9_threshold_store,
    },
    {
        .attr.name = "interval",
        .attr.mode = 0666,
        .show      = KXSD9_interval_show,
        .store     = KXSD9_interval_store,
    }
};
#endif

int atoi( const char* ) ;
/*****************************************************************************/
/*****************************************************************************/

#ifndef CONFIG_MACH_OSCAR
int KXSD9_sysfs_init(struct device *dev)
#else
int KXSD9_sysfs_init(struct sensors_dev *sdev)
#endif
{
    trace_in();

    #ifdef CONFIG_MACH_OSCAR
    sensors_register(NULL, sdev->dev, attrs, sizeof(attrs)/sizeof(struct device_attribute), sdev);
    #endif

    trace_out();
    return 0;
}	

#ifndef CONFIG_MACH_OSCAR
void KXSD9_sysfs_exit(struct device *dev)
#else
void KXSD9_sysfs_exit(struct sensors_dev *sdev)
#endif
{
    trace_in();

#ifdef CONFIG_MACH_OSCAR
    sensors_unregister(sdev);
#endif

    trace_out();
}	

#if 0
static ssize_t 
KXSD9_name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = 0 ;
    char* str= "gsensor" ;
 
    trace_in();

    debug("KXSD9_name: %s", buf );
    ret = snprintf(buf, PAGE_SIZE, "%s\n", str );

    trace_out();
    return ret;
}


static ssize_t 
KXSD9_vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    char* str= "samsung" ;
  
    trace_in();

    debug("KXSD9_vendor: %s", buf);
    ret = snprintf(buf, PAGE_SIZE, "%s\n", str );

    trace_out();
    return ret;
}
 

static ssize_t 
KXSD9_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    u16 type;
  
    trace_in();

    type= 0x01 ;
    debug("KXSD9_type: %d", type);
    ret = snprintf(buf, PAGE_SIZE, "%d\n", type );
 
    trace_out();
    return ret;
}
 

static ssize_t 
KXSD9_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    int status ;
    char* str_normal = "on" ;
    char* str_standby = "off" ;
  
    trace_in();
    status= KXSD9_dev_get_status() ;

    debug("KXSD9_switch: %d", status);

    if( status == STANDBY ) {
        ret = snprintf(buf, PAGE_SIZE, "%s\n", str_standby );
    } 
    else if( status == NORMAL ) {
        ret = snprintf(buf, PAGE_SIZE, "%s\n", str_normal );
    }
    else {
        printk( "%s, unknown status check it!!!\n", __FUNCTION__ ) ;
        ret= -1 ;
    }
 
    trace_out();
    return ret;
}
 
static ssize_t 
KXSD9_switch_store(struct device *dev, struct device_attribute *attr, 
                    const char *buf, size_t count )
{
    ssize_t ret = 0 ;
    int status ;
    char* operation ;

    trace_in() ;

    operation= (char*)kmalloc( count + 1, GFP_KERNEL ) ;
    status= KXSD9_dev_get_status() ;

    sscanf( buf, "%s", operation ) ;

    debug( "%s", operation ) ;

    if( status == NORMAL ) {
        ret= strncmp( operation, "off", 3 ) ;
        if( !ret ) {
            debug( "status is NORMAL, ret= %d", ret ) ;
            KXSD9_dev_set_operation_mode( ACCEL, STANDBY  ) ;
        }
        else {
            ret= strncmp( operation, "on", 2 ) ;
            if( !ret ) {
                debug( "status is NORMAL, ret= %d", ret ) ;
                printk("%s, KXSD9 running already!!!\n", __FUNCTION__ ) ;
            }
            else {
                printk("%s, Fault input statement!!! status= %d, ret= %d\n",
                __FUNCTION__, status, ret ) ;
            }
        }
    }
    else if( status == STANDBY ) 
    {
        ret= strncmp( operation, "on", 2 ) ;
        if( !ret ) {
            debug( "status is STANDBY, ret= %d", ret ) ;
            KXSD9_dev_set_operation_mode( ACCEL, NORMAL ) ;
        }
        else {
            ret= strncmp( operation, "off", 3 ) ;
            if( !ret ) {
            	debug( "status is STANDBY, ret= %d", ret ) ;
            	printk("%s, KXSD9 stopped already!!!\n", __FUNCTION__ ) ;
            }
            else {
                printk("%s, Fault input statement!!! status= %d, ret= %d\n",
                __FUNCTION__, status, ret ) ;
            }
        }
    }
    else {
        printk( "%s, !!! illigal input statement!!! status= %d\n",
        __FUNCTION__, status ) ;
    }

    kfree( operation ) ;

    trace_out() ;
    return ret ? ret : count ;
}

static ssize_t 
KXSD9_threshold_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret;
    unsigned short threshold ;
  
    trace_in();

    threshold = KXSD9_dev_get_threshold() ;

    debug("KXSD9_threshold: %d", threshold);
    ret = snprintf(buf, PAGE_SIZE, "%d\n", threshold );
 
    trace_out();
    return ret;
}
 
 
static ssize_t 
KXSD9_threshold_store(struct device *dev, struct device_attribute *attr, 
                    const char *buf, size_t count )
{
    char* str ;
    int operation ;
    trace_in() ;

    str = (char*)kmalloc( sizeof(char)*4, GFP_KERNEL ) ;

    sscanf( buf, "%s", str ) ;
    debug( "%s", str ) ;

    operation= atoi( str ) ;
    debug( "%d", operation ) ; 

    KXSD9_dev_set_threshold(operation);

    kfree( str ) ;

    trace_out() ;
    return count ; 
}


static ssize_t 
KXSD9_interval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret= 0;
    unsigned long polling_time ;
  
    trace_in();

    polling_time= KXSD9_timer_get_polling_time() ; 
    debug( "KXSD9_polling_time: %ld", polling_time );
    ret = snprintf(buf, PAGE_SIZE, "%ld\n", polling_time );
 
    trace_out();
    return ret;
}
 
static ssize_t 
KXSD9_interval_store(struct device *dev, struct device_attribute *attr, 
                    const char *buf, size_t count )
{
    char* str ;
    unsigned long polling_time ;

    trace_in() ;

    str= (char*)kmalloc( count+1, GFP_KERNEL ) ;
    sscanf( buf, "%s", str ) ;
    debug( "%s", str ) ;
    
    polling_time= atoi( str ) ;

    kfree(str);

    debug("KXSD9_polling_time: %ld", polling_time );

    KXSD9_timer_set_polling_time( polling_time ) ;
    
    trace_out() ;
    return count ; 
}
#endif

int atoi( const char* name )
{
    int value = 0;

    trace_in() ;

    for( ; *name >= '0' && *name <= '9'; name++ ) {
        switch( *name ) {
            case '0'...'9':
                value= 10 * value + ( *name - '0' ) ;
                break ;
            default:
                break ;
        }
    }
    
    debug(" %d\n", value);

    trace_out() ;
    return value ;
}
