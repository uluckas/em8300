##
## Process this file with automake to produce Makefile.in
##

INCLUDES = -I../../codec/ac3dec
libdir = $(PLUGIN_DIR)

## Lib
lib_LTLIBRARIES = codec_dxr3.la
codec_dxr3_la_SOURCES = dxr3_mpeg2.c dxr3_spu.c dxr3_ac3.c \
                        audio_dxr3.c video_dxr3.c plugin.c
codec_dxr3_la_LDFLAGS = -avoid-version -module -Wl,-soname,codec_dxr3
codec_dxr3_la_LIBADD = ../../codec/ac3dec/libac3/libac3.la -ldxr3
