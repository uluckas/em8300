CFLAGS = @BUILD_LIB_STATIC@ @GLOBAL_CFLAGS@ 

noinst_LTLIBRARIES = libac3.la

libac3_la_SOURCES = bitstream.c parse.c crc.c

noinst_HEADERS = ac3_internal.h bitstream.h parse.h crc.h ac3.h bswap.h

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*

maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
