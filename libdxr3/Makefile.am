EXTRA_DIST = autotools/
SUBDIRS = libac3

CFLAGS = @GLOBAL_CFLAGS@
INCLUDES = -I@srcdir@/include

lib_LTLIBRARIES = libdxr3.la

libdxr3_la_SOURCES = dxr3-api.c ac3spdif.c
libdxr3_la_LIBADD = libac3/libac3.la
# libdxr3_la_LDFLAGS = -version-info $EM8300_MAJOR.$EM8300_MINOR.$EM8300_SUB$EM8300_PRE

localdebug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

debug:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) \
	done;
	$(MAKE) localdebug

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
