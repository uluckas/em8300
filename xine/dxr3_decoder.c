/* 
 * Copyright (C) 2000-2001 the xine project
 * 
 * This file is part of xine, a unix video player.
 * 
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id$
 *
 * This dxr3 decoder plugin replaces the libmpeg2 and the
 * libspudec plugin
 * TODO: decoder priorities
 */


#include <stdlib.h>
#include <string.h>

#include <buffer.h>
#include <xine_internal.h>
#include <linux/soundcard.h>
#include <linux/em8300.h>

#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <inttypes.h>

char devname[]="/dev/em8300";

typedef struct dxr3_decoder_s {
	video_decoder_t video_decoder;
	vo_instance_t *video_out;

	int fd_video;
	int fd_control;
	int tv_mode;
	int last_pts;
} dxr3_decoder_t;

static int dxr3_can_handle (video_decoder_t *this_gen, int buf_type)
{
	return ((buf_type & 0xFFFF0000) == BUF_VIDEO_MPEG) ;
}

static void dxr3_init (video_decoder_t *this_gen, vo_instance_t *video_out)
{
	dxr3_decoder_t *this = (dxr3_decoder_t *) this_gen;
	char tmpstr[100];

	printf("dxr3: Entering video init, devname=%s.\n",devname);
    
	/* open control device */
	if ((this->fd_control = open(devname, O_WRONLY)) < 0) {
		fprintf(stderr, "dxr3: Failed to open control device %s (%s)\n",
		 devname, strerror(errno));
		return;
	}

	/* open video device */
	snprintf (tmpstr, sizeof(tmpstr), "%s_mv", devname);
	if ((this->fd_video = open (tmpstr, O_WRONLY)) < 0) {
		fprintf(stderr, "dxr3: Failed to open video device %s (%s)\n",
		 tmpstr, strerror(errno));
		return;
	}

	/* set video mode
	 * TODO: configurable
	 */
#if 0
	this->tv_mode = EM8300_VIDEOMODE_PAL;
	if (ioctl(this->fd_control, EM8300_IOCTL_SET_VIDEOMODE, &this->tv_mode))
		fprintf(stderr, "dxr3: setting video mode failed.");
#endif
	video_out->open(video_out);
	this->video_out = video_out;

	this->last_pts = 0;
}

static void dxr3_decode_data (video_decoder_t *this_gen, buf_element_t *buf)
{
	dxr3_decoder_t *this = (dxr3_decoder_t *) this_gen;
	ssize_t written;

	/* The dxr3 does not need the preview-data */
	if (buf->decoder_info[0] == 0) return;

	if (buf->PTS) {
		int vpts,pts;
		pts = this->video_decoder.metronom->get_current_time
		 (this->video_decoder.metronom);
		vpts = this->video_decoder.metronom->got_video_frame
		 (this->video_decoder.metronom, buf->PTS);

//		fprintf(stderr, "pts %d %d %d\n",vpts,pts,buf->PTS);

		if (vpts > pts && this->last_pts < vpts)
		{
			this->last_pts = vpts;
			if (ioctl(this->fd_video,EM8300_IOCTL_VIDEO_SETSCR,&pts))
				fprintf(stderr, "dxr3: set master pts failed.\n");

			if (ioctl(this->fd_video, EM8300_IOCTL_VIDEO_SETPTS, &vpts))
				fprintf(stderr, "dxr3: set video pts failed.\n");
		}
	}

	written = write(this->fd_video, buf->content, buf->size);
	if (written < 0) {
		fprintf(stderr, "dxr3: video device write failed (%s)\n",
		 strerror(errno));
		return;
	}
	if (written != buf->size)
		fprintf(stderr, "dxr3: Could only write %d of %d video bytes.\n",
		 written, buf->size);
}

static void dxr3_close (video_decoder_t *this_gen)
{
	dxr3_decoder_t *this = (dxr3_decoder_t *) this_gen;

	ioctl(this->fd_control, EM8300_IOCTL_SET_PLAYMODE, EM8300_PLAYMODE_STOPPED);
	close(this->fd_control);
	close(this->fd_video);

	this->video_out->close(this->video_out);
}

