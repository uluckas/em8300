EXTRA_DIST = autotools/ modules/ omsplugin/ xine/ scripts/ shinit.sh modules.tar.gz em8300.sysv

SUBDIRS = libdxr3 overlay include

debug:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) \
	done;

clean-generic:
	rm -rf BUILD SPECS RPMS SOURCES SRPMS
	rm -f rpmrc rpmmacros

modules.tar.gz:
	tar czvf modules.tar.gz modules

modules_install:
	$(mkinstalldirs) $(DESTDIR)$(libdir)/em8300
	$(INSTALL_DATA) modules.tar.gz $(DESTDIR)$(libdir)/em8300/
	$(mkinstalldirs) $(DESTDIR)/etc/rc.d/init.d/
	$(INSTALL_PROGRAM) em8300.sysv $(DESTDIR)/etc/rc.d/init.d/em8300
	$(mkinstalldirs) $(DESTDIR)/dev/
	mknod $(DESTDIR)/dev/em8300 c 121 0
	mknod $(DESTDIR)/dev/em8300_mv c 121 1
	mknod $(DESTDIR)/dev/em8300_ma c 121 2
	mknod $(DESTDIR)/dev/em8300_sp c 121 3

dist-rpm: em8300.spec modules.tar.gz dist
	rm -rf    BUILD SPECS RPMS/{noarch,i386,i686} SOURCES SRPMS
	mkdir -p BUILD SPECS RPMS/{noarch,i386,i686} SOURCES SRPMS
	cp -f ${top_srcdir}/@PACKAGE@-@VERSION@.tar.gz ./SOURCES
	echo "%_topdir `pwd`"	> rpmmacros
	echo "macrofiles: /usr/lib/rpm/macros: ... :`pwd`/rpmmacros"	> rpmrc
	rpm --target=i586-pc-linux-gnu --rcfile rpmrc -ba em8300.spec

