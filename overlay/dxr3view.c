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

// X stuff (XOpenDisplay)
#include <X11/Xlib.h>

#define DEFAULT_RATIO   1.33
#define DEFAULT_WIDTH   720
#define NUM_MONITORS    1

#define FULLSCREEN	GDK_KP_Multiply
#define QUIT		GDK_Escape
#define RATIOSELECT	GDK_R
#define RATIOSELECTLOW	GDK_r

#define KEY_COLOR 	0x80a040

typedef struct adjust_s {
	GtkWidget *dialog;
	GtkSpinButton *width;
	GtkSpinButton *xoff;
	GtkSpinButton *yoff;
	GtkSpinButton *xcorr;
	GtkSpinButton *jitter;
	GtkSpinButton *stability;
	GtkSpinButton *ratio;
} adjust_t;

typedef struct menu_s {
	GtkWidget *popup_menu;
	GtkWidget *monitor_menu;
	GtkWidget *aspect_menu;
	GtkWidget *settings_menu;
} menu_t;

typedef struct _dxr3view_globals {
	GtkWidget *window;
	GtkWidget *area;
	menu_t* menu;
	adjust_t* aj;
	overlay_t* ov;
	GdkColor overlay_color;

	int xpos, ypos, width;
	gint oldx, oldy, oldw;
	gint scr_wid, scr_hei, scr_dep;
	int monitor_xoff;

	gboolean fullscreen;
	float ratio;

	int ratiotoggle;
	float* ratiolist;
	int ratiolast;
} dxr3view_globals;

void create_window(dxr3view_globals *g);
void create_adjust(dxr3view_globals *g);
void create_popup_menu(dxr3view_globals *g);

static gboolean
update_ajwindow_cb(GtkObject *adjust,
		   dxr3view_globals *g);

static GtkSpinButton *
create_spin_box(char *text,
		GtkWidget *dialog,
		gfloat start,
		int low,
		int high,
		int places,
		dxr3view_globals *g);

static void
resize_overlay(overlay_t *ov,
	       int width,
	       int height,
	       int xpos,
	       int ypos);

static gboolean
overlay_mode_off_cb(GtkMenuItem *item,
		    overlay_t *ov);

static gboolean
overlay_mode_cb(GtkMenuItem *item,
		overlay_t *ov);

static gboolean
rectangle_mode_cb(GtkMenuItem *item,
		  overlay_t *ov);

static gboolean
monitor_cb(GtkMenuItem *item,
	   dxr3view_globals *g);

static gboolean
ratio_cb(GtkMenuItem *item,
	 dxr3view_globals *g);

static gboolean
fullscreen_cb(GtkMenuItem *item,
	      dxr3view_globals *g);

static gboolean 
expose_cb(GtkWidget *area,
	  GdkEventExpose *event,
	  dxr3view_globals *g);

static gboolean 
event_cb(GtkWidget* window,
	 GdkEventConfigure *event,
	 dxr3view_globals *g);

static gint
popup_cb(GtkWidget *widget, 
	 GdkEventButton *event, 
	 menu_t *menu);

static gboolean 
key_cb(GtkWidget *window,
       GdkEventKey *kevent,
       dxr3view_globals *g);

