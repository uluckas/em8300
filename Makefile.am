EXTRA_DIST = modules/* shinit.sh

if HAVE_GTK
SUBDIRS = overlay dhc include scripts em8300setup
else
SUBDIRS = include scripts em8300setup
endif

debug:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) \
	done;
