/*
 * Synaptics RMI4 touchscreen driver
 *
 * Copyright (C) 2010
 * Author: Joerie de Gram <j.de.gram@gmail.com>
 * Modified for i8320 replaced ts by <millay2630@yahoo.com>
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/regulator/consumer.h>
#include <linux/input/synaptics_i2c_rmi4.h>
#include <linux/slab.h>

#define DRIVER_NAME "synaptics_rmi4_i2c"
//#define GPIO_IRQ	142

#define ABS_X_MAX	480
#define ABS_Y_MAX	800
#define TOTAL_REG	27

//#define TP_USE_WORKQUEUE
extern int nowplus_enable_touch_pins(int enable);//r3d4 added in nowplus_board_config, enable pins here

static void synaptics_rmi4_early_suspend(struct early_suspend *handler);
static void synaptics_rmi4_late_resume(struct early_suspend *handler);

static struct workqueue_struct *synaptics_wq;

struct synaptics_rmi4_data {
	struct input_dev *input;
	struct i2c_client *client;
	struct hrtimer timer;
#ifdef TP_USE_WORKQUEUE
	struct work_struct work;
#endif	
	unsigned int irq;
	unsigned char f01_control_base;
	unsigned char f01_data_base;
	unsigned char f11_data_base;
	bool			touch_stopped;

	struct regulator	*regulator;
	struct early_suspend	early_suspend;
};

#ifdef TP_USE_WORKQUEUE
static enum hrtimer_restart synaptics_rmi4_timer_func(struct hrtimer *timer)
{
	printk("%s() called\n",__FUNCTION__);

	struct synaptics_rmi4_data *ts = container_of(timer, struct synaptics_rmi4_data, timer);

	queue_work(synaptics_wq, &ts->work);

	hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}
static void synaptics_rmi4_work(struct work_struct *work)
{	

	struct synaptics_rmi4_data *ts = dev_id;

	/*register status*/
	int i;
	s32 finger_pressure,finger_contact;
	int x,y,xx,yy;
	unsigned char val[TOTAL_REG];
	//i2c_smbus_read_i2c_block_data(ts->client,0,TOTAL_REG,val); /*一次读27个数，我认为可能太占时间*/
	//printk("tp\n");
	val[1] = i2c_smbus_read_byte_data(ts->client,1); /*读了这个后可以读到19前的数据*/
	//val[2] = i2c_smbus_read_byte_data(ts->client,2); 
	//val[3] = i2c_smbus_read_byte_data(ts->client,3); 
	//val[4] = i2c_smbus_read_byte_data(ts->client,4); 
	val[12] = i2c_smbus_read_byte_data(ts->client,12);
	val[13] = i2c_smbus_read_byte_data(ts->client,13);/*这个数低四位一直变化，没有找到规律*/
	val[19] = i2c_smbus_read_byte_data(ts->client,19);/*读了这个后可以读到19后面的数据*/
	val[22] = i2c_smbus_read_byte_data(ts->client,22);/*xx*/
	val[23] = i2c_smbus_read_byte_data(ts->client,23);/*yy*/
	val[24] = i2c_smbus_read_byte_data(ts->client,24);/*xy*/
	val[26] = i2c_smbus_read_byte_data(ts->client,26);/*这个值只有0和0x7f两个值，暂时用这个值做为size*/
	
	
	/*for (i = 0;i < TOTAL_REG; i++)
	{
		printk("val[%d] = %d = 0x%x\n",i,val[i],val[i]);
	}*/
	 
	//xx = val[2] << 8 | val[3];/*这个值再放手时为0其它时后等于x*/
	//yy = val[4] << 8 | val[5];/*这个值再放手时为0其它时后等于y*/

	finger_pressure = val[26]>>4;//val[13]&0xf;
	finger_contact = (val[12]<<4) | ((val[13] >> 4) & 0xf);

	x = val[22] << 4 | (val[24] & 0x0f);//x
	y = val[23] << 4 | ((val[24] & 0xf0) >> 4);//y
	
	if (x > 479)
		x = 479;
	if (y > 799)
		y = 799;
	if (x < 1)
		x = 1;
	if (y < 1)
		y = 1;


	//if (finger_contact > 30)
	//	finger_contact = 30;
	//if (val[26] == 0) 
	//	finger_pressure = finger_contact = 0;
	//else if (finger_pressure < 2)
	//	finger_pressure = 2;
	//if (finger_pressure > 255)
	//	finger_pressure = 30;
	/*
	if ((val[26] == 0) && (finger_contact != 0))
	{
		printk("contact val error\n");
		finger_pressure = finger_contact = 0;
	}*/
	//printk("X= %d Y= %d ,finger_contact = %d finger_pressure = %d ,val[12] = 0x%x,val[13] = 0x%x\n",x,y,finger_contact,finger_pressure,val[12],val[13] );
	//printk("finger_contact = %d finger_pressure = %d\n",finger_contact,finger_pressure);
	//input_report_abs(ts->input, ABS_MT_WIDTH_MINOR,finger_contact_minor);//pressure finger_pressure
	//printk("X= %d Y= %d val[12] = %x val[13] = %x\n",x,y,val[12],val[13] );	
	//printk("x = %d,y = %d,xx = %d,yy = %d,val[10] = %d,val[12] = %d,val[13] = %d,val[26] = %d,val[34] = %d,val[44] = %d,val[45] = %d,finger_contact\n",x,y,xx,yy,val[10] ,val[12],val[13],val[26] ,val[34] ,val[44] ,val[45],finger_contact);
	
	/* report to input subsystem */

	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR,finger_contact);//finger contact finger_contact
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR,finger_pressure);//pressure finger_pressure
	input_report_abs(ts->input, ABS_MT_POSITION_X,x - 1);//x
	input_report_abs(ts->input, ABS_MT_POSITION_Y,y - 1);//y

	input_mt_sync(ts->input);

	input_sync(ts->input);

	//printk("X= %d Y= %d finger_contact= %d finger_pressure= %d\n",x,y,finger_contact, finger_pressure );	
	//printk("[TSP-LCQ]: X= %d Y= %d CONTACT= %d SIZE= %d R13= %d=%02x\n",(val[2] << 8 | val[3]), (val[4] << 8 | val[5]), val[1],val[12], val[13],val[13] );
