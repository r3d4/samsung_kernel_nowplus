##
#	Project	Name	:	Samsung	Android	Build	System
#
#	Copyright	2009	by	Samsung	Electronics,	Inc.
#	All	rights	reserved.
#
#	Project	Description	:
##

##
#	File	Name	:	Android.mk
#
#	File	Description	:
#
#	Author	:	KIM	EUN	GON	(egstyle.kim@samsung.com)
#	Dept	:	Symbian	Lab.	(Open	OS	S/W	Group)
#	Created	Date	:	04/Dec./2009
#	Version	:	Baby-Raccoon
##

ifneq	($(KERNELRELEASE),	)

MOD_NAME	=	samsung_battery
obj-m	:=	$(MOD_NAME).o

EXTRA_CFLAGS := -DCONFIG_SEC_BATTERY_USE_RECOVERY_MODE

$(MOD_NAME)-y	:=	battery_monitor.o	\
				charger_max8845.o	\
				fuelgauge_max17040.o	\
				sleep_i2c1.o	\
				sleep_i2c2.o	\
				sleep_madc.o

else


all:
	@$(MAKE)	--no-print-directory	-C	$(KDIR)	\
		SUBDIRS=$(CURDIR)	modules
clean:
	rm	-f	*.o	*.ko	*.mod.c	*~	.*.cmd

install:
	@$(MAKE)	--no-print-directory	-C	$(KDIR)	\
		SUBDIRS=$(CURDIR)	modules_install

endif
