#define OVERLAY_MAX_WIDTH       998
#define OVERLAY_MAX_HEIGHT      767
#define DEFAULT_RATIO           1.75
#define DEFAULT_WIDTH           720
#define OVERLAY_X_OFFSET        251
#define OVERLAY_Y_OFFSET        27

#define FULLSCREEN_ON		GDK_KP_Multiply
#define FULLSCREEN_OFF		GDK_KP_Divide

#define KEY_COLOR 0x80a040

#include <unistd.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>

#include <linux/em8300.h>

#include "overlay.h"

static gboolean key_cb(GtkWidget *window,
			GdkEventKey *kevent,
			gpointer kdata);

static gboolean event_cb(GtkWidget* window,
			 GdkEventConfigure *event,
			 gpointer user_data);

static void resizeoverlay(int width,
			int height,
			int xpos,
			int ypos);

gint oldx,oldy,oldw,oldh;

FILE *full;
FILE *size ;     
float ratio;
int ratiotemp;
overlay_t *ov;


int main( int argc,
          char *argv[] )
{
	GtkWidget *window;
	GtkWidget *aspect_frame;
	GtkWidget *drawing_area;
	GdkColor gdk_color;
	GdkColormap *colormap;
	FILE *dev;
	
	float ratio;
	int initwidth;
	int tmp;

	dev = fopen("/dev/em8300","r");
	ov = overlay_init(dev);

	/* ****************************************************************
	   This should be set to the actual video resolution of the X-server
	   Henrik
	*/
	overlay_set_screen(ov,1280,1024,16);

	overlay_read_state(ov,NULL);
	overlay_set_keycolor(ov, KEY_COLOR);
	/*******************************************************************
	   When this program can paint the background with the #80a040    		   color the mode could be changed to EM8300_OVERLAY_MODE_OVERLAY
	*/
	overlay_set_mode(ov, EM8300_OVERLAY_MODE_RECTANGLE  );
 	if(argc == 1)
	{
	  ratio = DEFAULT_RATIO;
	  initwidth = DEFAULT_WIDTH;
	}
	else if(argc == 2)
	{
	  ratio = (float)atof(argv[1]);
	  initwidth = DEFAULT_WIDTH;
	}
	else
	{
	  ratio = (float)atof(argv[1]);
	  initwidth = atoi(argv[2]);
	}	
	

	ratiotemp = ((int)(ratio*100));

        gtk_init (&argc, &argv);

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

        gtk_window_set_title (GTK_WINDOW (window), "Viewing Window");

	gtk_window_set_policy(GTK_WINDOW(window),TRUE,TRUE,FALSE);

	gtk_window_set_default_size (GTK_WINDOW (window),
					 initwidth,
					 (initwidth /(gfloat)ratio));	
          


	gtk_signal_connect (GTK_OBJECT (window), 
					"destroy",
                             		GTK_SIGNAL_FUNC(gtk_main_quit),
					 NULL);

        gtk_container_set_border_width (GTK_CONTAINER (window), 0);
         
        aspect_frame = gtk_aspect_frame_new (NULL, /* label */
                                               0.0, /* center x */
                                               0.0, /* center y */
                                               (gfloat)ratio, /* xsize/ysize */
                                               FALSE /* ignore child's aspect */);
         
        gtk_container_add (GTK_CONTAINER(window), aspect_frame);

        gtk_widget_show (aspect_frame);
         
        drawing_area = gtk_drawing_area_new ();
        
        gtk_widget_set_usize (drawing_area,
				 initwidth,
				 initwidth / (gfloat)ratio);

        gtk_container_add (GTK_CONTAINER(aspect_frame), drawing_area);


	/* ****************************************************
	   This is a try to change background color to the chroma key
	   color. Obviously I've done it wrong, cause it doesn't work.
	   Maybe someone with more experience with GTK+ programming
	   could do this better.

	   Henrik
	*/
	
	gdk_color.red = ((KEY_COLOR >> 16)&0xff) * 256;
	gdk_color.green = ((KEY_COLOR >> 8)&0xff) * 256;
	gdk_color.blue = ((KEY_COLOR >> 0)&0xff) * 256;

	/* Allocate color */

        gdk_color_alloc (gtk_widget_get_colormap (drawing_area), &gdk_color);

	/* Set window background color */

        gdk_window_set_background (drawing_area->window, &gdk_color);


	

	gtk_signal_connect(GTK_OBJECT(window),
				"configure_event",
				GTK_SIGNAL_FUNC(event_cb),
				GINT_TO_POINTER(ratiotemp));

	gtk_signal_connect(GTK_OBJECT(window),
				"key_press_event",
				GTK_SIGNAL_FUNC(key_cb),
				GINT_TO_POINTER(ratiotemp));

	
	
	gtk_widget_grab_focus(window);
	
	gtk_widget_show (drawing_area);
         
        gtk_widget_show (window);
        gtk_main ();

	overlay_set_mode(ov, EM8300_OVERLAY_MODE_OFF);
	overlay_release(ov);

	return 0;
      }


static void
resizeoverlay(int width, int height, int xpos, int ypos)
{
    overlay_set_window(ov,xpos,ypos,width,height);
}


static gboolean
event_cb(GtkWidget *window, GdkEventConfigure *event, gpointer user_data)
{
	gint oxpos,oypos,owidth,oheight;
	gint scr_wid,scr_hei;
	scr_wid = gdk_screen_width();
	scr_hei = gdk_screen_height();

	ratio = GPOINTER_TO_INT(user_data) / 100.0;
	oxpos = OVERLAY_X_OFFSET + ((event->x) * OVERLAY_MAX_WIDTH / scr_wid);
	oypos = OVERLAY_Y_OFFSET + (event->y) * OVERLAY_MAX_HEIGHT / scr_hei;
	owidth = (event->width) * OVERLAY_MAX_WIDTH / scr_wid;
	oheight = ((int)(owidth / ratio)) * OVERLAY_MAX_HEIGHT / scr_hei;

	//	resizeoverlay((int)owidth,(int)oheight,(int)oxpos,(int)oypos);
	resizeoverlay(event->width, event->height, event->x, event->y);

	gtk_widget_set_usize(GTK_WIDGET(window),
				(gint)owidth,
				(gint)oheight);

	oldx=oxpos;
	oldy=oypos;
	oldw=owidth;
	oldh=oheight;

	return TRUE;
}

static gboolean
key_cb(GtkWidget *window, GdkEventKey *kevent, gpointer kdata)
{
	float ratio;
	int width,height,xpos,ypos;
	gint scr_wid, scr_hei;

	ratio = GPOINTER_TO_INT(kdata) / 100.0;
 
	scr_wid = gdk_screen_width();
	scr_hei = gdk_screen_height();

	switch (kevent->keyval)
	  {
	  case FULLSCREEN_ON:  		// Fullscreen

		width = OVERLAY_MAX_WIDTH;
		height = ((int)(OVERLAY_MAX_WIDTH / ratio)) * OVERLAY_MAX_HEIGHT / scr_hei;
		xpos = OVERLAY_X_OFFSET;
		ypos = OVERLAY_Y_OFFSET + (OVERLAY_MAX_HEIGHT - (OVERLAY_MAX_WIDTH / ratio)) / 2;
		resizeoverlay(width,height,xpos,ypos);
		break;

	  case FULLSCREEN_OFF:			// Regular
		width = oldw;
		height = oldh;
		xpos = oldx;
		ypos = oldy;
		resizeoverlay(width,height,xpos,ypos);
		break;
	  }

	return TRUE;
}