goback:
	if(ts->irq)	
	{
		/* Clear interrupt status register */
		//printk("[TSP-LCQ] Clear interrupt status register. ts->irq= %d\n",ts->irq);	
		i2c_smbus_read_word_data(ts->client, ts->f01_data_base + 1);
		enable_irq(ts->irq);
	}

}
static void synaptics_rmi4_irq_setup(struct synaptics_rmi4_data *ts)
{
	printk("[TSP]%s() called\n",__FUNCTION__);
	i2c_smbus_write_byte_data(ts->client,ts->f01_control_base + 1, 0x01);
}

static irqreturn_t synaptics_rmi4_irq_handler(int irq, void *dev_id)
{
	//printk("%s() called\n",__FUNCTION__);
	struct synaptics_rmi4_data *ts = dev_id;

	disable_irq_nosync(ts->irq);
	//udelay(10);
	queue_work(synaptics_wq, &ts->work);
	//udelay(1);
	return IRQ_HANDLED;
}
#else
#define X_OFFSET_MAX	10
#define Y_OFFSET_MAX	20
static int synaptics_rmi4_sensor_report(struct synaptics_rmi4_data *pdata)
{	
	struct synaptics_rmi4_data *ts = pdata;

	/*register status*/
	//int i;
	int finger_pressure,finger_contact;
	static int pre_x = -1,pre_y = -1,pre_val26 = -1;
	int x = 0,y = 0;//,xx,yy;
	unsigned char val[TOTAL_REG];
	//i2c_smbus_read_i2c_block_data(ts->client,0,TOTAL_REG,val); /*一次读27个数，我认为可能太占时间*/
	//printk("tp\n");
	val[1] = i2c_smbus_read_byte_data(ts->client,1); /*读了这个后可以读到19前的数据*/
	//val[2] = i2c_smbus_read_byte_data(ts->client,2); 
	//val[3] = i2c_smbus_read_byte_data(ts->client,3); 
	//val[4] = i2c_smbus_read_byte_data(ts->client,4); 
	val[12] = i2c_smbus_read_byte_data(ts->client,12);
	val[13] = i2c_smbus_read_byte_data(ts->client,13);/*这个数低四位一直变化，没有找到规律*/
	val[19] = i2c_smbus_read_byte_data(ts->client,19);/*读了这个后可以读到19后面的数据*/
	val[22] = i2c_smbus_read_byte_data(ts->client,22);/*xx*/
	val[23] = i2c_smbus_read_byte_data(ts->client,23);/*yy*/
	val[24] = i2c_smbus_read_byte_data(ts->client,24);/*xy*/
	val[26] = i2c_smbus_read_byte_data(ts->client,26);/*这个值只有0和0x7f两个值，暂时用这个值做为size*/
	
	
	/*for (i = 0;i < TOTAL_REG; i++)
	{
		printk("val[%d] = %d = 0x%x\n",i,val[i],val[i]);
	}*/
	 
	//xx = val[2] << 8 | val[3];/*这个值再放手时为0其它时后等于x*/
	//yy = val[4] << 8 | val[5];/*这个值再放手时为0其它时后等于y*/

	x = val[22] << 4 | (val[24] & 0x0f);//x
	y = val[23] << 4 | ((val[24] & 0xf0) >> 4);//y
	//printk("old X= %d Y= %d\n",x,y);	
	
	if ((x > 480) || (x < 1) || (y > 800) || (y < 1))
		goto goback;
	
	/*不想重复上报相同的点
	if ((x == pre_x) && (y == pre_y) && (pre_val26 == val[26]))
		goto goback;
	
	pre_x = x;
	pre_y = y;
	pre_val26 = val[26];
	*/	
	if ((x < 5) || (x > 478))
		x--;
	else
		x = x + X_OFFSET_MAX - X_OFFSET_MAX * x /240;

	if ((y < 3) || (y > 798))
		y--;
	else
		y = y + Y_OFFSET_MAX - Y_OFFSET_MAX * y /400;
	//printk("new X= %d Y= %d\n",x,y);	
		
	if ((x >= 480) || (x < 0) || (y >= 800) || (y < 0))
		goto goback;
	
	//x = x*1000/1025; /*480->468*//*1012 480 ->474*/
	//y = y*1000/1025; /*800->790*//*1012 800->780*/
	finger_pressure = val[26]>>4;//val[13]&0xf;
	finger_contact = (val[12]<<4) | ((val[13] >> 4) & 0xf);
	/*
	finger_contact <<= 1;
	if (finger_contact > 254)
		finger_contact = 254; 
	*/
			
	if (finger_pressure == 0) /*保证放手动作正常(finger_contact == 0) ||*/
		finger_contact = 0;
	
		
	//if (finger_contact > 30)
	//	finger_contact = 30;
	//if (val[26] == 0) 
	//	finger_pressure = finger_contact = 0;
	//else if (finger_pressure < 2)
	//	finger_pressure = 2;
	//if (finger_pressure > 255)
	//	finger_pressure = 30;
	/*
	if ((val[26] == 0) && (finger_contact != 0))
	{
		printk("contact val error\n");
		finger_pressure = finger_contact = 0;
	}*/
	//printk("X= %d Y= %d ,finger_contact = %d finger_pressure = %d ,val[12] = 0x%x,val[13] = 0x%x\n",x,y,finger_contact,finger_pressure,val[12],val[13] );
	//printk("finger_contact = %d finger_pressure = %d\n",finger_contact,finger_pressure);
	//input_report_abs(ts->input, ABS_MT_WIDTH_MINOR,finger_contact_minor);//pressure finger_pressure
	//printk("X= %d Y= %d val[12] = %x val[13] = %x\n",x,y,val[12],val[13] );	
	//printk("x = %d,y = %d,xx = %d,yy = %d,val[10] = %d,val[12] = %d,val[13] = %d,val[26] = %d,val[34] = %d,val[44] = %d,val[45] = %d,finger_contact\n",x,y,xx,yy,val[10] ,val[12],val[13],val[26] ,val[34] ,val[44] ,val[45],finger_contact);
	
	/* report to input subsystem */

	input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR,finger_contact);//finger contact finger_contact
	input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR,finger_pressure);//pressure finger_pressure
	input_report_abs(ts->input, ABS_MT_POSITION_X,x);//x
	input_report_abs(ts->input, ABS_MT_POSITION_Y,y);//y

	input_mt_sync(ts->input);

	input_sync(ts->input);

	//printk("X= %d Y= %d finger_contact= %d finger_pressure= %d\n",x,y,finger_contact, finger_pressure );	
	//printk("[TSP-LCQ]: X= %d Y= %d CONTACT= %d SIZE= %d R13= %d=%02x\n",(val[2] << 8 | val[3]), (val[4] << 8 | val[5]), val[1],val[12], val[13],val[13] );

