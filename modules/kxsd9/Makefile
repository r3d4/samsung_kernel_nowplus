ifneq ($(KERNELRELEASE), )

MOD_NAME = kxsd9
obj-m := $(MOD_NAME).o

#EXTRA_CFLAGS := -I$(PRJROOT)/modules/include

$(MOD_NAME)-y := KXSD9_dev.o KXSD9_i2c_drv.o KXSD9_main.o KXSD9_sysfs.o
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
