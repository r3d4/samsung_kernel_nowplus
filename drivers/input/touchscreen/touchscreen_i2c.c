#include <linux/module.h>
#include <linux/kernel_stat.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/random.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/slab_def.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <asm/irq.h>
#include <asm/mach/irq.h>
//#include <asm/arch/gpio.h>
//#include <asm/arch/mux.h>
#include <plat/gpio.h>	//ryun
#include <plat/mux.h>	//ryun 

//#include <asm/arch/hardware.h>
#include <mach/hardware.h>	// ryun
#include <linux/types.h>

#include "touchscreen.h"

#define I2C_M_WR 0	// ryun


static int i2c_tsp_sensor_attach_adapter(struct i2c_adapter *adapter);
static int i2c_tsp_sensor_probe_client(struct i2c_adapter *adapter, int address, int kind);
static int i2c_tsp_sensor_detach_client(struct i2c_client *client);

//#define TSP_SENSOR_ADDRESS		0x4b	// onegun test

#define I2C_DF_NOTIFY				0x01

struct i2c_driver tsp_sensor_driver =
{
	//.name	= "tsp_sensor_driver",
	.driver	= {
		.name	= "tsp_driver",
		.owner	= THIS_MODULE,
	},
	//.flags	= I2C_DF_NOTIFY | I2C_M_IGNORE_NAK,
	.attach_adapter	= &i2c_tsp_sensor_attach_adapter,
	//.detach_client	= &i2c_tsp_sensor_detach_client,
	.remove	= &i2c_tsp_sensor_detach_client,
};


static struct i2c_client *g_client;

int i2c_tsp_sensor_read(u8 reg, u8 *val, unsigned int len )
{
	int id;
	int 	 err;
	struct 	 i2c_msg msg[1];
	unsigned char data[1];

	if( (g_client == NULL) || (!g_client->adapter) )
	{
		return -ENODEV;
	}
	
	msg->addr 	= g_client->addr;
	msg->flags 	= I2C_M_WR;
	msg->len 	= 1;
	msg->buf 	= data;
	*data       = reg;

	err = i2c_transfer(g_client->adapter, msg, 1);

	if (err >= 0) 
	{
		msg->flags = I2C_M_RD;
		msg->len   = len;
		msg->buf   = val;
		err = i2c_transfer(g_client->adapter, msg, 1);
	}

	if (err >= 0) 
	{
		return 0;
	}

	id = i2c_adapter_id(g_client->adapter);

	return err;
}

int i2c_tsp_sensor_write(u8 reg, u8 *val, unsigned int len)
{
	int err;
	struct i2c_msg msg[1];
	unsigned char data[4];
	int i ;

	if(len > 3)
	{
		printk("[TSP][ERROR] %s() : excess of length(%d) !! \n", __FUNCTION__, len);
		return -1;
	}
	
	if( (g_client == NULL) || (!g_client->adapter) )
		return -ENODEV;

	msg->addr = g_client->addr;
	msg->flags = I2C_M_WR;
	msg->len = len + 1;
	msg->buf = data;
	data[0] = reg;

	for (i = 0; i < len; i++)
	{
		data[i+1] = *(val+i);
	}

	err = i2c_transfer(g_client->adapter, msg, 1);

	if (err >= 0) return 0;

	return err;
}

int i2c_tsp_sensor_write_reg(u8 address, int data)
{
	u8 i2cdata[1];

	i2cdata[0] = data;
	return i2c_tsp_sensor_write(address, i2cdata, 1);
}

static int i2c_tsp_sensor_attach_adapter(struct i2c_adapter *adapter)
{
	int addr = 0;
	int id = 0;

	addr = 0x2C;
	//return i2c_probe(adapter, &addr_data, &i2c_tsp_sensor_probe_client);

	id = adapter->nr;

#if 1//defined(CONFIG_MACH_NOWPLUS) || defined(CONFIG_MACH_NOWPLUS_MASS)
	if (id == 3)
#else
#if (CONFIG_ACME_REV >= CONFIG_ACME_REV04)
	if (id == 3)
#else
	if (id == 2)
#endif
#endif
		return i2c_tsp_sensor_probe_client(adapter, addr, 0);
	return 0;
}

static int i2c_tsp_sensor_probe_client(struct i2c_adapter *adapter, int address, int kind)
{
	struct i2c_client *new_client;
	int err = 0;

	if ( !i2c_check_functionality(adapter,I2C_FUNC_SMBUS_BYTE_DATA) ) {
		printk("byte op is not permited.\n");
		goto ERROR0;
	}

	new_client = kzalloc(sizeof(struct i2c_client), GFP_KERNEL );

	if ( !new_client )	{
		err = -ENOMEM;
		goto ERROR0;
	}

	new_client->addr = address; 
	printk("%s :: addr=%x\n", __FUNCTION__, address);
	new_client->adapter = adapter;
	new_client->driver = &tsp_sensor_driver;
	//new_client->flags = I2C_DF_NOTIFY | I2C_M_IGNORE_NAK;

	g_client = new_client;

	strlcpy(new_client->name, "tsp_sensor", I2C_NAME_SIZE);

//	if ((err = i2c_attach_client(new_client)))	// ryun 20091126 move to board-xxx.c
//		goto ERROR1;
	printk("[TSP] %s() - end : success\n", __FUNCTION__);	// ryun 20091126
	return 0;

	ERROR1:
		kfree(new_client);
	ERROR0:
		return err;
}

static int i2c_tsp_sensor_detach_client(struct i2c_client *client)
{
	int err;

  	/* Try to detach the client from i2c space */
	//if ((err = i2c_detach_client(client))) {
    //    return err;
	//}
	i2c_set_clientdata(client,NULL);

	kfree(client); /* Frees client data too, if allocated at the same time */
	g_client = NULL;
	return 0;
}


int i2c_tsp_sensor_init(void)
{
	int ret;

	if ( (ret = i2c_add_driver(&tsp_sensor_driver)) ) 
	{
		printk("TSP I2C Driver registration failed!\n");
		return ret;
	}

	return 0;
}

