MODULE_VERSION="0.12.0.cvs"
# where the kernel sources are located
KERNEL_LOCATION?=/lib/modules/$(shell uname -r)/build

# If your I2C stuff is builtin to kernel leave this blank
# otherwise change to the location of the I2C headers
#I2C_LOCATION=/usr/local/include
I2C_LOCATION?=

#################################################
# some magic for using linux kernel settings
# when compiling module(s)

EXTRA_CFLAGS = -g -I ../include -I $(KERNEL_LOCATION)/include/ \
		-O2 -fomit-frame-pointer -Wall \
		-DEM8300_VIDEOMODE_DEFAULT=EM8300_VIDEOMODE_PAL \
		-DEM8300_AUDIOMODE_DEFAULT=EM8300_AUDIOMODE_ANALOG

MODULES = em8300.o adv717x.o bt865.o

EM8300_OBJS = em8300_main.o em8300_i2c.o em8300_audio.o em8300_fifo.o \
	   em8300_video.o em8300_misc.o em8300_dicom.o em8300_ucode.o \
	   em8300_ioctl.o em8300_spu.o em9010.o

MODVERS=$(shell cat $(KERNEL_LOCATION)/include/linux/autoconf.h | grep "CONFIG_MODVERSIONS" | cut -d " " -f 3)
KERNVER=$(shell cat $(KERNEL_LOCATION)/include/linux/version.h | grep "UTS" | sed -e 's/.*"\(.*\)"/\1/')
RKERNVER=`uname -r`

MODULE_CC=$(shell $(CC) -v 2>&1 | tail -1)
KERNEL_CC=$(shell cat /proc/version | sed 's/^Linux.*\((gcc.*)\).*$$/\1/')

MODULE_CC:=$(shell echo "$(MODULE_CC)" | cut -d ' ' -f 3)
KERNEL_CC:=$(shell echo "$(KERNEL_CC)" | cut -d ' ' -f 3)

VERSION_HEADER=em8300_version.h

CFLAGS=-D__KERNEL__ -DMODULE $(EM8300_DEBUG) -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer $(EXTRA_CFLAGS)

# Check the kernel version compiled and running
IS22=NO
IS22=$(patsubst 2.2.%,YES,$(KERNVER))
IS22RUN=NO
IS22RUN=$(patsubst 2.2.%,YES,$(RKERNVER))

ifeq "$(MODVERS)" "1"
	CFLAGS+=-DMODVERSIONS -include $(KERNEL_LOCATION)/include/linux/modversions.h
endif

# Change the modules path depending on kernel version, also force I2C for
# 2.2 due to lack of sensible way to detect its presence
ifeq "$(IS22)" "YES"
	SMODINST="misc"
	ifeq "$(I2C_LOCATION)" ""
		CFLAGS+=-DCONFIG_I2C_ALGOBIT
	else
		CFLAGS+=-I$(I2C_LOCATION) -DCONFIG_I2C_ALGOBIT
	endif
else
	SMODINST="kernel/drivers/video"
endif

# Change the modules path depending on kernel version
ifeq "$(IS22RUN)" "YES"
	RMODINST="misc"
else
	RMODINST="kernel/drivers/video"
endif

all : $(MODULES)

debug :
	EM8300_DEBUG="-DDEBUG" make

gcc-check:
	@if [ -z $(IGNORE_CC_MISMATCH) ]; then \
	 if [ $(KERNEL_CC) != $(MODULE_CC) ]; then \
	echo "                                          "; \
	echo "You are trying to compile the em8300      "; \
	echo "driver with a gcc version different from  "; \
	echo "the one used to compile the kernel!       "; \
	echo "                                          "; \
	echo "If you want to override this, set the     "; \
	echo "environment variable IGNORE_CC_MISMATCH   "; \
	echo "                                          "; \
	echo "In any other case, set the CC environment "; \
	echo "variable to the compiler that was used to "; \
	echo "compile the kernel.                       "; \
	exit 1; \
	 fi \
	fi

$(VERSION_HEADER):
	@echo \#define COMPILER_VERSION \"`$(CC) -v 2>&1 | tail -1`\" >  $@
	@echo \#define MODULE_VERSION \"$(MODULE_VERSION)\" >>  $@

em8300.o : gcc-check $(VERSION_HEADER) $(EM8300_OBJS)
	$(LD) $(EXTRA_LDFLAGS) -r -o $@ $(EM8300_OBJS)

install : $(MODULES)
	@echo Installing modules for $(KERNVER) into $(RKERNVER)
	@for module in $^ ;\
	do \
	install -D $$module $(prefix)/lib/modules/$(RKERNVER)/$(RMODINST)/$$module ;\
	done
	@depmod -a
	@echo em8300.h
	@install -D ../include/linux/em8300.h \
		/usr/include/linux/em8300.h
	@echo Installed to running kernel

uninstall :
	@echo Uninstalling modules for $(KERNVER)
	@for module in $^ ;\
	do \
	rm -f $$module $(prefix)/lib/modules/$(RKERNVER)/$(RMODINST)/$$module ;\
	done
	@depmod -a
	@echo em8300.h
	@rm -f /usr/include/linux/em8300.h
	@echo Uninstall successfull

install-newkern : $(MODULES)
	@for module in $^ ;\
	do \
	install -D $$module $(prefix)/lib/modules/$(KERNVER)/$(SMODINST)/$$module ;\
	echo $$module ;\
	done
	@echo em8300.h
	@install -D ../include/linux/em8300.h \
		/usr/include/linux/em8300.h
	@echo Installed to kernel version of header files

devices :
	@sh devices.sh

clean :
	rm -f *.o
	rm -f $(VERSION_HEADER)