goback:
	if(ts->irq)
	
	{
		/* Clear interrupt status register */
		//printk("[TSP-LCQ] Clear interrupt status register. ts->irq= %d\n",ts->irq);	
		i2c_smbus_read_word_data(ts->client, ts->f01_data_base + 1);
	}
	return 1;
	//return IRQ_HANDLED;
}

/**
 * synaptics_rmi4_irq() - thread function for rmi4 attention line
 * @irq: irq value
 * @data: void pointer
 *
 * This function is interrupt thread function. It just notifies the
 * application layer that attention is required.
 */
static irqreturn_t synaptics_rmi4_irq_handler(int irq, void *data)
{
	struct synaptics_rmi4_data *ts = data;
	int touch_count;
	if (!ts->touch_stopped){
		touch_count = synaptics_rmi4_sensor_report(ts);
	}
	return IRQ_HANDLED;
}

#endif

static int __devinit synaptics_rmi4_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct synaptics_rmi4_data *ts;
	struct input_dev *input_dev;
	int err;
	const struct synaptics_rmi4_platform_data *platformdata =
						client->dev.platform_data;
	
	printk("[TSP]%s() called\n",__FUNCTION__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_READ_WORD_DATA)) {
		return -EIO;
	}

	ts = kzalloc(sizeof(struct synaptics_rmi4_data), GFP_KERNEL);
	input_dev = input_allocate_device();

	if (!ts || !input_dev) {
		printk("synaptics-rmi4: failed to allocate memory\n");
		err = -ENOMEM;
		goto err_free_mem;
	}
	
	ts->regulator = regulator_get(&client->dev, "vaux2");
	if (IS_ERR(ts->regulator)) {
		dev_err(&client->dev, "%s:get regulator failed\n",
							__func__);
		err = -ENOMEM;//PTR_ERR(ts->regulator);
		goto err_regulator;
	}
	regulator_enable(ts->regulator);
	mdelay(1);	
	// as long as VIO or VAUX2 is powered, no needed to disable touch pins
    // touch i2c has external pullups to VIO
	//nowplus_enable_touch_pins(1);//r3d4 added!
	i2c_set_clientdata(client, ts);

	ts->irq = platformdata->irq_number;//OMAP_GPIO_IRQ(OMAP_GPIO_TOUCH_IRQ);
	ts->f01_control_base = 0x23;
	ts->f01_data_base = 0x13;
	ts->f11_data_base = 0x15; /* FIXME */
	ts->touch_stopped	= false;

	ts->client = client;
	ts->input = input_dev;

