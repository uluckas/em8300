AUTOMAKE_OPTIONS = 1.8

firmwaredir = @FIRMWARE_DIR@
alsadir = @ALSA_DIR@

EXTRA_DIST = \
	$(srcdir)/modules/EM8300.conf \
	$(srcdir)/modules/INSTALL \
	$(srcdir)/modules/Makefile \
	$(srcdir)/modules/README* \
	$(srcdir)/modules/*.c \
	$(srcdir)/modules/*.h \
	$(srcdir)/modules/devices.sh \
	$(srcdir)/modules/em8300-udev.rules \
	$(srcdir)/modules/em8300.uc \
	$(srcdir)/modules/ldm \
	$(srcdir)/modules/rmm \
	shinit.sh

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
	rm -f "$(DESTDIR)$(alsadir)/cards/EM8300.conf"
