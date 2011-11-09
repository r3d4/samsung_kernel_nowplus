##############################################################################
# COPYRIGHT(C)	: Samsung Electronics Co.Ltd, 2006-2011 ALL RIGHTS RESERVED
# AUTHOR		: KyoungHOON Kim (khoonk)
##############################################################################
# VERSION&DATE	: Version 1.00	2006/06/02 (khoonk)
##############################################################################

ifneq ($(KERNELRELEASE), )

EXTRA_CFLAGS := -I$(PRJROOT)/modules/include -I$(PRJROOT)/kernel/drivers/media/video/

CAM_NAME = camera
VT_SEN_NAME = vt_sensor
MAIN_SEN_NAME = main_sensor
PMIC_NAME = camera_pmic

obj-m := $(CAM_NAME).o $(VT_SEN_NAME).o $(MAIN_SEN_NAME).o $(PMIC_NAME).o

$(CAM_NAME)-y := omap34xxcam.o 
$(VT_SEN_NAME)-y := s5ka3dfx.o s5ka3dfx_platform.o
$(MAIN_SEN_NAME)-y := m4mo.o m4mo_platform.o
$(PMIC_NAME)-y := cam_pmic.o

else

ifndef KDIR
	KDIR :=../../linux-2.6.24.7  
endif 

all:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules
clean:
	rm -f *.o *.ko *.mod.c *~ .*.cmd

install:
	@$(MAKE) --no-print-directory -C $(KDIR) \
		SUBDIRS=$(CURDIR) modules_install
#	cp dcm.ko $(PRJROOT)/images/lib/modules/

endif

