#define OVERLAY_MAX_WIDTH       998
#define OVERLAY_MAX_HEIGHT      767
//#define DEFAULT_RATIO           1.75
#define DEFAULT_RATIO           1.3333333333333333
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

typedef struct adjust_s {
    GtkWidget *window,*area;
    GtkWidget *aspect_frame;
    GtkSpinButton *width;
    GtkSpinButton *xoff;
    GtkSpinButton *yoff;
    GtkSpinButton *xcorr;
    GtkSpinButton *jitter;
    GtkSpinButton *stability;
    GtkSpinButton *ratio;
} adjust_t;

static gboolean key_cb(GtkWidget *window,
			GdkEventKey *kevent,
			gpointer kdata);

static gboolean expose_cb(GtkWidget *drawing_area,
			  GdkEventExpose *event,
			  adjust_t *aj);

static void resizeoverlay(int width,
			int height,
			int xpos,
			int ypos);

static gpointer
adjust_window(void);

gint oldx,oldy,oldw,oldh;

FILE *full;
FILE *size ;     
float ratio;
int ratiotemp;
overlay_t *ov;
adjust_t *aj;
GdkColor overlay_color;


int main( int argc,
          char *argv[] )
{
	GtkWidget *window;
	GtkWidget *drawing_area;
	GdkColormap *colormap;
	FILE *dev;
	
	float ratio;
	int initwidth;
	int tmp;

	dev = fopen("/dev/em8300","r");
	ov = overlay_init(dev);
	aj = malloc(sizeof(adjust_t));

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
         
        aj->aspect_frame = 
	    gtk_aspect_frame_new (NULL, /* label */
				  0.5, /* center x */
				  0.5, /* center y */
				  (gfloat)ratio, /* xsize/ysize */
				  FALSE /* ignore child's aspect */);
	
	
	gtk_container_add (GTK_CONTAINER(window), aj->aspect_frame);

        gtk_widget_show (aj->aspect_frame);
	
	aj->window = window;
	adjust_window();

        drawing_area = gtk_drawing_area_new ();
	aj->area = drawing_area;
        
        gtk_widget_set_usize (drawing_area,
			      initwidth,
			      initwidth / (gfloat)ratio);

        gtk_container_add (GTK_CONTAINER(aj->aspect_frame), drawing_area);


	/* ****************************************************
	   This is a try to change background color to the chroma key
	   color. Obviously I've done it wrong, cause it doesn't work.
	   Maybe someone with more experience with GTK+ programming
	   could do this better.

	   Henrik
	*/
	
	overlay_color.red = ((KEY_COLOR >> 16)&0xff) * 256;
	overlay_color.green = ((KEY_COLOR >> 8)&0xff) * 256;
	overlay_color.blue = ((KEY_COLOR >> 0)&0xff) * 256;

	/* Allocate color */
        gdk_color_alloc (gtk_widget_get_colormap (drawing_area), 
			 &overlay_color);

	/* Set window background color */
	gtk_signal_connect(GTK_OBJECT(drawing_area),
			   "expose_event",
			   GTK_SIGNAL_FUNC(expose_cb),
			   aj);

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

static gboolean
update_window(GtkObject *adjust,gpointer user_data) 
{
    gfloat ratio = gtk_spin_button_get_value_as_float(aj->ratio);
    gtk_aspect_frame_set(GTK_ASPECT_FRAME(aj->aspect_frame),
			 0.5,0.5,ratio,FALSE);

    ov->xoffset = gtk_spin_button_get_value_as_int(aj->xoff);
    ov->yoffset = gtk_spin_button_get_value_as_int(aj->yoff);
    ov->xcorr = gtk_spin_button_get_value_as_int(aj->xcorr);
    ov->jitter = gtk_spin_button_get_value_as_int(aj->jitter);
    ov->stability = gtk_spin_button_get_value_as_int(aj->stability);
    overlay_update_params(ov);

    gtk_widget_queue_draw(aj->area);

    return TRUE;
}



static GtkSpinButton *
spin_box(char *text,GtkWidget *dialog, gpointer aj,gfloat start,
		int low, int high,int places)
{
    GtkWidget *label,*spin;
    GtkObject *adjust;

    label = gtk_label_new(text);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),label);
    adjust = gtk_adjustment_new(start,low,high,places ? .1 : 1,5,5);
    spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjust),1,places);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),spin);    
    gtk_signal_connect(GTK_OBJECT(adjust),"value-changed",
		       GTK_SIGNAL_FUNC(update_window),NULL);
    return GTK_SPIN_BUTTON(spin);
}

static gpointer
adjust_window(void) 
{
    GtkWidget *dialog,*label,*spin;
    GtkObject *adjust;

    dialog = gtk_dialog_new();
    
//    aj->width     = spin_box("width",     dialog,aj,DEFAULT_WIDTH, 0, 4000,0);
    aj->ratio     = spin_box("ratio",     dialog,aj,DEFAULT_RATIO,.1, 5,   3);
    aj->xoff      = spin_box("x-offset",  dialog,aj,ov->xoffset,   0, 4000,0);
    aj->yoff      = spin_box("y-offset",  dialog,aj,ov->yoffset,   0, 4000,0);
    aj->xcorr     = spin_box("correction",dialog,aj,ov->xcorr,     0, 4000,0);
    aj->jitter    = spin_box("jitter",    dialog,aj,ov->jitter,    0, 4000,0);
    aj->stability = spin_box("stability", dialog,aj,ov->stability, 0, 4000,0);

    gtk_widget_show_all(dialog);
    return aj;
}


static void
resizeoverlay(int width, int height, int xpos, int ypos)
{
    overlay_set_window(ov,xpos,ypos,width,height);
}

static gboolean
expose_cb(GtkWidget *drawing_area,
	  GdkEventExpose *event,
	  adjust_t *aj) 
{
    int w,h,x,y,d;
    gdk_window_set_background(drawing_area->window,&overlay_color);
    gdk_window_clear(drawing_area->window);
    gdk_window_get_geometry(event->window,&x,&y,&w,&h,&d);
    gdk_window_get_origin(event->window,&x,&y);
    resizeoverlay(w,h,x,y);
    oldx=x;
    oldy=y;
    oldw=w;
    oldh=h;
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
