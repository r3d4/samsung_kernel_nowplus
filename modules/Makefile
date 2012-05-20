#ifndef PRJROOT
#    $(error Cannot locate project environment)
#endif

#strip debug info
INSTALL_MOD_STRIP :=1
INSTALL_MOD_PATH := ~/lib

MODULES_CAMERA=n
MODULES_DPRAM=y
MODULES_PL_SENSOR=y
MODULES_FMRADIO=y
MODULES_VIBETONZ=y
MODULES_KXSD9=y
MODULES_PARAM=y
MODULES_BATTERY=y
MODULES_OVERCLOCK=y

.PHONY: clean install

ifeq ($(MODULES_DPRAM),y)
#subdir-y += dpram
#subdir-y += multipdp
subdir-y += dpram
subdir-y += multipdp
endif
ifeq ($(MODULES_CAMERA),y)
subdir-y += camera
endif
ifeq ($(MODULES_VIBETONZ),y)
subdir-y += vibetonz
endif
ifeq ($(MODULES_FMRADIO),y)
subdir-y += fm_si4709
endif
ifeq ($(MODULES_KXSD9),y)
subdir-y += kxsd9
endif
ifeq ($(MODULES_PL_SENSOR),y)
subdir-y += PL_sensor
endif
ifeq ($(MODULES_PARAM),y)
subdir-y += param
endif
ifeq ($(MODULES_BATTERY),y)
subdir-y += battery
endif
ifeq ($(MODULES_OVERCLOCK),y)
subdir-y += overclock symsearch
endif

SUBDIRS := $(subdir-y)

DO_INSTALL_MODS = y

all: $(SUBDIRS)

$(SUBDIRS)::
	@$(MAKE) -C $@

install: $(SUBDIRS)
ifeq ($(strip $(DO_INSTALL_MODS)),y)
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) --no-print-directory -C $$i $@ > /dev/null 2>&1; \
	done
endif
	@$(MAKE) --no-print-directory -C $(KDIR) modules_install

clean:
	@for i in `echo $(SUBDIRS)`; do \
		$(MAKE) -C $$i $@; \
	done
