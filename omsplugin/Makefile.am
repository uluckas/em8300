##
## Process this file with automake to produce Makefile.in
##

AUTOMAKE_OPTIONS = no-dependencies foreign

SUBDIRS = sw ts avp dxr2 mgadvd

EXTRA_DIST = $(EXTRA_buf_FILES)

EXTRA_buf_FILES = buf_multi.c buf_single.c


mostlyclean-generic:
	-rm -f *~ \#* .*~ .\#*


maintainer-clean-generic:
	-@echo "This command is intended for maintainers to use;"
	-@echo "it deletes files that may require special tools to rebuild."
	-rm -f Makefile.in
