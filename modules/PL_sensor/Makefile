ifneq ($(KERNELRELEASE), )

MOD_NAME = pl_sensor
obj-m := $(MOD_NAME).o

#EXTRA_CFLAGS := -I$(PRJROOT)/modules/include

$(MOD_NAME)-y := i2c_drv.o L_dev.o main.o P_dev.o PL_power.o sysfs.o
else

#ifndef KDIR
#	KDIR :=$(PRJROOT)/linux-2.6.29
#endif 

all:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules

clean:
	rm -f *.o *.ko *.mod.c *~ .*.cmd

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install

endif
