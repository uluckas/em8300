CFLAGS = @GLOBAL_CFLAGS@ `gtk-config --cflags`
INCLUDES = -I@srcdir@/include -I/usr/local/include
LDFLAGS = `gtk-config --libs`

bin_PROGRAMS = autocal dxr3view
noinst_HEADERS = autocal.h allblackbut.h overlay.h

autocal_SOURCES = autocal.c allblackbut.c overlay.c
autocal_DEPENDENCIES = libsdeps

dxr3view_SOURCES = dxr3view.c overlay.c
dxr3view_DEPENDENCIES = libsdeps

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS) `gtk-config --cflags`"

libsdeps:
	@cd $(top_srcdir)/libdxr3; $(MAKE) libdxr3.la

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
