##
## Process this file with automake to produce Makefile.in
##

AUTOMAKE_OPTIONS = no-dependencies foreign

##
DEFS = -fPIC $(CFLAGS) $(GLIB_CFLAGS) @DEFS@
INCLUDES = -I$(top_builddir)/include

libdir = $(OMS_PLUGINDIR)

## Lib
lib_LTLIBRARIES = codec_dxr3.la
codec_dxr3_la_SOURCES = codec.c dxr3-api.c
#codec_dxr3_la_DEPENDENCIES = libsdeps
codec_dxr3_la_LDFLAGS = -avoid-version -module -Wl,-soname,codec_dxr3.so\
	-lac3
#	-L$(top_builddir)/src/plugin/codec/mpeg2dec/.libs -lmpeg2\
#	-L$(top_builddir)/src/plugin/codec/spudec/.libs -lspu\
#	-L$(top_builddir)/src/plugin/codec/mpg123/.libs -lmp3


#libsdeps:
#	@cd $(top_builddir)/src/plugin/codec/spudec; $(MAKE) libspu.la
#	@cd $(top_builddir)/src/plugin/codec/mpeg2dec; $(MAKE) libmpeg2.la


mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*


maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
