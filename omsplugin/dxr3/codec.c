
/*
 *
 * Copyright (C) 1999, 2000  Thomas Mirlacher
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
#include <oms/plugin.h>

#include <ac3.h>
#include <mpeg2.h>

#include "dxr3-api.h"

extern ao_functions_t audio_out_dxr3;
extern dxr3_state_t state;

static struct dxr3_priv_struct {
    char *path;
} dxr3_priv;

static int _dxr3_init (plugin_codec_t *plugin, buf_t *buf, char *output_path);
static int _dxr3_open (plugin_codec_t *plugin);
static int _dxr3_read (plugin_codec_t *plugin, buf_t *buf, buf_entry_t *buf_entry);
static int _dxr3_set_status (plugin_codec_t *plugin, u_int status);
static int _dxr3_set_attributes (plugin_codec_t *plugin, u_int id, void *attr);
static int _dxr3_install_firmware (struct dxr3_priv_struct *priv, char *ucode);


static plugin_codec_t codec_dxr3 = {
    &dxr3_priv,
    _dxr3_init,	// init
    NULL,		// exit
    _dxr3_open,	// open
    NULL,		// close
    _dxr3_read,	// read
    NULL,		// write
    _dxr3_set_status,
    _dxr3_set_attributes,
}; 


static ac3_config_t ac3_config;


/**
 *
 **/

static int _dxr3_open (plugin_codec_t *plugin)
{
    struct dxr3_priv_struct *priv = plugin->priv;


    return 0;
}

/**
 *
 **/

static int _dxr3_read (plugin_codec_t *plugin, buf_t *buf, buf_entry_t *buf_entry)
{
    switch (buf_entry->buf_id) {
    case BUF_AUDIO:
	if(buf_entry->flags & BUF_FLAG_PTS_VALID) 
	    dxr3_audio_set_pts(buf_entry->pts);

	if (state.audiomode==DXR3_AUDIOMODE_DIGITALAC3)
		output_spdif(buf_entry->data,
			     buf_entry->data+buf_entry->data_len,
			     state.fd_audio);
	else
	ac3_decode_data (buf_entry->data,
			 buf_entry->data+buf_entry->data_len);  
	break;
    case BUF_SUBPIC: 
	if(buf_entry->flags & BUF_FLAG_PTS_VALID)
	    dxr3_subpic_set_pts(buf_entry->pts);
	dxr3_subpicture_write(buf_entry->data,buf_entry->data_len);
	break;
    case BUF_VIDEO:
	if(buf_entry->flags & BUF_FLAG_PTS_VALID) 
	    dxr3_video_set_pts(buf_entry->pts);
	dxr3_video_write(buf_entry->data,buf_entry->data_len);
	break;
    default:
	fprintf (stderr, "codec_dxr3: unhandled stream type\n");
    }
    return 0;
}


/**
 *
 **/

static int _dxr3_init (plugin_codec_t *plugin, buf_t *_buf, char *output_path)
{
    int i;
    vo_functions_t *vo_funcs = NULL;
    ao_functions_t *ao_funcs = NULL;
    //FIXME hack output_path format "videoname:audioname"
    char buf[100], *vo, *ao;

    struct dxr3_priv_struct *priv = plugin->priv; 

    // FIXME
    strncpy(buf,output_path,100);
    vo = strtok(buf, ":, ");
    ao = strtok(NULL, ":, ");
    fprintf(stderr,"v:%s a:%s\n", vo, ao);

    ac3_config.num_output_ch = 2;
    ac3_config.flags = 0;
     
    // FIXME - not the sexiest bug-fix in the world - however
    // it does uphold the first law of a bug-fix, that is it
    // fixes the bug. vo -> "/dev/em8300"
    if(dxr3_open("/dev/em8300","/etc/dxr3.ux")) {
	fprintf(stderr,"Can't open dxr3 driver\n");
	return -1;
    }
    ac3_init (&ac3_config, &audio_out_dxr3);
   
    return 0;
}


/**
 *
 **/

static int _dxr3_set_status (plugin_codec_t *plugin, u_int status)
{
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
	fprintf (stderr, "DecoderSetStatus: unhandled status %d\n", status);
	return -1;
    }

    return 0;
}

/**
 *
 **/

static int _dxr3_set_attributes (plugin_codec_t *plugin, u_int id, void *attr)
{
    struct dxr3_priv_struct *priv = plugin->priv;
#ifdef DEBUG
    fprintf (stderr, "setting CLUT\n");
#endif
    switch (id) {
    case BUF_SUBPIC:
	{	
	    plugin_codec_attr_spu_t *spu = attr;
	    unsigned char clut[16*4];
	    int i;

	    if(spu->clut) {
		for (i=0; i<16; i++) {
		    clut[i*4+0] = spu->clut[i*4+3];
		    clut[i*4+1] = spu->clut[i*4+2];
		    clut[i*4+2] = spu->clut[i*4+1];
		    clut[i*4+3] = spu->clut[i*4+0];
		}
		dxr3_subpic_setpalette(clut);
	    }
	    break;
	}
	return 0;
    case BUF_VIDEO:
	{
	    plugin_codec_attr_video_t *attr = attr;
	    if(attr->aspect_output != -1) {
		switch(attr->aspect_output) {
		case 0:
		    dxr3_video_setaspectratio(EM8300_ASPECTRATIO_3_2);
		    break;
		case 3:
		    dxr3_video_setaspectratio(EM8300_ASPECTRATIO_16_9);
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
	}
    }
}

/**
 * Initialize Plugin.
 **/

void *plugin_init (char *whoami)
{
    pluginRegister (whoami,
		    PLUGIN_ID_CODEC_VIDEO,
		    STREAM_ID_MPEG2,
		    &codec_dxr3);

    return &codec_dxr3;
}


/**
 * Cleanup Plugin.
 **/

void plugin_exit (void)
{
}
