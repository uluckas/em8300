MODULE_VERSION="0.15.0.cvs"
# where the kernel sources are located
KERNEL_LOCATION?=/lib/modules/$(shell uname -r)/build

# If your I2C stuff is builtin to kernel leave this blank
# otherwise change to the location of the I2C headers
#I2C_LOCATION=/usr/local/include
I2C_LOCATION?=

#################################################
# some magic for using linux kernel settings
# when compiling module(s)

ARCH := $(shell uname -m | sed -e s/i.86/i386/ -e s/sun4u/sparc64/ -e s/arm.*/arm/ -e s/sa110/arm/)
BUILD_IOCTL32 := no

ARCH_CFLAGS =

ifeq "$(ARCH)" "alpha"
ARCH_CFLAGS = -mno-fp-regs -ffixed-8
endif

ifeq "$(ARCH)" "ppc"
ARCH_CFLAGS = -fsigned-char -msoft-float -pipe -ffixed-r2 -mmultiple -mstring
endif

ifeq "$(ARCH)" "sparc64"
ARCH_CFLAGS = -m64 -pipe -mno-fpu -mcpu=ultrasparc -mcmodel=medlow \
	      -ffixed-g4 -fcall-used-g5 -fcall-used-g7 -Wno-sign-compare \
	      -Wa,--undeclared-regs
LD += -m elf64_sparc
BUILD_IOCTL32 := $(shell echo -e '\#include <linux/config.h>\n\#ifdef CONFIG_SPARC32_COMPAT\nyes\n\#else\nno\n\#endif' | cpp -E -I $(KERNEL_LOCATION)/include - | egrep -v '^(\#.*)? *$$')
endif

EXTRA_CFLAGS = -g -I ../include -I $(KERNEL_LOCATION)/include/ \
		-O2 -fomit-frame-pointer -Wall \
		-DEM8300_VIDEOMODE_DEFAULT=EM8300_VIDEOMODE_PAL \
		-DEM8300_AUDIOMODE_DEFAULT=EM8300_AUDIOMODE_ANALOG \
		-DEM8300_VERSION=\"$(MODULE_VERSION)\" \
		-DCONFIG_ADV717X_PIXELPORT16BIT -DCONFIG_ADV717X_PIXELPORTPAL \
		-DCONFIG_EM8300_DICOMPAL -DCONFIG_EM8300_DICOMFIX \
		-DCONFIG_EM8300_DICOMCTRL
ifeq ($(BUILD_IOCTL32),yes)
EXTRA_CFLAGS += -DCONFIG_EM8300_IOCTL32=1
endif

MODULES = em8300.o adv717x.o bt865.o

EM8300_OBJS = em8300_main.o em8300_i2c.o em8300_audio.o em8300_fifo.o \
	   em8300_video.o em8300_misc.o em8300_dicom.o em8300_ucode.o \
	   em8300_ioctl.o em8300_spu.o em9010.o
ifeq ($(BUILD_IOCTL32),yes)
EM8300_OBJS += em8300_ioctl32.o
endif

MODVERS:=$(shell cat $(KERNEL_LOCATION)/include/linux/autoconf.h | grep "CONFIG_MODVERSIONS" | cut -d " " -f 3)
KERNVER?=$(shell cat $(KERNEL_LOCATION)/include/linux/version.h | grep "UTS" | sed -e 's/.*"\(.*\)"/\1/')
RKERNVER:=$(shell uname -r)

CFLAGS=-D__KERNEL__ -DMODULE $(EM8300_DEBUG) -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer $(ARCH_CFLAGS) $(EXTRA_CFLAGS)

# Check the kernel version compiled and running
IS22:=$(patsubst 2.2.%,YES,$(KERNVER))
IS22RUN:=$(patsubst 2.2.%,YES,$(RKERNVER))

ifeq "$(MODVERS)" "1"
	CFLAGS+=-DMODVERSIONS -include $(KERNEL_LOCATION)/include/linux/modversions.h
endif

# Change the modules path depending on kernel version, also force I2C for
# 2.2 due to lack of sensible way to detect its presence
ifeq "$(IS22)" "YES"
	SMODINST:=misc
	ifeq "$(I2C_LOCATION)" ""
		CFLAGS+=-DCONFIG_I2C_ALGOBIT
	else
		CFLAGS:=-I$(I2C_LOCATION) $(CFLAGS) -DCONFIG_I2C_ALGOBIT
	endif
else
	SMODINST:=kernel/drivers/video
	ifneq "$(I2C_LOCATION)" ""
		CFLAGS:=-I$(I2C_LOCATION) $(CFLAGS) -DCONFIG_I2C_ALGOBIT
	endif
endif

# Change the modules path depending on kernel version
ifeq "$(IS22RUN)" "YES"
	RMODINST:=misc
else
	RMODINST:=kernel/drivers/video
endif

all : $(MODULES)

debug :
	EM8300_DEBUG="-DDEBUG" make

em8300.o : $(EM8300_OBJS)
	$(LD) $(EXTRA_LDFLAGS) -r -o $@ $(EM8300_OBJS)

install : $(MODULES)
	@echo Installing modules for $(KERNVER) into $(RKERNVER)
	install -d /lib/modules/$(RKERNVER)/$(RMODINST)
	install $^ /lib/modules/$(RKERNVER)/$(RMODINST)
	@/sbin/depmod -a
	install -D ../include/linux/em8300.h \
		/usr/include/linux/em8300.h
	install -d /usr/share/misc/
	install em8300.uc /usr/share/misc
	@echo Installed to running kernel

uninstall :
	@echo Uninstalling modules for $(KERNVER)
	@for module in $^ ;\
	do \
	rm -f $$module $(prefix)/lib/modules/$(RKERNVER)/$(RMODINST)/$$module ;\
	done
	@/sbin/depmod -a
	@echo em8300.h
	@rm -f /usr/include/linux/em8300.h
	@echo Uninstall successfull

install-newkern : $(MODULES)
	install -d $(prefix)/lib/modules/$(KERNVER)/$(SMODINST)
	install $^ $(prefix)/lib/modules/$(KERNVER)/$(SMODINST)
	@echo Installed to kernel version of header files

devices :
	@sh devices.sh

clean :
	rm -f *.o

