/* This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <linux/jiffies.h>
#include <plat/gpio.h>
#include <plat/hardware.h>
#include <plat/mux.h>


static int power_key_driver_probe(struct platform_device *plat_dev);
static irqreturn_t powerkey_press_handler(int irq_num, void * dev);
  
static irqreturn_t powerkey_press_handler(int irq_num, void * dev)
{
  struct input_dev *ip_dev = (struct input_dev *)dev;
  int key_press_status=0; 

  if(!ip_dev){
    dev_err(ip_dev->dev.parent,"Input Device not allocated\n");
    return IRQ_HANDLED;
  }
  
  key_press_status = gpio_get_value(OMAP_GPIO_KEY_PWRON);

  if( key_press_status < 0 ){
    dev_err(ip_dev->dev.parent,"Failed to read GPIO value\n");
    return IRQ_HANDLED;
  }
  
  input_report_key(ip_dev,KEY_POWER,key_press_status);
  input_sync(ip_dev);
  dev_dbg(ip_dev->dev.parent,"Sent KEY_POWER event = %d\n",key_press_status);
  printk("[PWR-KEY] KEY_POWER event = %d\n",key_press_status);
  return IRQ_HANDLED;
}

static int power_key_driver_probe(struct platform_device *plat_dev)
{
  struct input_dev *power_key=NULL;
  int pwr_key_irq=-1, err=0;

  pwr_key_irq = platform_get_irq(plat_dev, 0);
  if(pwr_key_irq <= 0 ){
    dev_err(&plat_dev->dev,"failed to map the power key to an IRQ %d\n",pwr_key_irq);
    err = -ENXIO;
    return err;
  }
  power_key = input_allocate_device();
  if(!power_key)
  {
    dev_err(&plat_dev->dev,"failed to allocate an input devd %d \n",pwr_key_irq);
    err = -ENOMEM;
    return err;
  }
  err = request_irq(pwr_key_irq, &powerkey_press_handler ,IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
                    "power_key_driver",power_key);
  if(err) {
    dev_err(&plat_dev->dev,"failed to request an IRQ handler for num %d\n",pwr_key_irq);
    goto free_input_dev;
  }

  /* Let the power key wake the system */
  enable_irq_wake(pwr_key_irq);

  dev_dbg(&plat_dev->dev,"\n Power Key Drive:Assigned IRQ num %d SUCCESS \n",pwr_key_irq);
  /* register the input device now */
  power_key->evbit[0] = BIT_MASK(EV_KEY);
  power_key->keybit[BIT_WORD(KEY_POWER)] = BIT_MASK(KEY_POWER);
  power_key->name = "power_key_driver";
  power_key->phys = "power_key_driver/input0";
  power_key->dev.parent = &plat_dev->dev;
  platform_set_drvdata(plat_dev, power_key);

  err = input_register_device(power_key);
  if (err) {
    dev_err(&plat_dev->dev, "power key couldn't be registered: %d\n", err);
    goto release_irq_num;
  }
 
  return 0;

release_irq_num:
  free_irq(pwr_key_irq,NULL); //pass devID as NULL as device registration failed 

free_input_dev:
  input_free_device(power_key);

return err;

}

static int power_key_driver_remove(struct platform_device *plat_dev)
{
  struct input_dev *ip_dev= platform_get_drvdata(plat_dev);
  int pwr_key_irq=0;

  pwr_key_irq = platform_get_irq(plat_dev,0);
  
  free_irq(pwr_key_irq,ip_dev);

  input_unregister_device(ip_dev);

  return 0;
}
struct platform_driver power_key_driver_t = {
        .probe          = &power_key_driver_probe,
        .remove         = __devexit_p(power_key_driver_remove),
        .driver         = {
                .name   = "power_key_device", 
                .owner  = THIS_MODULE,
        },
};

static int __init power_key_driver_init(void)
{
        return platform_driver_register(&power_key_driver_t);
}
module_init(power_key_driver_init);

static void __exit power_key_driver_exit(void)
{
        platform_driver_unregister(&power_key_driver_t);
}
module_exit(power_key_driver_exit);

MODULE_ALIAS("platform:power key driver");
MODULE_DESCRIPTION("board zeus power key");
MODULE_LICENSE("GPL");






















