em8300incdir = /usr/include/linux/

em8300inc_HEADERS = em8300.h

debug:
	$(MAKE) CFLAGS="$(DEBUG_CFLAGS)"

install-exec-hook:
	$(mkinstalldirs) $(DESTDIR)$(em8300incdir)
