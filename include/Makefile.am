## Process this file with automake to produce Makefile.in

dxr3incdir = $(includedir)

dxr3inc_HEADERS = libdxr3/api.h

em8300incdir = /usr/include

em8300inc_HEADERS = linux/em8300.h

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(dxr3incdir) $(DESTDIR)$(dxr3incdir)/libdxr3