#ifdef TP_USE_WORKQUEUE
	INIT_WORK(&ts->work, synaptics_rmi4_work);
#endif
//	input_dev->name = "Synaptics RMI4 touchscreen";
//	input_dev->phys = "synaptics-rmi4/input0";

	input_dev->name = DRIVER_NAME;
	input_dev->phys = "Synaptics_Clearpad";

	input_dev->id.bustype = BUS_I2C;

	//input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	//input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	input_dev->evbit[0] = BIT_MASK(EV_ABS);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, 0, ABS_X_MAX, 0, 0);	
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, 0, ABS_Y_MAX, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 30 , 0, 0);
	//input_set_abs_params(input_dev, ABS_MT_WIDTH_MINOR, 0, 15 , 0, 0);
	



	err = input_register_device(input_dev);
	if (err) {
		printk("synaptics-rmi4: failed to register input device\n");
		goto err_free_mem;
	}

#ifdef TP_USE_WORKQUEUE

	if(ts->irq) {
		err = request_irq(ts->irq, synaptics_rmi4_irq_handler, IRQF_TRIGGER_FALLING, DRIVER_NAME, ts);
	}

	if(!err && ts->irq) {
		synaptics_rmi4_irq_setup(ts);
	} else {
		printk("synaptics-rmi4: GPIO IRQ missing, falling back to polled mode\n");
		ts->irq = 0;

		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_rmi4_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
#else
	if(ts->irq){
		err = request_threaded_irq(ts->irq, NULL,
						synaptics_rmi4_irq_handler,
						platformdata->irq_type,
						platformdata->name, ts);
	}
	else {
		printk("[TSP] no irq!\n");
		goto err_regulator;	
	}
		
	if(err) {
		printk("[TSP] irq request error!\n");
		goto err_regulator;	
	}
#endif
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_rmi4_early_suspend;
	ts->early_suspend.resume = synaptics_rmi4_late_resume;
	register_early_suspend(&ts->early_suspend);
	return 0;

err_free_mem:
	regulator_disable(ts->regulator);
	regulator_put(ts->regulator);
	
err_regulator:
	input_free_device(input_dev);
	kfree(ts);
	return err;
}

