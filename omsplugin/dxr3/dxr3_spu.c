
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
#include <oms/plugin/output_video.h>

#include "dxr3-api.h"

static int _dxr3_spu_open (void *this, void *name);
static int _dxr3_spu_close(void *this);
static int _dxr3_spu_read (void *this, buf_t *buf, buf_entry_t *buf_entry);
static int _dxr3_spu_ctrl (void *this, uint ctrl_id, ...);

#if 0
static plugin_codec_t dxr3_spu = {
#endif
plugin_codec_t dxr3_spu = {
  open:           _dxr3_spu_open,
  close:          _dxr3_spu_close,
  read:           _dxr3_spu_read,
  ctrl:           _dxr3_spu_ctrl
};

static int _dxr3_spu_open (void *this, void *name)
{
#if 0
  // open spu device
  if ((dxr3_spu = open("/dev/em8300_sp", O_WRONLY)) < 0) {
    perror("/dev/em8300_sp");
    return -1;
  }
#endif
  if (dxr3_get_status() == DXR3_STATUS_CLOSED) {
    dxr3_open("/dev/em8300", "/etc/dxr3.ux");
  }
  LOG(LOG_INFO, "dxr3_spu_open");
  return 0;
}

static int _dxr3_spu_close(void *this)
{
#if 0
  close(dxr3_spu);
#endif
  return 0;
}

static int _dxr3_spu_read (void *this, buf_t *buf, buf_entry_t *buf_entry)
{
  LOG(LOG_INFO, "dxr3_spu_read");
  if(buf_entry->flags & BUF_FLAG_PTS_VALID) {
    dxr3_subpic_set_pts(buf_entry->pts);
    LOG(LOG_INFO, "ptsvalid");
  }
  dxr3_subpic_write(buf_entry->data,buf_entry->data_len);
  return 0;
}

static int _dxr3_spu_ctrl (void *plugin, uint ctrl_id, ...)
{
  va_list arg_list;

  LOG(LOG_INFO, "dxr3_spu_ctrl");
  va_start(arg_list, ctrl_id);
  switch (ctrl_id) {
  case CTRL_SPU_SET_CLUT: {
    clut_t *clut = va_arg (arg_list, clut_t *);
    LOG(LOG_INFO, "--------------- CLUTi %p", clut);
    dxr3_subpic_set_palette((char*) clut);
    break;
  }
  default:
    va_end (arg_list);
    return -1;
  }
  
  va_end (arg_list);
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
		  PLUGIN_ID_CODEC_SPU,
		  "spu",
		  NULL,
		  NULL,
		  &codec_dxr3_spu);
  return 0;
}

/**
 * Cleanup Plugin.
 **/

void plugin_exit (void)
{
}
#endif
