dxr3incdir = $(includedir)/libdxr3

dxr3inc_HEADERS = api.h

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(dxr3incdir)
