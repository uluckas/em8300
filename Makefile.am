EXTRA_DIST = modules/* shinit.sh

if HAVE_GTK
SUBDIRS = libdxr3 overlay dhc include scripts em8300setup
else
SUBDIRS = libdxr3 include scripts em8300setup
endif

debug:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) \
	done;
