XINE_PATH=../..
INSTALL_PATH=/tmp/xine

CFLAGS=-O3 -Wall -I$(XINE_PATH)/xine-lib/src -I$(XINE_PATH)/xine-lib/src/xine-engine -DXINE_COMPILE -fPIC -DPIC -D_FILE_OFFSET_BITS=64
#CFLAGS=-I$(XINE_PATH)/xine-lib/src -I$(XINE_PATH)/xine-lib/src/xine-engine -DXINE_COMPILE -fPIC -DPIC -malign-loops=4 -malign-jumps=4 -malign-functions=4 -mwide-multiply -mpreferred-stack-boundary=2 -fschedule-insns2 -Wall -D_REENTRANT

#DESTS=xineplug_decode_dxr3.so xineplug_vo_out_dxr3.so
DESTS=xineplug_decode_dxr3.so
all: $(DESTS)

xineplug_decode_dxr3.so: dxr3_decoder.o
	gcc -shared -o $@ dxr3_decoder.o

xineplug_vo_out_dxr3.so: video_out_dxr3.o
	gcc -shared -o $@ video_out_dxr3.o

install: all
	cp $(DESTS) $(INSTALL_PATH)/lib/xine/plugins
