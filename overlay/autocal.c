// os stuff
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <gtk/gtk.h>
#include <errno.h>

// X stuff (XOpenDisplay)
#include <X11/Xlib.h>


#include "allblackbut.h"

#include <linux/em8300.h>

#define NUM_MONITORS 1

int xsize=1280;
int ysize=1024;
int depth=24;

Display *dpy;
int scr;

#include "overlay.h"

int pattern_draw(int fgcol, int bgcol,
			     int xpos, int ypos, int width, int height, void *arg)
{
    AllDrawRectangle(fgcol,bgcol, xpos,ypos,width,height);
    return 1;
}

int main( int   argc,
          char *argv[] )
{
    FILE *dev	;
    overlay_t *ov;
    Screen *xscrn;
    
    dpy = XOpenDisplay (NULL);
    if (!dpy) _exit(1);

    xscrn=ScreenOfDisplay(dpy, 0);
    xsize=WidthOfScreen(xscrn)/NUM_MONITORS;
    ysize=HeightOfScreen(xscrn);
    depth=PlanesOfScreen(xscrn);
    fprintf(stderr, "Width: %d, Height: %d, Depth: "
	    "%d\n", xsize, ysize, depth);

    AllBlackButInit();

    if(!(dev=fopen("/dev/em8300-0", "r")))
        {
	    const gchar *errstr = g_strerror(errno);
	    perror("Could not open /dev/em8300-0 for reading");
	    AllBlackButClose();
	    gtk_init(&argc, &argv);
	    GtkWidget *dialog = gtk_message_dialog_new(
		NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
		"Could not open EM8300 device for reading.\n\n"
		"Make sure the hardware is present, modules are "
		"loaded, and you have read permissions to the "
		"device.\n\nThe error was: /dev/em8300-0: %s", errstr);
	    gtk_window_set_title(GTK_WINDOW(dialog),
		"Autocal: Exiting with error");
	    gtk_dialog_run(GTK_DIALOG(dialog));
	    gtk_widget_destroy(dialog);
	    _exit(-1);
        }

    ov = overlay_init(dev);

    overlay_set_screen(ov,xsize,ysize,depth);

    overlay_read_state(ov,NULL);

    overlay_autocalibrate(ov, pattern_draw, NULL);

    overlay_write_state(ov,NULL);
    
    overlay_release(ov);

    fclose(dev);    

    AllBlackButClose();
    return 0;
}
