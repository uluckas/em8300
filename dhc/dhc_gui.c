/* 
 * dhc - a control panel for the DXR3/H+ cards.
 *
 * Copyright (C) 2000 Ze'ev Maor <zeevm@siglab.technion.ac.il>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * The author may be reached as zeevm@siglab.technion.ac.il
 *
 */

#include <sys/ioctl.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <linux/em8300.h>
#include "dhc_gui.h"
#include "dhc_callback.h"
#include "pixmaps.h"

void gui_loadpixs ()
{

	aspect_btns[0].on=gui_mkpix(a4_3_1_xpm);
	aspect_btns[0].off=gui_mkpix(a4_3_0_xpm);
	aspect_btns[1].on=gui_mkpix(a16_9_1_xpm);
	aspect_btns[1].off=gui_mkpix(a16_9_0_xpm);
	aspect_btns[2].on=gui_mkpix(a235_1_1_xpm);
	aspect_btns[2].off=gui_mkpix(a235_1_0_xpm);

	tvmode_btns[0].on=gui_mkpix(pal_1_xpm);
	tvmode_btns[0].off=gui_mkpix(pal_0_xpm);
	tvmode_btns[1].on=gui_mkpix(pal60_1_xpm);
	tvmode_btns[1].off=gui_mkpix(pal60_0_xpm);
	tvmode_btns[2].on=gui_mkpix(ntsc_1_xpm);
	tvmode_btns[2].off=gui_mkpix(ntsc_0_xpm);
	
	audio_btn.on=gui_mkpix(digital_xpm);
	audio_btn.off=gui_mkpix(analog_xpm);

	spu_btn.on=gui_mkpix(spu_1_xpm);
	spu_btn.off=gui_mkpix(spu_0_xpm);

	bcs_btn.on=gui_mkpix(bcs_1_xpm);
	bcs_btn.off=gui_mkpix(bcs_0_xpm);
}
	
struct gdkpix gui_mkpix (char *xpm[])
{
	struct gdkpix pix;
	GdkBitmap *mask;
	
	pix.xpm=gdk_pixmap_create_from_xpm_d (window->window, &mask, NULL, xpm);
	pix.mask=mask;
	return(pix);
}

	
GtkWidget *gui_button (struct gdkpix pix, char *event, gint (*call)(GtkWidget *, GdkEvent *, gpointer), gpointer data ,GtkWidget **disp)
{
	GtkWidget *ebox;
	
	ebox=gtk_event_box_new();
	gtk_widget_show(ebox);
	*disp=gtk_pixmap_new(pix.xpm, pix.mask);
	gtk_container_add(GTK_CONTAINER(ebox), *disp);
	gtk_widget_show(*disp);
	gtk_widget_set_events(ebox, GDK_BUTTON_PRESS_MASK);
	gtk_signal_connect(GTK_OBJECT(ebox), event, GTK_SIGNAL_FUNC(*call), data);
	gtk_widget_shape_combine_mask(ebox, pix.mask, 0, 0);
	
	return ebox;
}

	

void redraw ()
{
	gdk_window_set_back_pixmap(window->window, panel, 0);
	gdk_window_clear (window->window);
	gdk_flush ();
}

