/* 
 * dgc - a control panel for the DXR3/H+ cards.
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
#include <glib.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <linux/em8300.h>

#include <errno.h>
#include "dhc_callback.h"
#include "dhc_gui.h"

#define EM_DEV "/dev/em8300-0"
#define VER "0.2 Beta"

int dev;
GtkWidget *window;

int main (int argc, char *argv[])
{

	gtk_init(&argc, &argv);

	// Open em8300 device
	if((dev=open(EM_DEV, O_WRONLY))==-1)
	{
		const gchar *errstr = g_strerror(errno);
		perror("Could not open " EM_DEV " for writing");
		GtkWidget *dialog = gtk_message_dialog_new(
			NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			"Could not open EM8300 device for writing.\n\n"
			"Make sure the hardware is present, modules are "
			"loaded, and you have write permissions to the "
			"device.\n\nThe error was: " EM_DEV ": %s", errstr);
		gtk_window_set_title(GTK_WINDOW(dialog),
			"DHC: Exiting with error");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		_exit(-1);
	}

	cur_state.tvmode=0;
	cur_state.aspect=0;
	cur_state.audio=0;
	cur_state.spu=1;
	cur_state.bcs=0;

	if (ioctl(dev, EM8300_IOCTL_GETBCS, &bcs)==-1)
	{
		const gchar *errstr = g_strerror(errno);
		perror("Could not get B/C/S values, exiting");
		GtkWidget *dialog = gtk_message_dialog_new(
			NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			"Could not get EM8300 brightness/contrast/saturation "
			"values.\n\nThe error was: %s", errstr);
		gtk_window_set_title(GTK_WINDOW(dialog),
			"DHC: Exiting with error");
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		_exit(1);
	}
	
	pthread_mutex_init(&mut, NULL);
	pthread_cond_init(&cond, NULL);
		      
	gui_setpanel();

	pthread_create(&bcs_th, NULL, &bcs_thread, NULL);

	gtk_main();
	return (0);
}