int main( int argc, char *argv[] )
{
	dxr3view_globals g;
	GdkColormap *colormap;
	FILE *dev;
	float tmpratiolist[7] = {0,1.33,1.5,1.66,1.75,1.85,2.0};
	Screen *xscrn;
	Display *dpy;
	
	
	dev = fopen("/dev/em8300","r");
	g.ov = overlay_init(dev);
	g.aj = malloc(sizeof(adjust_t));
	g.menu = malloc(sizeof(menu_t));
	g.xpos = 0;
	g.ypos = 0;
        g.monitor_xoff = 0; 
	g.fullscreen = 0;
	g.ratiotoggle = 0;

	g.ratiolast = 7;
	g.ratiolist = malloc(7*sizeof(int));
	memcpy(g.ratiolist, &tmpratiolist, 7*sizeof(int));

 	if(argc == 1) {
		g.ratio = DEFAULT_RATIO;
		g.width = DEFAULT_WIDTH;
		g.ratiolist[0] = g.ratio;
	} else if(argc == 2) {
		g.ratio = (float)atof(argv[1]);
		g.width = DEFAULT_WIDTH;
		g.ratiolist[0] = g.ratio;
	} else {
		g.ratio = (float)atof(argv[1]);
		g.width = atoi(argv[2]);
		g.ratiolist[0] = g.ratio;
	}	

        gtk_init (&argc, &argv);
	gdk_rgb_init();
	
	/* Setup Overlay */	
		
	dpy = XOpenDisplay (NULL);
	if (!dpy) exit(1);
	xscrn = ScreenOfDisplay (dpy, 0);
	g.scr_dep = PlanesOfScreen (xscrn);
	
	g.scr_wid = gdk_screen_width() / NUM_MONITORS;
	g.scr_hei = gdk_screen_height();
	printf("width: %i\theight: %i\tdepth: %i\n", g.scr_wid, g.scr_hei, g.scr_dep);

	/* init/release/init to fix vertical squish */

	overlay_set_screen(g.ov, g.scr_wid, g.scr_hei, g.scr_dep);
	overlay_read_state(g.ov, NULL);
	overlay_set_keycolor(g.ov, KEY_COLOR);
	overlay_set_mode(g.ov, EM8300_OVERLAY_MODE_OVERLAY );

	overlay_set_mode(g.ov, EM8300_OVERLAY_MODE_OFF);
	overlay_write_state(g.ov, NULL);
	overlay_release(g.ov);
	g.ov = overlay_init(dev);

	overlay_set_screen(g.ov, g.scr_wid, g.scr_hei, g.scr_dep);
	overlay_read_state(g.ov, NULL);
	overlay_set_keycolor(g.ov, KEY_COLOR);
	overlay_set_mode(g.ov, EM8300_OVERLAY_MODE_OVERLAY ); 

	/* Build Interface */

	create_window(&g);

	update_ajwindow_cb(NULL, &g);

 	g.overlay_color.red = ((KEY_COLOR >> 16)&0xff) * 256;
 	g.overlay_color.green = ((KEY_COLOR >> 8)&0xff) * 256;
 	g.overlay_color.blue = ((KEY_COLOR >> 0)&0xff) * 256;

	/* Allocate color */
        gdk_colormap_alloc_color(gtk_widget_get_colormap (g.area), &g.overlay_color, FALSE,TRUE);

	/* Show Widgets */
	
	gtk_widget_grab_focus(g.window);
	
	gtk_widget_show (g.area);
         
        gtk_widget_show (g.window);

        gtk_main ();

	overlay_set_mode(g.ov, EM8300_OVERLAY_MODE_OFF);
	overlay_write_state(g.ov, NULL);
	overlay_release(g.ov);

	return 0;
}

void create_window(dxr3view_globals *g)
{
	GtkWidget* window;

        window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	g->window = window;

        gtk_window_set_title (GTK_WINDOW (window), "Viewing Window");
	gtk_window_set_policy(GTK_WINDOW(window),TRUE,TRUE,FALSE);
	gtk_window_set_default_size (GTK_WINDOW (window),
				     g->width,
				     (g->width/g->ratio));	
	gtk_container_set_border_width (GTK_CONTAINER (window), 0);

	g->area = gtk_drawing_area_new ();

        gtk_container_add (GTK_CONTAINER(window), g->area);

	create_adjust(g);
	create_popup_menu(g);

	/* General Signal Connects */
 
	gtk_signal_connect(GTK_OBJECT (window), 
			   "destroy",
			   GTK_SIGNAL_FUNC(gtk_main_quit),
			   NULL);
		
	gtk_signal_connect(GTK_OBJECT(g->area),
			   "expose_event",
			   GTK_SIGNAL_FUNC(expose_cb),
			   g);
	
	gtk_signal_connect(GTK_OBJECT(window),
			   "key_press_event",
			   GTK_SIGNAL_FUNC(key_cb),
			   g);

 	gtk_signal_connect(GTK_OBJECT(window),
			   "configure_event",
			   GTK_SIGNAL_FUNC(event_cb),
			   g);
	
 	gtk_signal_connect(GTK_OBJECT(window),
			   "button_press_event",
			   GTK_SIGNAL_FUNC(popup_cb),
			   g->menu);
}

