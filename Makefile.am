EXTRA_DIST = autotools/ modules/ omsplugin/ xine/ scripts/ shinit.sh

SUBDIRS = libdxr3 overlay include

debug:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  (cd $$subdir && $(MAKE) $@) \
	done;
