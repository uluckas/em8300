LIBS = -L/usr/X11R6/lib -lX11 -lm
CFLAGS = -g `gtk-config --cflags` -I/usr/local/include

all:	autocal dxr3view

autocal:	autocal.o allblackbut.o overlay.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

dxr3view:	dxr3view.o overlay.o
	$(CC) $(CFLAGS) `gtk-config --libs` -o $@ $^ $(LIBS)

clean:
	rm -f autocal dxr3view *.o