# $Id$
#
# Makefile -- makefile for the em8300 modules
# Copyright (C) 2004 Nicolas Boullis <nboullis@debian.org>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

ifeq ($(KERNELRELEASE),)

KERNEL_LOCATION := /lib/modules/$(shell uname -r)/build

# If you are using external i2c modules, you should set
# EM8300_I2C_LOCATION to the source directory of the modules.
# Moreover, if the API of the modules is not properly guessed, you can
# force it by setting EM8300_I2C_FORCE_API to NEW. (That is
# especially for using 2.8.x modules with 2.4.x kernels.)
#export EM8300_I2C_LOCATION := /usr/src/modules/i2c
#export EM8300_I2C_FORCE_API := NEW
export EM8300_VERSION := "0.15.2"
export EM8300_DIR := $(PWD)/..

build:
	$(MAKE) -C $(KERNEL_LOCATION) SUBDIRS=$(PWD) modules

clean:
	-rm -f *.o *.ko *.mod.c .*.cmd .*.o.flags

KERNVER := $(shell $(CPP) -dM $(KERNEL_LOCATION)/include/linux/version.h | sed -ne 's/^\#define[[:space:]]\+UTS_RELEASE[[:space:]]\+"\(.*\)"$$/\1/p')
MODINST := em8300

# Test if the kernel if older than 2.5.51
ifeq ($(shell echo $(KERNVER) | sed -e 's/^\([[:digit:]]\+\)\.\([[:digit:]]\+\)\.\([[:digit:]]\+\).*/\1 * 65536 + \2 * 256 + \3 < 2 * 65536 + 5 * 256 + 51/' | xargs expr),1)
MODULES := adv717x.o bt865.o em8300.o
else
MODULES := adv717x.ko bt865.ko em8300.ko
endif

install: $(MODULES)
	install -d $(DESTDIR)/lib/modules/$(KERNVER)/$(MODINST)
	install $(MODULES) $(DESTDIR)/lib/modules/$(KERNVER)/$(MODINST)

uninstall:
	rm -rf /lib/modules/$(KERNVER)/$(MODINST)

.PHONY: build clean install uninstall

else

BUILD_IOCTL32 := n

ifeq ($(ARCH),sparc64)
BUILD_IOCTL32 := $(CONFIG_SPARC32_COMPAT)
endif

ifeq ($(ARCH),x86_64)
BUILD_IOCTL32 := $(CONFIG_IA32_EMULATION)
endif

EXTRA_CFLAGS := -I$(EM8300_DIR)/include \
		-DEM8300_VIDEOMODE_DEFAULT=EM8300_VIDEOMODE_PAL \
		-DEM8300_AUDIOMODE_DEFAULT=EM8300_AUDIOMODE_ANALOG \
		-DEM8300_VERSION=\"$(EM8300_VERSION)\" \
		-DCONFIG_ADV717X_PIXELPORT16BIT -DCONFIG_ADV717X_PIXELPORTPAL \
		-DCONFIG_EM8300_DICOMPAL -DCONFIG_EM8300_DICOMFIX \
		-DCONFIG_EM8300_DICOMCTRL
ifeq ($(BUILD_IOCTL32),y)
EXTRA_CFLAGS += -DCONFIG_EM8300_IOCTL32=1
endif
ifneq ($(EM8300_I2C_LOCATION),)
EXTRA_CFLAGS += -DCONFIG_I2C_ALGOBIT=1
# This is a hack to include i2c headers from $(EM8300_I2C_LOCATION)
# over those from the kernel source tree.
override CC += -I$(EM8300_I2C_LOCATION)
endif
ifneq ($(EM8300_I2C_FORCE_API),)
EXTRA_CFLAGS += -DEM8300_I2C_FORCE_$(EM8300_I2C_FORCE_API)_API
endif

#MOD_DESTDIR := drivers/video

em8300-objs := em8300_main.o em8300_i2c.o em8300_audio.o em8300_fifo.o \
		em8300_video.o em8300_misc.o em8300_dicom.o em8300_ucode.o \
		em8300_ioctl.o em8300_spu.o em9010.o em8300_registration.o \
		em8300_procfs.o em8300_devfs.o em8300_udev.o em8300_sysfs.o
ifeq ($(BUILD_IOCTL32),y)
em8300-objs += em8300_ioctl32.o
endif

obj-m += adv717x.o
obj-m += bt865.o
obj-m += em8300.o

# The following hack tests for old kernels (2.4 and old 2.5).
ifneq ($(TOPDIR),$(shell /bin/pwd))

include $(TOPDIR)/Rules.make

em8300.o: $(em8300-objs)
	$(LD) $(LDFLAGS) $(LDFLAGS_MODULE) -r -o $@ $(em8300-objs)

endif

endif
