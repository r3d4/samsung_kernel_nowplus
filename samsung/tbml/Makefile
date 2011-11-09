#
#   Makefile for eXtended Sector Remapper (XSR) driver
#
#   Copyright(c) 2004-2006, Samsung Electronics, Co., Ltd.
#

# tinyBML for cramfs booting */
EXTRA_CFLAGS	+= -I$(srctree)/samsung/tbml/include \
		   -I$(srctree)/samsung/tbml/ -DXSR_NW
# For XSR 1.5 RC5 and below version
#EXTRA_CFLAGS	+= -DSYNC_BURST_READ

ifeq ($(CONFIG_RFS_TINY_DEBUG),y)
#EXTRA_CFLAGS	+= -D_RFS_INTERNAL_RESET
#EXTRA_CFLAGS	+= -D_RFS_INTERNAL_STAT_BH
endif

# Note: The following options are only used for development purpose
#	We don't guarantee these options on production
#EXTRA_CFLAGS	+= -D__RFS_INTERNAL_BLOCK_IO

# For test suspend/resume
#EXTRA_CFLAGS	+= -D__BML_INTERNAL_PM_TEST__

# Kernel gcov
ifeq ($(CONFIG_GCOV_PROFILE),y)
ifeq ($(PATCHLEVEL),4)
include Makefile.gcov
else
include $(srctree)/drivers/tbml/Makefile.gcov
endif
endif

ifeq ($(PATCHLEVEL),4)
include Makefile.24
else
include $(srctree)/samsung/tbml/Makefile.26
endif
