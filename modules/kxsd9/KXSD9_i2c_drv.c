
#include <linux/kernel.h>
#include <linux/i2c.h>

#include "KXSD9_dev.h"
#include "common.h"
#include "KXSD9_main.h"

/*extern functions*/
int KXSD9_i2c_drv_init(void);
void KXSD9_i2c_drv_exit(void);

/*static functions*/
static int KXSD9_probe (struct i2c_client *, const struct i2c_device_id*);
static int KXSD9_remove(struct i2c_client *);
static void KXSD9_shutdown(struct i2c_client *);
//static int KXSD9_suspend(struct i2c_client *, pm_message_t mesg);
//static int KXSD9_resume(struct i2c_client *);

static const struct i2c_device_id KXSD9_i2c_id[] = {
	{ "kxsd9", 0 },
	{},
};

static struct i2c_driver KXSD9_i2c_driver =
{
    .driver = {
        .name = "kxsd9",
        .owner= THIS_MODULE,
    },
	
    .probe = KXSD9_probe,
    .remove = KXSD9_remove,
    .shutdown = KXSD9_shutdown,
#ifndef CONFIG_HAS_EARLYSUSPEND
    .suspend = KXSD9_suspend,
    .resume = KXSD9_resume,
#endif // CONFIG_HAD_EARLYSUSPEND
    .id_table   = KXSD9_i2c_id,
};

static int KXSD9_probe (struct i2c_client *client, const struct i2c_device_id* id )
{
    int ret = 0;
    trace_in() ;

    if( strcmp(client->name, "kxsd9" ) != 0 )
    {
        ret = -1;
        printk("%s: device not supported", __FUNCTION__ );
        return ret ;
    }
    else if( (ret = KXSD9_dev_init(client, KXSD9_main_getparam())) < 0 )
    {
        printk("%s: KXSD9_dev_init failed", __FUNCTION__ );
    }

    trace_out() ;
    return ret;
}

static int KXSD9_remove(struct i2c_client *client)
{
    int ret = 0;
    trace_in() ;

    if( strcmp(client->name, "kxsd9" ) != 0 )
    {
        ret = -1;
        printk("%s: KXSD9_remove: device not supported", __FUNCTION__ );
    }
    else if( (ret = KXSD9_dev_exit()) < 0 )
    {
        printk("%s: KXSD9_dev_exit failed", __FUNCTION__ );
    }
  
    trace_out() ; 
    return ret;
}

static void KXSD9_shutdown(struct i2c_client *client)
{
    trace_in() ;

    if( strcmp(client->name, "kxsd9" ) != 0 )
    {
        printk("%s: KXSD9_shutdown: device not supported", __FUNCTION__ );
    }
    else if( KXSD9_dev_disable() < 0 )
    {
        printk( "%s: KXSD9_dev_disable failed", __FUNCTION__ );
    }

    if( KXSD9_timer_disable() != TIMER_OFF ) 
    {
        printk( "%s: KXSD9_timer_ON, this is fail", __FUNCTION__ ) ;
    }

    trace_out() ; 
}

#if 0
static int KXSD9_suspend(struct i2c_client *client, pm_message_t mesg)
{
    int ret = 0;
    trace_in() ;
	   
    if( strcmp(client->name, "kxsd9" ) != 0 )
    {
        ret = -1;
        printk("%s: KXSD9_suspend: device not supported", __FUNCTION__ );
    }
    else if( (ret = KXSD9_dev_disable()) < 0 )
    {
        printk("%s: KXSD9_dev_disable failed", __FUNCTION__ );
    }

    if( (ret= KXSD9_timer_disable()) != TIMER_OFF ) 
    {
        printk( "%s: KXSD9_timer_ON, this is fail", __FUNCTION__ ) ;
        trace_out() ; 
        return ret ;
    }
    
    trace_out() ; 
    return ret;
}

static int KXSD9_resume(struct i2c_client *client)
{
    int ret = 0;
    trace_in() ;
	   
    if( strcmp(client->name, "kxsd9" ) != 0 )
    {
        ret = -1;
        printk("%s: KXSD9_resume: device not supported", __FUNCTION__ );
    }
    else if( (ret = KXSD9_dev_enable()) < 0 )
    {
        printk("%s: KXSD9_dev_enable failed", __FUNCTION__ );
    }

    if( KXSD9_timer.saved_timer_state == TIMER_ON ) 
    {
        if( (ret= KXSD9_timer_enable( POLLING_INTERVAL )) != TIMER_ON ) 
        {
            printk( "%s: KXSD9_timer_OFF, this is fail", __FUNCTION__ ) ;
            trace_out() ;
            return ret ;
        }
    }
 
    trace_out() ; 
    return ret;
}
#endif

int KXSD9_i2c_drv_init(void)
{	
    int ret = 0;
    trace_in() ;

    if ( (ret = i2c_add_driver(&KXSD9_i2c_driver) < 0) ) 
    {
        printk("%s: KXSD9 i2c_add_driver failed", __FUNCTION__ );
    }

    trace_out() ; 
    return ret;
}

void KXSD9_i2c_drv_exit(void)
{
    trace_in() ;

    i2c_del_driver(&KXSD9_i2c_driver);

    trace_out() ; 
}