int gui_setpanel ()
{
	GtkWidget *button_pix, *table;
	GdkPixmap *icon;
	GdkBitmap *mask;
	gint wx, wy;
        struct gdkpix audio_pix;

	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	
	gtk_widget_set_events(window, gtk_widget_get_events(window) | GDK_BUTTON_PRESS_MASK | GDK_BUTTON1_MOTION_MASK);
	gtk_widget_realize(window);
	gtk_window_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	
	gtk_signal_connect(GTK_OBJECT(window), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "button_press_event", GTK_SIGNAL_FUNC(press), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "motion_notify_event", GTK_SIGNAL_FUNC(motion), NULL);
	gtk_signal_connect(GTK_OBJECT(window), "client_event", GTK_SIGNAL_FUNC (client_e), NULL);
	
	gtk_widget_set_app_paintable(window, TRUE);

	gdk_window_set_decorations(window->window,0);

	gtk_window_set_title( GTK_WINDOW (window), "DXR3 / H+  Control Panel");
	gtk_window_set_policy(GTK_WINDOW(window), 0, 0, 0);
	
	gui_loadpixs();
	
	table=gtk_table_new (5, 3, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 10);
	gtk_container_add(GTK_CONTAINER(window), table);
	gtk_widget_realize(table);

	icon=gdk_pixmap_create_from_xpm_d(window->window, &mask, NULL, dhc_xpm);
	gdk_window_set_icon (window->window, NULL, icon, mask);

	// Set main window background and shape

	panel = gdk_pixmap_create_from_xpm_d (window->window, &mask, NULL, bg_xpm);
	gdk_window_set_back_pixmap(window->window, panel, 0);
	gdk_window_get_size(panel, &wx, &wy);
	gtk_widget_shape_combine_mask(window, mask, 0, 0);
	gtk_widget_set_usize(table, wx, wy);
	gtk_widget_set_uposition(table, 15 , 15);

	// Set-up "Quit" button
	gtk_table_attach (GTK_TABLE(table), gui_button(gui_mkpix(quit_xpm), "button_press_event", &destroy, NULL, &button_pix), 
				1, 2, 4, 5, 0, 0, 0, 0);
	

	// Set-up TV mode buttons
	
	gtk_table_attach (GTK_TABLE(table), 
			gui_button(tvmode_btns[0].on, "button_press_event", &set_tvmode, (gpointer)EM8300_VIDEOMODE_PAL, &d_tvmode_btns[0])
			, 0, 1, 0, 1, 0, 0, 0, 5);
	
	gtk_table_attach (GTK_TABLE(table), 
			gui_button(tvmode_btns[1].off, "button_press_event",&set_tvmode, (gpointer)EM8300_VIDEOMODE_PAL60, &d_tvmode_btns[1])
			, 0, 1, 1, 2, 0, 0, 0, 5);

	gtk_table_attach (GTK_TABLE(table), 
			gui_button(tvmode_btns[2].off, "button_press_event", &set_tvmode, (gpointer)EM8300_VIDEOMODE_NTSC, &d_tvmode_btns[2])
			, 0, 1, 2, 3, 0, 0, 0, 5);
	 
	// Set-up aspect buttons
	
	gtk_table_attach (GTK_TABLE(table), 
			gui_button(aspect_btns[0].on, "button_press_event", &set_aspect, (gpointer)EM8300_ASPECTRATIO_4_3, &d_aspect_btns[0])
			, 2, 3, 0, 1, 0, 0, 0, 5);
	
	gtk_table_attach (GTK_TABLE(table), 
			gui_button(aspect_btns[1].off, "button_press_event",&set_aspect, (gpointer)EM8300_ASPECTRATIO_16_9, &d_aspect_btns[1])
			, 2, 3, 1, 2, 0, 0, 0, 5);
	
	gtk_table_attach (GTK_TABLE(table), 
			gui_button(aspect_btns[2].off, "button_press_event", &set_aspect, (gpointer)EM8300_ASPECTRATIO_16_9, &d_aspect_btns[2])
			, 2, 3, 2, 3, 0, 0, 0, 5);
	// Set-up audio mode button
	

	ioctl(dev, EM8300_IOCTL_GET_AUDIOMODE, &cur_state.audio);
	
	if (cur_state.audio) audio_pix=audio_btn.on;
	else audio_pix=audio_btn.off;

	gtk_table_attach (GTK_TABLE(table), gui_button(audio_pix, "button_press_event", &set_audio, NULL, &d_audio_btn), 
			  2, 3, 3, 4, 0, 0, 0, 25);

			
	// Set-up sub-picture mode button
	
			
	gtk_table_attach (GTK_TABLE(table), gui_button(spu_btn.on, "button_press_event", &set_spu, NULL, &d_spu_btn), 
			   0, 1, 3, 4, 0, 0, 0, 25);

	// Set-up BCS button
	
	gtk_table_attach (GTK_TABLE(table), gui_button(bcs_btn.off, "button_press_event", &bcs_toggle, NULL, &d_bcs_btn), 
			   1, 2, 1, 2, 0, 0, 0, 0);

	gtk_table_set_col_spacing(GTK_TABLE(table), 1, 8);

	gtk_widget_show(table);
	gtk_widget_show(window);
	gui_bcs();
	return 0;
}