void create_adjust(dxr3view_globals *g)
{
	GtkWidget *dialog;
	adjust_t *aj = g->aj;

	dialog = gtk_dialog_new();
	aj->dialog = dialog;

//	aj->width     = create_spin_box("width",     dialog,DEFAULT_WIDTH, 0, 4000,0,g);
	aj->ratio     = create_spin_box("ratio",     dialog,DEFAULT_RATIO,.1, 5,   3,g);
	aj->xoff      = create_spin_box("x-offset",  dialog,g->ov->xoffset,   0, 4000,0,g);
	aj->yoff      = create_spin_box("y-offset",  dialog,g->ov->yoffset,   0, 4000,0,g);
	aj->xcorr     = create_spin_box("correction",dialog,g->ov->xcorr,     0, 4000,0,g);
	aj->jitter    = create_spin_box("jitter",    dialog,g->ov->jitter,    0, 4000,0,g);
	aj->stability = create_spin_box("stability", dialog,g->ov->stability, 0, 4000,0,g);

	gtk_widget_show_all(dialog);
}

static gboolean
ratio_133_cb(GtkMenuItem *item, dxr3view_globals *g)
{ g->ratiotoggle = 0; return ratio_cb(NULL, g); }

static gboolean
ratio_150_cb(GtkMenuItem *item, dxr3view_globals *g)
{ g->ratiotoggle = 1; return ratio_cb(NULL, g); }

static gboolean
ratio_166_cb(GtkMenuItem *item, dxr3view_globals *g)
{ g->ratiotoggle = 2; return ratio_cb(NULL, g); }

static gboolean
ratio_175_cb(GtkMenuItem *item, dxr3view_globals *g)
{ g->ratiotoggle = 3; return ratio_cb(NULL, g); }

static gboolean
ratio_185_cb(GtkMenuItem *item, dxr3view_globals *g)
{ g->ratiotoggle = 4; return ratio_cb(NULL, g); }

static gboolean
ratio_200_cb(GtkMenuItem *item, dxr3view_globals *g)
{ g->ratiotoggle = 5; return ratio_cb(NULL, g); }

