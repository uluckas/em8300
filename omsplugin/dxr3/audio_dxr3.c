//PLUGIN_INFO(INFO_NAME, "audio driver output");
//PLUGIN_INFO(INFO_AUTHOR, "Aaron Holtzman <aholtzma@ess.engr.uvic.ca>");

/*
 *
 *  audio_out_sys.c
 *    
 *	Copyright (C) Aaron Holtzman - May 1999
 *	Port to IRIX by Jim Miller, SGI - Nov 1999
 *	Port to output plugin/unified support for linux/solaris/...
 *		by Thome Mirlacher - Jul 2000
 *
 *  This file is part of ac3dec, a free Dolby AC-3 stream decoder.
 *	
 *  ac3dec is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  ac3dec is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *
 */


#ifdef HAVE_CONFIG_H
#	include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <sys/ioctl.h>

#include <oms/oms.h>
#include <oms/plugin/output_audio.h>

#include "dxr3-api.h"

static int _dxr3_init(char *devname, char *ucodefile);

static int _audio_dxr3_open (void *plugin, void *name);
static int _audio_dxr3_close (void *plugin);
static int _audio_dxr3_setup (plugin_output_audio_attr_t *attr);
static int _audio_dxr3_write (const void *buf, size_t num_bytes);
static int _audio_dxr3_check (void);

#if 0
static plugin_output_audio_t audio_dxr3 = {
#endif
plugin_output_audio_t audio_dxr3 = {
        open:		_audio_dxr3_open,
        close:		_audio_dxr3_close,
        setup:		_audio_dxr3_setup,
        write:		_audio_dxr3_write,
	check:		_audio_dxr3_check
};

static int _dxr3_init(char *devname, char *ucodefile)
{
  LOG(LOG_INFO, "_dxr3_init(devname=%s)", devname);

  if (dxr3_get_status() == DXR3_STATUS_CLOSED) {
    dxr3_open("/dev/em8300", "/etc/dxr3.ux");
  }

  return 0;
}

/**
 * open device
 **/

static int _audio_dxr3_open (void *plugin, void *name)
{
  if(_dxr3_init("/dev/em8300","/etc/dxr3.ux")) {
    LOG(LOG_ERROR, "Can't open dxr3 driver");
    return -1;
  }
  printf("audio_dxr3\n");
#if 0
// Open the device driver
  if ((fd = open ((char *) name, O_WRONLY)) < 0) {
    LOG(LOG_ERROR, "%s: Opening audio device %s", strerror(errno), (char *) name);
    return -1;
  }
  LOG(LOG_INFO, "Opened audio device \"%s\"", (char *) name);
#endif  
  return 0;
}


/**
 *
 **/

static int _audio_dxr3_close (void *plugin)
{
#if 0
        close(dxr3_control);
        return close (fd);
#endif
  LOG(LOG_INFO, "_audio_dxr3_close");
  if (dxr3_get_status() == DXR3_STATUS_OPENED)
    dxr3_close();
  return 0;
}


/**
 * setup audio device
 **/

static int _audio_dxr3_setup (plugin_output_audio_attr_t *attr)
{
  /* dxr3_mpeg2 plugin must be loaded already (to upload microcode) */
  LOG(LOG_INFO, "_audio_dxr3_setup");
  LOG(LOG_INFO, "channels: %i\tsamplesize: %i\trate: %i", attr->channels, attr->format, attr->speed);
  dxr3_audio_set_stereo(attr->channels == 2 ? 1 : 0);
  dxr3_audio_set_samplesize(attr->format);
  dxr3_audio_set_rate(attr->speed);
  return 0;
}


/**
 * play the sample to the already opened file descriptor
 **/

static int _audio_dxr3_write (const void *buf, size_t num_bytes)
{
  return dxr3_audio_write(buf, num_bytes);
}

static int _audio_dxr3_check (void)
{
  return 0;
}

#if 0
/**
 * Initialize Plugin.
 **/

int plugin_init (char *whoami)
{
        pluginRegister (whoami,
                PLUGIN_ID_OUTPUT_AUDIO,
                "dxr3",
		NULL,
		NULL,
                &audio_sys);

        return 0;
}


/**
 * Cleanup Plugin.
 **/

void plugin_exit (void)
{
}
#endif
