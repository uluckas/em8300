EXTRA_DIST = dhc.desktop

AM_CFLAGS = @GLOBAL_CFLAGS@ $(GTK_CFLAGS)
INCLUDES = -I@srcdir@/include -I/usr/local/include
AM_LDFLAGS = $(GTK_LIBS) -lpthread

bin_PROGRAMS = dhc
noinst_HEADERS = dhc_callback.h dhc_gui.h pixmaps.h

dhc_SOURCES = dhc_main.c dhc_callback.c dhc_gui.c

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS) $(GTK_LIBS)"

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