static int __devexit synaptics_rmi4_remove(struct i2c_client *client)
{
	struct synaptics_rmi4_data *ts = i2c_get_clientdata(client);
	printk("[TSP]%s() called\n",__FUNCTION__);
	input_unregister_device(ts->input);
	ts->touch_stopped = true;

	if(ts->irq) {
		free_irq(ts->irq, ts);
	}
#ifdef TP_USE_WORKQUEUE
	else {
		hrtimer_cancel(&ts->timer);
	}
#endif
	regulator_disable(ts->regulator);
	regulator_put(ts->regulator);
	kfree(ts);

	return 0;
}

static int synaptics_rmi4_suspend(struct i2c_client *client, pm_message_t msg)
{
	/*touch sleep mode*/
	//int ret;
	struct synaptics_rmi4_data *ts = i2c_get_clientdata(client);
	ts->touch_stopped = true;
	if(ts->irq){		
		//i2c_smbus_read_word_data(ts->client, ts->f01_data_base + 1);
		disable_irq(ts->irq);
	}
#ifdef TP_USE_WORKQUEUE
	else
		hrtimer_cancel(&ts->timer);
#endif
	#if 0
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->irq)
		enable_irq(ts->irq);
	#endif
	//i2c_smbus_read_word_data(ts->client,20);
	//nowplus_enable_touch_pins(0);
	/* 关了电后触摸屏再上电要很久才有响应*/
	if (ts->regulator)
	{
		regulator_disable(ts->regulator); /*关一下电，让TP完全复位*/
		mdelay(100);		
		regulator_enable(ts->regulator); /*这样可以在suspend期间，TP自己恢复工作*/
	}
	printk("[TSP] touchscreen suspend!\n");
	return 0;
}


static int synaptics_rmi4_resume(struct i2c_client *client)
{
	/*touch sleep mode*/

	struct synaptics_rmi4_data *ts = i2c_get_clientdata(client);
	/* 关了电后触摸屏再上电要4秒才有响应
	if (ts->regulator)
		regulator_enable(ts->regulator);
	mdelay(1);
	*/ 
	//nowplus_enable_touch_pins(1);
	mdelay(1);
	if (ts->irq){
		//i2c_smbus_read_word_data(ts->client, ts->f01_data_base + 1);
		enable_irq(ts->irq);
	}
	ts->touch_stopped = false;
#ifdef TP_USE_WORKQUEUE
	if (!ts->irq)
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
#endif
	printk("[TSP] touchscreen resume!\n");
	return 0;
}

static void synaptics_rmi4_early_suspend(struct early_suspend *h)
{
	struct synaptics_rmi4_data *ts;
	printk("[TSP]%s() called\n",__FUNCTION__);
	ts = container_of(h, struct synaptics_rmi4_data, early_suspend);
	synaptics_rmi4_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_rmi4_late_resume(struct early_suspend *h)
{
	struct synaptics_rmi4_data *ts;
	printk("[TSP]%s() called\n",__FUNCTION__);
	ts = container_of(h, struct synaptics_rmi4_data, early_suspend);
	synaptics_rmi4_resume(ts->client);
}

static struct i2c_device_id synaptics_rmi4_idtable[] = {
	{ DRIVER_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, synaptics_rmi4_idtable);

static struct i2c_driver synaptics_rmi4_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= DRIVER_NAME,
	},	
	
	.probe		= synaptics_rmi4_probe,
	.remove		= __devexit_p(synaptics_rmi4_remove),
	.suspend	= synaptics_rmi4_suspend,
	.resume		= synaptics_rmi4_resume,	
	.id_table	= synaptics_rmi4_idtable,
};

static int __init synaptics_rmi4_init(void)
{
	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
	if (!synaptics_wq) {
		return -ENOMEM;
	}

	return i2c_add_driver(&synaptics_rmi4_driver);
}

static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);
}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_AUTHOR("Joerie de Gram <j.de.gram@gmail.com");
MODULE_DESCRIPTION("Synaptics RMI4 touchscreen driver");
MODULE_LICENSE("GPL");