void create_popup_menu(dxr3view_globals *g)
{
	GtkWidget *popup_item;
	menu_t* menu = g->menu;

	menu->popup_menu    = gtk_menu_new();
	menu->monitor_menu  = gtk_menu_new();
	menu->aspect_menu   = gtk_menu_new();
	menu->settings_menu = gtk_menu_new();

	popup_item = gtk_menu_item_new_with_label("Fullscreen");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(fullscreen_cb),
			    g);
	gtk_menu_append(GTK_MENU(menu->popup_menu),popup_item);

	popup_item = gtk_menu_item_new_with_label("Multi-Monitor");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(monitor_cb),
			    g);
	gtk_menu_append(GTK_MENU(menu->popup_menu),popup_item);

	popup_item = gtk_menu_item_new_with_label("Aspect Ratio");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(popup_item),menu->aspect_menu);
	gtk_menu_append(GTK_MENU(menu->popup_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("2.00");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(ratio_200_cb),
			    g);
	gtk_menu_append(GTK_MENU(menu->aspect_menu),popup_item);
	
	
	popup_item = gtk_menu_item_new_with_label("1.85");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(ratio_185_cb),
			    g);
	gtk_menu_append(GTK_MENU(menu->aspect_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("1.75");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(ratio_175_cb),
			    g);
	gtk_menu_append(GTK_MENU(menu->aspect_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("1.66");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(ratio_166_cb),
			    g);
	gtk_menu_append(GTK_MENU(menu->aspect_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("1.50");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(ratio_150_cb),
			    g);
	gtk_menu_append(GTK_MENU(menu->aspect_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("1.33");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(ratio_133_cb),
			    g);
	gtk_menu_append(GTK_MENU(menu->aspect_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("Settings");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(popup_item),menu->settings_menu);
	gtk_menu_append(GTK_MENU(menu->popup_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("Adjustment");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(monitor_cb),
			    g);
	gtk_menu_append(GTK_MENU(menu->settings_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("Overlay");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(overlay_mode_cb),
			    g->ov);
	gtk_menu_append(GTK_MENU(menu->settings_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("Rectangle");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(rectangle_mode_cb),
			    g->ov);
	gtk_menu_append(GTK_MENU(menu->settings_menu),popup_item);

	popup_item = gtk_menu_item_new_with_label("Off");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(overlay_mode_off_cb),
			    g->ov);
	gtk_menu_append(GTK_MENU(menu->settings_menu),popup_item);
	
	popup_item = gtk_menu_item_new_with_label("Exit");
	gtk_signal_connect (GTK_OBJECT(popup_item),
			    "activate",
			    GTK_SIGNAL_FUNC(gtk_main_quit),
			    NULL);
	gtk_menu_append(GTK_MENU(menu->popup_menu),popup_item);
}
	
static gboolean
update_ajwindow_cb(GtkObject *adjust, dxr3view_globals *g) 
{
    g->ov->xoffset = gtk_spin_button_get_value_as_int(g->aj->xoff);
    g->ov->yoffset = gtk_spin_button_get_value_as_int(g->aj->yoff);
    g->ov->xcorr = gtk_spin_button_get_value_as_int(g->aj->xcorr);
    g->ov->jitter = gtk_spin_button_get_value_as_int(g->aj->jitter);
    g->ov->stability = gtk_spin_button_get_value_as_int(g->aj->stability);
    overlay_update_params(g->ov);

    if (g->ratiolist[0] != gtk_spin_button_get_value_as_float(g->aj->ratio)) {
	    g->ratiolist[0] = gtk_spin_button_get_value_as_float(g->aj->ratio);
	//    g->ratiotoggle = -1;
	//    ratio_cb(NULL, g);
    } else {
	    gtk_widget_queue_draw(g->area);
    }

    return TRUE;
}

static GtkSpinButton *
create_spin_box(char *text,GtkWidget *dialog, gfloat start,
		int low, int high, int places, dxr3view_globals *g)
{
    GtkWidget *label,*spin;
    GtkObject *adjust;

    label = gtk_label_new(text);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),label);
    adjust = gtk_adjustment_new(start,low,high,places ? .1 : 1,5,5);
    spin = gtk_spin_button_new(GTK_ADJUSTMENT(adjust),1,places);
    gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->vbox),spin);    
    gtk_signal_connect(GTK_OBJECT(adjust),"value-changed",
		       GTK_SIGNAL_FUNC(update_ajwindow_cb), g);
    return GTK_SPIN_BUTTON(spin);
}

static void
resize_overlay(overlay_t* ov, int width, int height, int xpos, int ypos)
{
	overlay_set_window(ov, xpos, ypos, width, height);
}

static gboolean
overlay_mode_off_cb(GtkMenuItem *item, overlay_t* ov)
{
	overlay_set_mode(ov, EM8300_OVERLAY_MODE_OFF);
	return TRUE;
}

static gboolean
overlay_mode_cb(GtkMenuItem *item, overlay_t* ov)
{
	overlay_set_mode(ov, EM8300_OVERLAY_MODE_OVERLAY);
	return TRUE;
}

static gboolean
rectangle_mode_cb(GtkMenuItem *item, overlay_t* ov)
{
	overlay_set_mode(ov, EM8300_OVERLAY_MODE_RECTANGLE);
	return TRUE;
}

static gboolean
monitor_cb(GtkMenuItem *item, dxr3view_globals *g)
{
	if(g->monitor_xoff < 1)
		g->monitor_xoff = g->scr_wid;
	else
		g->monitor_xoff = 0; 
	resize_overlay(g->ov, g->width, (int)(g->width/g->ratio),
		       (int)g->xpos+g->monitor_xoff, (int)g->ypos);
	return TRUE;
}

static gboolean
ratio_cb(GtkMenuItem *item, dxr3view_globals *g)
{
	g->ratiotoggle = (g->ratiotoggle + 1) % g->ratiolast;
	g->ratio = g->ratiolist[g->ratiotoggle];
	gtk_spin_button_set_value(g->aj->ratio, g->ratio);
	if (g->fullscreen) {
		g->ypos = (g->scr_hei-(g->scr_wid/g->ratio))/2;
	}
	gdk_window_move_resize(g->window->window, g->xpos, g->ypos,
			       g->width, (g->width/g->ratio)-10);
	resize_overlay(g->ov, g->width, (int)(g->width/g->ratio),
		       (int)g->xpos, (int)g->ypos);
	return TRUE;
}
 
gboolean
fullscreen_cb(GtkMenuItem *item, dxr3view_globals *g)
{
	if (!g->fullscreen) {
		g->oldw = g->width;
		g->oldx = g->xpos;
		g->oldy = g->ypos;
		
		g->width = g->scr_wid;
		g->xpos = g->monitor_xoff;
		g->ypos = (g->scr_hei-(g->scr_wid/g->ratio))/2;
		gdk_window_move_resize(g->window->window, g->xpos, g->ypos,
				       g->width, (g->width/g->ratio)-10);
		rectangle_mode_cb(NULL,g->ov);
		overlay_signalmode(g->ov,EM8300_OVERLAY_SIGNAL_ONLY );
		g->fullscreen = TRUE;
	} else {
		g->width = g->oldw;
		g->xpos = g->oldx;
		g->ypos = g->oldy;
		gdk_window_move_resize(g->window->window, g->xpos, g->ypos,
				       g->width, (g->width/g->ratio)-10);
		overlay_mode_cb(NULL,g->ov);
		overlay_signalmode(g->ov,EM8300_OVERLAY_SIGNAL_WITH_VGA );
		g->fullscreen = FALSE;
	}
	
	resize_overlay(g->ov, g->width, (int)(g->width/g->ratio),
		       (int)g->xpos, (int)g->ypos);
 	return TRUE;
}

static gboolean
expose_cb(GtkWidget *drawing_area,
	  GdkEventExpose *event,
	  dxr3view_globals *g) 
{
	int h,d;
	gdk_window_set_background(g->area->window, &g->overlay_color);
	gdk_window_clear(g->area->window);

	gdk_window_get_geometry(event->window, &g->xpos, &g->ypos, &g->width, &h, &d);
	gdk_window_get_origin(event->window, &g->xpos, &g->ypos);
	resize_overlay(g->ov, g->width, h, g->xpos, g->ypos);

	if (g->fullscreen)
		return TRUE;

	g->oldx = g->xpos;
	g->oldy = g->ypos;
	g->oldw = g->width;
		
	return TRUE;
}

gboolean
event_cb(GtkWidget *window, GdkEventConfigure *event, dxr3view_globals *g)
 {
	 if(g->fullscreen)
		 return FALSE;	

	 g->xpos = (((event->x) + g->monitor_xoff));
	 g->ypos = ((event->y));
	 g->width = (event->width);
	 resize_overlay(g->ov, (int)g->width, (int)(g->width/g->ratio),
			(int)g->xpos, (int)g->ypos);
	 
	 g->oldx = g->xpos;
	 g->oldy = g->ypos;
	 g->oldw = g->width;
	 
	 return TRUE;
}

static gint
popup_cb(GtkWidget *widget, GdkEventButton *event, menu_t* menu)
{
	printf("%d\n",event->type);
	if(event->type == GDK_BUTTON_PRESS) {
 		gtk_widget_show_all(GTK_WIDGET(menu->popup_menu));
		gtk_menu_popup (GTK_MENU(menu->popup_menu), NULL, NULL, NULL, NULL, 
				event->button, event->time);
	        return TRUE;
	}
	return FALSE;
}
     
static gboolean
key_cb(GtkWidget *window, GdkEventKey *kevent, dxr3view_globals *g)
{
	switch (kevent->keyval)	{
	case FULLSCREEN:  		// Fullscreen
		fullscreen_cb(NULL, g);
		break;
	case RATIOSELECT:    //Change Ratio
	case RATIOSELECTLOW:
		ratio_cb(NULL, g);
		break;
	case GDK_t:		//Popup Menu
		gtk_widget_show_all(GTK_WIDGET(g->menu->popup_menu));
		gtk_menu_popup (GTK_MENU(g->menu->popup_menu), NULL, NULL, NULL, NULL, 
				0, kevent->time);
 		break;
	case QUIT:
		gtk_main_quit();
		break;
	default:
		break;
	}
 	return TRUE;
}
