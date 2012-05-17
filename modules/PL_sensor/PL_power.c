#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#ifdef CONFIG_MACH_OSCAR
#include <mach/oscar.h>
#endif
#include "common.h"

#define DRIVER_NAME "secPLSensorPower"


struct regulator *vaux1;
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
struct regulator *vmmc2;
#endif

int pl_sensor_power_on( void );
int pl_sensor_power_off( void );

static int __devinit pl_power_probe( struct platform_device *pdev );
static int __devexit pl_power_remove( struct platform_device *pdev );
int pl_power_init( void );
void pl_power_exit( void );

struct platform_driver pl_power_platform_driver = {
	.probe		= pl_power_probe,
	.remove		= __devexit_p( pl_power_remove ),
//	.suspend	= &pl_power_suspend,
//	.resume		= &pl_power_resume,
	.driver		= {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

int pl_sensor_power_on( void )
{
	int ret;
	
	trace_in();
	
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
	if( !vaux1 || !vmmc2 )
#else
	if( !vaux1 )
#endif
	{
		printk("PSENSOR: %s: vaux1 is NULL!\n", __func__);
		return -1;
	}
	ret = regulator_enable( vaux1 );
	if ( ret )
	{
		printk("Regulator aux1 error!!\n");
		trace_out();
		return ret;
	}

#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
	ret = regulator_enable( vmmc2 );
	if( ret )
		printk("Regulator vmmc2 error!!\n");
#endif

	trace_out();
	return ret;

}

int pl_sensor_power_off( void )
{
	int ret = 0;
	trace_in();
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV == CONFIG_OSCAR_EMU02)
	return 0;
#endif
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
        if( !vaux1 || !vmmc2 )
#else
        if( !vaux1 )
#endif
		return 0;
	
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
	ret = regulator_disable( vaux1 );
	return ret ? ret : regulator_disable( vmmc2 );
#else
	return regulator_disable( vaux1 );	
#endif
}

static int __devinit pl_power_probe( struct platform_device *pdev )
{ 
	trace_in();

	vaux1 = regulator_get( &pdev->dev, "vaux1" );
	if( IS_ERR( vaux1 ) )
	{
		printk("PSENSOR: %s: regulator_get failed to get vaux1\n", __func__);
		trace_out();
		return -1;
	}

#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
	vmmc2 = regulator_get( &pdev->dev, "vmmc2" );
	if( IS_ERR( vmmc2 ) )
	{
		printk("PSENSOR: %s: regulator_get failed to get vmmc2!\n", __func__);
		return -1;
	}
#endif
	trace_out();
	return 0;
}

static int __devexit pl_power_remove( struct platform_device *pdev )
{
	trace_in();
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
        if( !vaux1 || !vmmc2 )
#else
        if( !vaux1 )
#endif
		return 0;
	regulator_put( vaux1 );
#if defined(CONFIG_MACH_OSCAR) && (CONFIG_OSCAR_REV >= CONFIG_OSCAR_REV00)
	regulator_put( vmmc2 );
#endif

	trace_out();
	return 0;
}

int pl_power_init( void )
{
	int ret;
	
	trace_in();
	ret = platform_driver_register( &pl_power_platform_driver );
	trace_out();
	return ret;
}

void pl_power_exit( void )
{
	trace_in();
	platform_driver_unregister( &pl_power_platform_driver );
	trace_out();
}

