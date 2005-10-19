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
		perror("Error opening em8300");
		_exit(-1);
	}

	cur_state.tvmode=0;
	cur_state.aspect=0;
	cur_state.audio=0;
	cur_state.spu=1;
	cur_state.bcs=0;

	if (ioctl(dev, EM8300_IOCTL_GETBCS, &bcs)==-1)
	{
		perror("Failed getting BCS values...exiting");
		_exit(1);
	}
	
	pthread_mutex_init(&mut, NULL);
	pthread_cond_init(&cond, NULL);
		      
	gui_setpanel();

	pthread_create(&bcs_th, NULL, &bcs_thread, NULL);

	gtk_main();
	return (0);
}
