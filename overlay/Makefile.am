EXTRA_DIST = configs/ autocal.desktop dxr3view.desktop

AM_CFLAGS = @GLOBAL_CFLAGS@ $(GTK_CFLAGS)
INCLUDES = -I@srcdir@/include -I/usr/local/include
AM_LDFLAGS = $(GTK_LIBS)

bin_PROGRAMS = autocal dxr3view
noinst_HEADERS = autocal.h allblackbut.h overlay.h

autocal_SOURCES = autocal.c allblackbut.c overlay.c

dxr3view_SOURCES = dxr3view.c overlay.c

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS) $(GTK_LIBS)"

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
