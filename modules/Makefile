# where the kernel sources are located
KERNEL_LOCATION=/usr/src/linux

#################################################
# some magic for using linux kernel settings
# when compiling module(s)

CC=gcc

EXTRA_CFLAGS = -g -I $(KERNEL_LOCATION)/include/ -I../include -I/usr/local/include \
		-DEM8300_VIDEOMODE_DEFAULT=EM8300_VIDEOMODE_PAL \
		-DEM8300_AUDIOMODE_DEFAULT=EM8300_AUDIOMODE_ANALOG

MODULES = em8300.o adv717x.o bt865.o eeprom.o

EM8300_OBJS = em8300_main.o em8300_i2c.o em8300_audio.o em8300_fifo.o \
	   em8300_video.o em8300_misc.o em8300_dicom.o em8300_ucode.o \
	   em8300_ioctl.o em8300_spu.o em9010.o

MODVERS=$(shell cat $(KERNEL_LOCATION)/include/config/modversions.h | cut -d " " -f 3)
KERNVER=$(shell cat $(KERNEL_LOCATION)/include/linux/version.h | grep "UTS" | sed -e 's/.*"\(.*\)"/\1/')
 
CFLAGS=-D__KERNEL__ -DMODULE $(EM8300_DEBUG) -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer $(EXTRA_CFLAGS)

ifeq "$(MODVERS)" "1"
	CFLAGS+=-DMODVERSIONS -include $(KERNEL_LOCATION)/include/linux/modversions.h
endif

all : $(MODULES)

debug :
	EM8300_DEBUG="-DDEBUG" make

em8300.o : $(EM8300_OBJS)
	$(LD) $(EXTRA_LDFLAGS) -r -o $@ $(EM8300_OBJS)

install : $(MODULES)
	@for module in $^ ;\
	do \
	install -D $$module /lib/modules/`uname -r`/kernel/drivers/video/$$module ;\
	echo $$module ;\
	done
	@depmod -a
	@echo Installed to running kernel

install-newkern : $(MODULES)
	@for module in $^ ;\
	do \
	install -D $$module /lib/modules/$(KERNVER)/kernel/drivers/video/$$module ;\
	echo $$module ;\
	done
	@echo Installed to kernel version of header files

clean :
	rm *.o
