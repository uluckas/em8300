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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * The author may be reached as zeevm@siglab.technion.ac.il
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <glib.h>
#include <sys/ioctl.h>
#include <linux/em8300.h>

#include <errno.h>
#include "dhc_gui.h"
#include "dhc_callback.h"

#include <pthread.h>
 
int x, y, sx, sy, lastx, pos;
GtkWidget *move=0;

gint slider_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GdkModifierType modmask;
	move=0;
	gdk_window_get_pointer (NULL, &sx, &sy, &modmask);
	lastx=sx;
	pos=((slider *)data)->xpos;
	return (TRUE);
}

void  *bcs_thread (void *arg)
{
	while (1)
	{
		pthread_mutex_lock(&mut);
		pthread_cond_wait(&cond, &mut);
		if (ioctl(dev, EM8300_IOCTL_SETBCS, &bcs)==-1)
			perror("Failed setting BCS...exitting");
		pthread_mutex_unlock(&mut);
	}
	return NULL;
}

gint slider_move (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	gint mx, my;
	GdkModifierType modmask;
	gdk_window_get_pointer (NULL, &mx, &my, &modmask);
	
	if (lastx==mx || pos+(mx-sx) < -(arrow_width/2) || pos+(mx-sx) > 240-arrow_width/2) return (TRUE);
	lastx=mx;
	gtk_fixed_move(GTK_FIXED( ((slider *)data)->fix), ((slider *)data)->ebox_down, pos+(mx-sx), 0);
	gtk_fixed_move(GTK_FIXED( ((slider *)data)->fix), ((slider *)data)->ebox_up, pos+(mx-sx), 13);
	*((slider *)data)->value=((float)pos+(mx-sx)+(arrow_width/2))/240*1000;

	pthread_mutex_lock(&mut);
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mut);

	return (TRUE);
}


gint slider_release (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	gint mx, my;
	GdkModifierType modmask;
	gdk_window_get_pointer (NULL, &mx, &my, &modmask);
	if (pos+(mx-sx) < -(arrow_width/2)) ((slider *)data)->xpos=-(arrow_width/2);
        else if (pos+(mx-sx) > 240-arrow_width/2) ((slider *)data)->xpos=240-arrow_width/2;
	else ((slider *)data)->xpos+=(mx-sx);
	return (TRUE);
}

gint client_e (GtkWidget *widget, GdkEventClient *event, gpointer data)
{
	gtk_signal_emit_stop_by_name( GTK_OBJECT(widget), "client_event");
	return (TRUE);
}

// Toggle BCS window

gint bcs_toggle (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gint x, y, i;
	move=0;
	if (!cur_state.bcs) 
	{
		gtk_pixmap_set(GTK_PIXMAP(d_bcs_btn), bcs_btn.on.xpm, bcs_btn.on.mask);
		gdk_window_get_position(window->window, &x, &y);
		gtk_widget_set_uposition(bcs_window, x+75, y);
		gtk_widget_show(bcs_window);
		gdk_window_raise(window->window);
		for (i=0; i<84; i+=2) gtk_widget_set_uposition(bcs_window, x+75, y-i);

	}
	else 
	{
		gtk_pixmap_set(GTK_PIXMAP(d_bcs_btn), bcs_btn.off.xpm, bcs_btn.off.mask);
		gdk_window_get_position(bcs_window->window, &x, &y);
		gdk_window_raise(window->window);
		for (i=0; i<90; i+=2) gtk_widget_set_uposition(bcs_window, x, y+i);
		gtk_widget_hide(bcs_window);
	}
	cur_state.bcs = !cur_state.bcs;
	return (TRUE);
}
	
		
gint press (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	move=widget;
	gdk_window_raise(widget->window);
	gdk_window_raise(bcs_window->window);
	x=event->x;
	y=event->y;

	return (TRUE);
}

gint motion (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
	gint mx, my;
	GdkModifierType modmask;
	if (!move) return (TRUE);
	gdk_window_get_pointer (NULL, &mx, &my, &modmask);

	gdk_window_move( widget->window, mx-x, my-y );
	gdk_window_move( bcs_window->window, mx-x+75, my-y-82 );

	return (TRUE);
}


gint delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	if (widget==bcs_window) return (TRUE);
	close(dev);
	gtk_main_quit();
	return (FALSE);
}

// Set audio mode
gint set_audio (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	int next=!cur_state.audio;
	move=0;
	if(ioctl(dev, EM8300_IOCTL_SET_AUDIOMODE,&next) == -1)
		perror("Error doin ioctl");
	if (!cur_state.audio) gtk_pixmap_set(GTK_PIXMAP(d_audio_btn), audio_btn.on.xpm, audio_btn.on.mask);
	else gtk_pixmap_set(GTK_PIXMAP(d_audio_btn), audio_btn.off.xpm, audio_btn.off.mask);
	cur_state.audio = !cur_state.audio;
	return (TRUE);
}

// Set TV mode
gint set_tvmode (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	move=0;
	if ((int)data==cur_state.tvmode) return (TRUE);
	if(ioctl(dev, EM8300_IOCTL_SET_VIDEOMODE,&data) == -1)
		perror("Error doin ioctl");

	gtk_pixmap_set(GTK_PIXMAP(d_tvmode_btns[(int)data]), tvmode_btns[(int)data].on.xpm, tvmode_btns[(int)data].on.mask);
	gtk_pixmap_set(GTK_PIXMAP(d_tvmode_btns[cur_state.tvmode]), tvmode_btns[cur_state.tvmode].off.xpm, tvmode_btns[cur_state.tvmode].off.mask);
	cur_state.tvmode=(int)data;
	return (TRUE);

}

// Set aspect ratio
gint set_aspect (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	move=0;
	if ((int)data==cur_state.aspect) return (TRUE);
	if(ioctl(dev, EM8300_IOCTL_SET_ASPECTRATIO,&data) == -1)
		perror("Error doin ioctl");
	gtk_pixmap_set(GTK_PIXMAP(d_aspect_btns[(int)data]), aspect_btns[(int)data].on.xpm, aspect_btns[(int)data].on.mask);
	gtk_pixmap_set(GTK_PIXMAP(d_aspect_btns[cur_state.aspect]), aspect_btns[cur_state.aspect].off.xpm, aspect_btns[cur_state.aspect].off.mask);
	cur_state.aspect=(int)data;
	return (TRUE);
			
}

// Set sub-picture
gint set_spu (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	int next=!cur_state.spu;
	move=0;
	if(ioctl(dev, EM8300_IOCTL_SET_SPUMODE,&next) == -1)
		perror("Error doin ioctl");
	if (!cur_state.spu) gtk_pixmap_set(GTK_PIXMAP(d_spu_btn), spu_btn.on.xpm, spu_btn.on.mask);
	else gtk_pixmap_set(GTK_PIXMAP(d_spu_btn), spu_btn.off.xpm, spu_btn.off.mask);
	cur_state.spu = !cur_state.spu;
	return (TRUE);
			
}

gint destroy (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	close(dev);
	gtk_main_quit();
	return (TRUE);
}
