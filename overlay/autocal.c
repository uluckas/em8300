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


// X stuff (XOpenDisplay)
#include <X11/Xlib.h>


#include "allblackbut.h"

#include <linux/em8300.h>

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
    int tmp;
    int i;
    overlay_t *ov;
    
    em8300_overlay_calibrate_t cal;
    em8300_overlay_screen_t scr;
      
    dpy = XOpenDisplay (NULL);
    if (!dpy) exit(1);

    AllBlackButInit();

    if(!(dev=fopen("/dev/em8300", "r")))
        {
	    perror("Error opening em8300");
	    exit(-1);
        }

    ov = overlay_init(dev);

    overlay_set_screen(ov,1280,1024,16);

    overlay_read_state(ov,NULL);

    overlay_autocalibrate(ov, pattern_draw, NULL);

    overlay_write_state(ov,NULL);
    
    overlay_release(ov);

    fclose(dev);    

    AllBlackButClose();

}
