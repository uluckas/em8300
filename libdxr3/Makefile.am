SUBDIRS = libac3

CFLAGS = @GLOBAL_CFLAGS@

INCLUDES = -I.

dxr3incdir = $(includedir)
dxr3inc_HEADERS = dxr3-api.h em8300.h

lib_LTLIBRARIES = libdxr3.la

libdxr3_la_SOURCES = dxr3-api.c ac3spdif.c
libdxr3_la_LIBADD = libac3/libac3.la

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