void gui_bcs ()
{
	GtkWidget *box;
	slider *brightness, *contrast, *saturation;

	bcs_window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(bcs_window), "client_event", GTK_SIGNAL_FUNC (client_e), NULL);
	gtk_signal_connect(GTK_OBJECT(bcs_window), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL);
	gtk_widget_realize(bcs_window);
	gtk_window_set_title(GTK_WINDOW (bcs_window), "BCS settings");
	gdk_window_set_decorations(bcs_window->window, 0);

	brightness=slider_new(bcs_window, brightness_xpm, &bcs.brightness);
	contrast=slider_new(bcs_window, contrast_xpm, &bcs.contrast);
	saturation=slider_new(bcs_window, saturation_xpm, &bcs.saturation);

	box=gtk_vbox_new(FALSE, 10);
	gtk_container_add(GTK_CONTAINER(bcs_window), box);
	gtk_box_pack_start(GTK_BOX(box), brightness->fix, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), contrast->fix, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), saturation->fix, FALSE, FALSE, 0);
	gtk_widget_show(box);
}

slider  *slider_new (GtkWidget *window, char *xpm[], int *value)
{
	GdkPixmap *pix;
	GdkBitmap *mask;
	int x, y;
	slider *slide=(slider *)malloc(sizeof(slider));


	pix=gdk_pixmap_create_from_xpm_d(window->window, &mask, NULL, xpm);
	slide->pix=gtk_pixmap_new(pix, mask);
	gdk_window_get_size(pix, &x, &y);
	slide->fix=gtk_fixed_new();
	gtk_widget_set_usize(slide->fix, x, y);
	gtk_fixed_put(GTK_FIXED(slide->fix), slide->pix, 0, 0);

	pix=gdk_pixmap_create_from_xpm_d(window->window, &mask, NULL, up_xpm);
	gdk_window_get_size(pix, &arrow_width, &y);

	x=((float)*value/1000*x)-(arrow_width/2);
	
	slide->arrow_up=gtk_pixmap_new(pix, mask);
	slide->ebox_up=gtk_event_box_new();
	gtk_signal_connect(GTK_OBJECT(slide->ebox_up), "motion_notify_event", GTK_SIGNAL_FUNC(slider_move), (gpointer) slide);
	gtk_signal_connect(GTK_OBJECT(slide->ebox_up), "button_press_event", GTK_SIGNAL_FUNC(slider_press), (gpointer) slide);
	gtk_signal_connect(GTK_OBJECT(slide->ebox_up), "button_release_event", GTK_SIGNAL_FUNC(slider_release), (gpointer) slide);
	gtk_widget_shape_combine_mask(slide->ebox_up, mask, 0, 0);
	gtk_container_add(GTK_CONTAINER(slide->ebox_up), slide->arrow_up);
	gtk_fixed_put(GTK_FIXED(slide->fix), slide->ebox_up, x, 13);
	
	pix=gdk_pixmap_create_from_xpm_d(window->window, &mask, NULL, down_xpm);
	slide->arrow_down=gtk_pixmap_new(pix, mask);
	slide->ebox_down=gtk_event_box_new();
	gtk_signal_connect(GTK_OBJECT(slide->ebox_down), "motion_notify_event", GTK_SIGNAL_FUNC(slider_move), (gpointer) slide);
	gtk_signal_connect(GTK_OBJECT(slide->ebox_down), "button_press_event", GTK_SIGNAL_FUNC(slider_press), (gpointer) slide);
	gtk_signal_connect(GTK_OBJECT(slide->ebox_down), "button_release_event", GTK_SIGNAL_FUNC(slider_release), (gpointer) slide);
	gtk_widget_shape_combine_mask(slide->ebox_down, mask, 0, 0);
	gtk_container_add(GTK_CONTAINER(slide->ebox_down), slide->arrow_down);
	gtk_fixed_put(GTK_FIXED(slide->fix), slide->ebox_down, x, 0);
	
	
	gtk_widget_show(slide->ebox_up);
	gtk_widget_show(slide->ebox_down);
	gtk_widget_show(slide->arrow_up);
	gtk_widget_show(slide->arrow_down);
	gtk_widget_show(slide->pix);
	gtk_widget_show(slide->fix);

	slide->lower=0;
	slide->value=value;
	slide->upper=1000;
	slide->xpos=x;

	return (slide);
}

