AM_CFLAGS = @GLOBAL_CFLAGS@
INCLUDES = -I@top_srcdir@/include

bin_PROGRAMS = em8300setup
man_MANS = em8300setup.1
noinst_HEADERS = 

em8300setup_SOURCES = em8300setup.c

EXTRA_DIST = $(man_MANS)

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
