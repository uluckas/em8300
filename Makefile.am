EXTRA_DIST = autotools/ modules/ omsplugin/ xine/ scripts/ shinit.sh bootstrap

pkgdata_DATA = omsplugin.tar.gz xine.tar.gz modules.tar.gz
pkgdata_SCRIPTS = em8300.sysv

SUBDIRS = libdxr3 overlay include scripts

debug:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) \
	done;

clean-generic:
	rm -rf BUILD SPECS RPMS SOURCES SRPMS
	rm -f rpmrc rpmmacros

omsplugin.tar.gz:
	tar czvf omsplugin.tar.gz omsplugin

xine.tar.gz:
	tar czvf xine.tar.gz xine

modules.tar.gz:
	tar czvf modules.tar.gz modules

dist-rpm: em8300.spec modules.tar.gz dist
	rm -rf    BUILD SPECS RPMS/{noarch,i386,i686} SOURCES SRPMS
	mkdir -p BUILD SPECS RPMS/{noarch,i386,i686} SOURCES SRPMS
	cp -f ${top_srcdir}/@PACKAGE@-@VERSION@.tar.gz ./SOURCES
	echo "%_topdir `pwd`"	> rpmmacros
	echo "macrofiles: /usr/lib/rpm/macros: ... :`pwd`/rpmmacros"	> rpmrc
	rpm --target=i586-pc-linux-gnu --rcfile rpmrc -ba em8300.spec

