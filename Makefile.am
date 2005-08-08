EXTRA_DIST = $(srcdir)/modules/INSTALL $(srcdir)/modules/Makefile $(srcdir)/modules/README* $(srcdir)/modules/*.c $(srcdir)/modules/*.h $(srcdir)/modules/devices.sh $(srcdir)/modules/em8300-udev.rules $(srcdir)/modules/em8300.uc $(srcdir)/modules/ldm $(srcdir)/modules/rmm shinit.sh

if HAVE_GTK
SUBDIRS = include overlay dhc scripts em8300setup
else
SUBDIRS = include scripts em8300setup
endif

debug:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) \
	done;
