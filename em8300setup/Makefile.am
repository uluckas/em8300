CFLAGS = @GLOBAL_CFLAGS@
INCLUDES = -I@srcdir@/include -I/usr/local/include

bin_PROGRAMS = em8300setup
noinst_HEADERS = 

em8300setup_SOURCES = em8300setup.c

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
