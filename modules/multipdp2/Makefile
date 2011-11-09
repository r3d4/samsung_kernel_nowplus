#
### Makefile for the vibrator driver ICs with vibrator driver
#
#
#obj-y := ../dpram/
obj-m += multipdp.o
#KBUILD_EXTRA_SYMBOLS=../dpram/Module.symvers
all:

	@echo "$(MAKE) --no-print-directory -C $(KDIR) SUBDIRS=$(CURDIR) modules"
	@$(MAKE) --no-print-directory -C $(KDIR) \
        SUBDIRS=$(CURDIR) KBUILD_EXTRA_SYMBOLS=../dpram2/Module.symvers modules

clean:
	rm -rf *.o *.ko *.mod.c *.mod.o *~ .*.cmd .*.o.cmd Module.symvers modules.order .tmp_versions

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
        SUBDIRS=$(CURDIR) modules_install
