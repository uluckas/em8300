AUTOMAKE_OPTIONS = 1.8

firmwaredir = @FIRMWARE_DIR@
alsadir = @ALSA_DIR@

EXTRA_DIST = \
	$(srcdir)/modules/EM8300.conf \
	$(srcdir)/modules/INSTALL \
	$(srcdir)/modules/Makefile \
	$(srcdir)/modules/Kbuild \
	$(srcdir)/modules/README* \
	$(srcdir)/modules/adv717x.c \
	$(srcdir)/modules/adv717x.h \
	$(srcdir)/modules/bt865.c \
	$(srcdir)/modules/bt865.h \
	$(srcdir)/modules/em8300_alsa.c \
	$(srcdir)/modules/em8300_alsa.h \
	$(srcdir)/modules/em8300_audio.c \
	$(srcdir)/modules/em8300_compat24.h \
	$(srcdir)/modules/em8300_devfs.c \
	$(srcdir)/modules/em8300_devfs.h \
	$(srcdir)/modules/em8300_dicom.c \
	$(srcdir)/modules/em8300_eeprom.c \
	$(srcdir)/modules/em8300_eeprom.h \
	$(srcdir)/modules/em8300_fifo.c \
	$(srcdir)/modules/em8300_fifo.h \
	$(srcdir)/modules/em8300_i2c.c \
	$(srcdir)/modules/em8300_ioctl.c \
	$(srcdir)/modules/em8300_ioctl32.c \
	$(srcdir)/modules/em8300_ioctl32.h \
	$(srcdir)/modules/em8300_main.c \
	$(srcdir)/modules/em8300_misc.c \
	$(srcdir)/modules/em8300_models.c \
	$(srcdir)/modules/em8300_models.h \
	$(srcdir)/modules/em8300_params.c \
	$(srcdir)/modules/em8300_params.h \
	$(srcdir)/modules/em8300_procfs.c \
	$(srcdir)/modules/em8300_procfs.h \
	$(srcdir)/modules/em8300_reg.c \
	$(srcdir)/modules/em8300_reg.h \
	$(srcdir)/modules/em8300_registration.c \
	$(srcdir)/modules/em8300_registration.h \
	$(srcdir)/modules/em8300_spu.c \
	$(srcdir)/modules/em8300_sysfs.c \
	$(srcdir)/modules/em8300_sysfs.h \
	$(srcdir)/modules/em8300_ucode.c \
	$(srcdir)/modules/em8300_udev.c \
	$(srcdir)/modules/em8300_udev.h \
	$(srcdir)/modules/em8300_video.c \
	$(srcdir)/modules/em9010.c \
	$(srcdir)/modules/encoder.h \
	$(srcdir)/modules/encoder_output_mode.h \
	$(srcdir)/modules/devices.sh \
	$(srcdir)/modules/em8300-udev.rules \
	$(srcdir)/modules/em8300.uc \
	$(srcdir)/modules/ldm \
	$(srcdir)/modules/rmm \
	shinit.sh \
	ChangeLog.old

if HAVE_GTK
SUBDIRS = include overlay dhc scripts em8300setup
else
SUBDIRS = include scripts em8300setup
endif

debug:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) \
	done;

install-data-local:
	test -z "$(firmwaredir)" || $(mkdir_p) "$(DESTDIR)$(firmwaredir)"
	test ! -f $(top_srcdir)/modules/em8300.uc || $(INSTALL_DATA) $(top_srcdir)/modules/em8300.uc "$(DESTDIR)$(firmwaredir)/em8300.bin"
	test -z "$(alsadir)" || $(mkdir_p) "$(DESTDIR)$(alsadir)/cards"
	test -z "$(alsadir)" || $(INSTALL_DATA) $(top_srcdir)/modules/EM8300.conf "$(DESTDIR)$(alsadir)/cards/EM8300.conf"

uninstall-local:
	rm -f "$(DESTDIR)$(firmwaredir)/em8300.bin"
	test -z "$(alsadir)" || rm -f "$(DESTDIR)$(alsadir)/cards/EM8300.conf"

ChangeLog:
	hg log -R $(top_srcdir) --follow --style changelog > $@

dist-hook:
	version="$(VERSION)"; \
	if [ "$${version:(-3)}" == "-hg" ]; then \
	  version="hg-$$(hg id -R $(top_srcdir) -i)"; \
	fi; \
	printf '#define EM8300_VERSION "%s"\n' "$$version" > $(distdir)/modules/em8300_version.h

if MAINTAINER_MODE
.PHONY: ChangeLog
ACLOCAL_AMFLAGS = -I autotools
endif
