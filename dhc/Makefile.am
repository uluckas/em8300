EXTRA_DIST = dhc.desktop

AM_CFLAGS = @GLOBAL_CFLAGS@ $(GTK_CFLAGS)
INCLUDES = -I@top_srcdir@/include
AM_LDFLAGS = $(GTK_LIBS) -lpthread
CLEANFILES = dhc.xpm

bin_PROGRAMS = dhc
noinst_HEADERS = dhc_callback.h dhc_gui.h pixmaps.h
noinst_DATA = dhc.xpm

dhc_SOURCES = dhc_main.c dhc_callback.c dhc_gui.c

dhc.xpm:
	echo "/* XPM */" > dhc.xpm~~ && \
	sed -ne '/char.*dhc_xpm/,/};/p' $(srcdir)/pixmaps.h >> dhc.xpm~~ && \
	mv dhc.xpm~~ dhc.xpm

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS) $(GTK_LIBS)"

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