static char *dxr3_get_id(void) {
  return "dxr3-mpeg2";
}

video_decoder_t *init_video_decoder_plugin (int iface_version,
 config_values_t *cfg)
{
	dxr3_decoder_t *this ;

	if (iface_version != 1)
	return NULL;

	this = (dxr3_decoder_t *) malloc (sizeof (dxr3_decoder_t));

	this->video_decoder.interface_version   = 1;
	this->video_decoder.can_handle          = dxr3_can_handle;
	this->video_decoder.init                = dxr3_init;
	this->video_decoder.decode_data         = dxr3_decode_data;
	this->video_decoder.close               = dxr3_close;
	this->video_decoder.get_identifier      = dxr3_get_id;

	return (video_decoder_t *) this;
}

/*
 * Second part of the dxr3 plugin: subpicture decoder
 */

typedef struct spudec_decoder_s {
	spu_decoder_t    spu_decoder;

	vo_instance_t   *vo_out;
	int fd_spu;
} spudec_decoder_t;

static int spudec_can_handle (spu_decoder_t *this_gen, int buf_type)
{
  return ((buf_type & 0xFFFF0000) == BUF_SPU_PACKAGE);
}

static void spudec_init (spu_decoder_t *this_gen, vo_instance_t *vo_out)
{
	spudec_decoder_t *this = (spudec_decoder_t *) this_gen;
	char tmpstr[100];

	this->vo_out = vo_out;

	/* open spu device */
	snprintf (tmpstr, sizeof(tmpstr), "%s_sp", devname);
	if ((this->fd_spu = open (tmpstr, O_WRONLY)) < 0) {
		fprintf(stderr, "dxr3: Failed to open spu device %s (%s)\n",
		 tmpstr, strerror(errno));
		return;
	}

}

static void spudec_decode_data (spu_decoder_t *this_gen, buf_element_t *buf)
{
	spudec_decoder_t *this = (spudec_decoder_t *) this_gen;
	ssize_t written;

	/* Is this also needed for subpicture? */
	if (buf->decoder_info[0] == 0) return;

	if (buf->PTS) {
		int vpts;
		vpts = this->spu_decoder.metronom->got_spu_packet
		 (this->spu_decoder.metronom, buf->PTS, 0);

//		fprintf(stderr, "spu pts %d %d\n",vpts,buf->PTS);
		if (ioctl(this->fd_spu, EM8300_IOCTL_SPU_SETPTS, &vpts))
			fprintf(stderr, "spu write failed\n");
	}

	written = write(this->fd_spu, buf->content, buf->size);
	if (written < 0) {
		fprintf(stderr, "dxr3: spu device write failed (%s)\n",
		 strerror(errno));
		return;
	}
	if (written != buf->size)
		fprintf(stderr, "dxr3: Could only write %d of %d spu bytes.\n",
		 written, buf->size);
}

static void spudec_close (spu_decoder_t *this_gen)
{
	spudec_decoder_t *this = (spudec_decoder_t *) this_gen;
	
	close(this->fd_spu);
}

static char *spudec_get_id(void)
{
	return "dxr3-spudec";
}

spu_decoder_t *init_spu_decoder_plugin (int iface_version,
 config_values_t *cfg)
{
  spudec_decoder_t *this;

  if (iface_version != 1)
    return NULL;

  this = (spudec_decoder_t *) malloc (sizeof (spudec_decoder_t));

  this->spu_decoder.interface_version   = 1;
  this->spu_decoder.can_handle          = spudec_can_handle;
  this->spu_decoder.init                = spudec_init;
  this->spu_decoder.decode_data         = spudec_decode_data;
  this->spu_decoder.close               = spudec_close;
  this->spu_decoder.get_identifier      = spudec_get_id;
  
  return (spu_decoder_t *) this;
}

