
/*
 *
 * Copyright (C) 1999, 2000  Thomas Mirlacher
 *
 * 07-10-00: Rick Haines - oms plugin api changed..
 *
 * 18-09-00: Ze'ev Maor - added auto setting of audio output mode
 *
 * 24-08-00: Ze'ev Maor - SPDIF framing of AC3 sync frames integration
 * 	     zeevm@siglab.technion.ac.il			
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * The author may be reached as dent@cosy.sbg.ac.at, or
 * Thomas Mirlacher, Jakob-Haringerstr. 2, A-5020 Salzburg,
 * Austria
 *
 *------------------------------------------------------------
 *
 */


//#define TOAST_SPU
//#define TOAST_SYNC 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <oms/oms.h>
#include <oms/plugin/codec.h>

#include "dxr3-api.h"

static int _dxr3_init(char *devname, char *ucodefile);

static int _dxr3_video_open (void *this, void *name);
static int _dxr3_video_close(void *this);
static int _dxr3_video_read (void *this, buf_t *buf, buf_entry_t *buf_entry);
static int _dxr3_video_ctrl (void *this, uint flag, ...);
static int _dxr3_video_set_status (void *this, u_int status);
static int _dxr3_video_set_attributes (void *this, u_int id, void *attr);

#if 0
static plugin_codec_t dxr3_mpeg2 = {
#endif
plugin_codec_t dxr3_mpeg2 = {
  open:           _dxr3_video_open,
  close:          _dxr3_video_close,
  read:           _dxr3_video_read,
  ctrl:           _dxr3_video_ctrl,
  set_status:     _dxr3_video_set_status,
  set_attributes: _dxr3_video_set_attributes,
};

static int _dxr3_init(char *devname, char *ucodefile)
{
  LOG(LOG_INFO, "_dxr3_init(devname=%s)", devname);

  if (dxr3_get_status() == DXR3_STATUS_CLOSED) {
    dxr3_open("/dev/em8300", "/etc/dxr3.ux");
  }
  return 0;
}

static int _dxr3_video_open (void *this, void *name)
{
  if(_dxr3_init("/dev/em8300","/etc/dxr3.ux")) {
    LOG(LOG_ERROR, "Can't open dxr3 driver");
    return -1;
  }
  LOG(LOG_INFO, "dxr3_mpeg2");
#if 0
  // open video device
  if ((dxr3_video = open("/dev/em8300_mv", O_WRONLY)) < 0) {
    perror("/dev/em8300_mv");
    return -1;
  }
#endif
  return 0;
}

static int _dxr3_video_close(void *this)
{
  LOG(LOG_INFO, "dxr3_video_close");
  if (dxr3_get_status() == DXR3_STATUS_OPENED)
    dxr3_close();
  return 0;
}

static int _dxr3_video_read (void *this, buf_t *buf, buf_entry_t *buf_entry)
{
  if(buf_entry->flags & BUF_FLAG_PTS_VALID) 
    dxr3_video_set_pts(buf_entry->pts);
  dxr3_video_write(buf_entry->data, buf_entry->data_len);
  return 0;
}

static int _dxr3_video_ctrl (void *this, uint ctrl_id, ...)
{
  va_list arg_list;
  
  va_start (arg_list, ctrl_id);
  switch (ctrl_id) {
  case CTRL_VIDEO_INITIALIZED: {
    int val = va_arg (arg_list, int);
    val = 0;
    LOG(LOG_INFO, "ctrl_video_initialized");
#if 0
    mpeg2_output_init (&mpeg2dec, val);
#endif
    break;
  }
  case CTRL_VIDEO_DROP_FRAME: {
    int val = va_arg (arg_list, int);
    val = 0;
#if 0
    LOG(LOG_INFO, "ctrl_video_drop_frame");
#endif
#if 0
    fprintf(stderr, "%c", val ? '-':'+');
    mpeg2_drop (&mpeg2dec, val);
#endif
    break;
  }
  default:
    va_end (arg_list);
    return -1;
  }
  
  va_end (arg_list);
  return 0;
}

static int _dxr3_video_set_status (void *this, u_int status)
{
    LOG(LOG_INFO, "dxr3 video set status");
    switch (status) {
    case STATUS_PLAY:
	dxr3_set_playmode(DXR3_PLAYMODE_PLAY);
	break;
    case STATUS_PAUSE:
	dxr3_set_playmode(DXR3_PLAYMODE_PAUSED);
	break;
    case STATUS_STOP:
	dxr3_set_playmode(DXR3_PLAYMODE_STOPPED);
	break;
    default:
        LOG(LOG_ERROR, "DecoderSetStatus: unhandled status %d", status);
	return -1;
    }
    return 0;
}

static int _dxr3_video_set_attributes (void *this, u_int id, void *att)
{
    plugin_codec_attr_video_t *attr = att;
    LOG(LOG_INFO, "dxr3 video set attributes");
    if(attr->aspect_output != -1) {
        switch(attr->aspect_output) {
	case 0:
	    dxr3_video_set_aspectratio(EM8300_ASPECTRATIO_3_2);
	    break;
	case 3:
	    dxr3_video_set_aspectratio(EM8300_ASPECTRATIO_16_9);
	    break;
	}
    }
    if(attr->tv_system != -1) {
        switch(attr->tv_system) {
	case 0:
	    dxr3_video_set_tvmode(EM8300_VIDEOMODE_NTSC);
	    break;
	case 2:
	    dxr3_video_set_tvmode(EM8300_VIDEOMODE_PAL);
	    break;
	case 7:
	    dxr3_video_set_tvmode(EM8300_VIDEOMODE_PAL60);
	    break;
	}
    }
    return 0;
}

#if 0
/**
 * Initialize Plugin.
 **/

int plugin_init (char *whoami)
{
  fprintf(stderr, "plugin_init\n");
  pluginRegister (whoami,
		  PLUGIN_ID_CODEC_VIDEO,
		  "mpg2",
		  NULL,
		  NULL,
		  &codec_dxr3_video);
  return 0;
}

/**
 * Cleanup Plugin.
 **/

void plugin_exit (void)
{
}
#endif
