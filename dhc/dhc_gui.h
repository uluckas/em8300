#ifndef __GUI__
#define __GUI__

#include <gtk/gtk.h>
#define NR_BUTTONS 8

gint bcs_x, bcs_y;
extern GtkWidget *window;
GtkWidget *bcs_window;

int gui_setpanel ();
GdkPixmap *panel;

void gui_loadpixs();
void gui_bcs();
int arrow_width;

struct gdkpix {
	GdkPixmap *xpm;
	GdkBitmap *mask;
};

struct btn {
	struct gdkpix on;
	struct gdkpix off;
};

struct btn aspect_btns [3], tvmode_btns [3], audio_btn, spu_btn, bcs_btn;

GtkWidget *gui_button (struct gdkpix pix, char *event, gint (*call) (GtkWidget *, GdkEvent *, gpointer), gpointer data, GtkWidget **disp );

GtkWidget *d_aspect_btns[3], *d_tvmode_btns[3], *d_audio_btn, *d_spu_btn, *d_bcs_btn;

struct gdkpix gui_mkpix (char *xpm[]);
void redraw ();

typedef struct _slider 
{
	GtkWidget *pix, *fix, *ebox_up, *ebox_down, *arrow_up, *arrow_down;
	int lower, upper, *value, xpos;
} slider;

slider  *slider_new (GtkWidget *window, char *xpm[], int *value);
#endif
