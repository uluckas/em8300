/*
 *
 * Copyright (C) 1999, 2000  Thomas Mirlacher
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


#define DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <oms/oms.h>
#include <oms/plugin.h>

#include <ac3.h>
#include <mpeg2.h>

#include <linux/em8300.h>

#include <errno.h>
#if defined(__OpenBSD__)
#include <soundcard.h>
#elif defined(__FreeBSD__)
#include <machine/soundcard.h>
#else
#include <sys/soundcard.h>
#endif
#include <sys/ioctl.h>
#define AUDIO_CHUNK_SIZE 2048
#define AUDIO_BUF_SIZE 2*AUDIO_CHUNK_SIZE

uint_8 audio_buf[AUDIO_BUF_SIZE];
uint_8 *audio_buf_current, *audio_buf_next;
uint_8 *video_buf_current, *video_buf_next;
int audio_played = 1;
#define TEMP_BUF_SIZE 1024*1024
uint_8 tempbuf[TEMP_BUF_SIZE];
long long audio_pts,audio_ts = 0, video_ts = 0;
int sound_started = 0;

unsigned int video_bytes = 0, audio_bytes = 0;

#include "../../codec/mpg123/mpg123.h"
#include "../../codec/mpg123/mpglib.h"

static struct dxr3_priv_struct {
    char *path;
    
    int fd_control;
    int fd_video;
    int fd_audio;
    int fd_other;
    int fd_spu;

    int status;
    
    buf_t *buf;
} dxr3_priv;

static int fd_audio;

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
} ; 


static ac3_config_t ac3_config;

plugin_codec_t *plugin_init ()
{
    return &codec_dxr3;
}

/**
 *
 **/

static int _dxr3_open (plugin_codec_t *plugin)
{
    struct dxr3_priv_struct *priv = plugin->priv;
    //dxr3_status_info_t *dxr3_info = &priv->dxr3_info;

    int size = 100;
    char *buffer = (char *) malloc (size);
    if(buffer == NULL)
	return -ENOMEM;

    fprintf(stderr,"In Dxr3 Open\n");

    // open device
    fprintf(stderr,"Dxr3 Path %s\n", priv->path);
    if ((priv->fd_control = open (priv->path, O_WRONLY)) < 0) {
	perror(priv->path);
	return -1;
    }


     dxr3_init (priv->fd_control); 

     if (_dxr3_install_firmware (priv, "/etc/dxr3.ux") == -1) 
 	return -1; 

    // We don't need the control device anymore
    close(priv->fd_control);
    priv->fd_control=-1;

    // open video device
    snprintf (buffer, size, "%s_mv", priv->path);
    if ((priv->fd_video = open (buffer, O_WRONLY)) < 0) {
	perror(buffer);
	return -1;
    }

    // open audio device
    snprintf (buffer, size, "%s_ma", priv->path);
    if ((priv->fd_audio = open (buffer, O_WRONLY)) < 0) {
	perror(buffer);
	return -1;
    }
    fd_audio = priv->fd_audio;

    // open spu device
    snprintf (buffer, size, "%s_sp", priv->path);
    if ((priv->fd_spu = open (buffer, O_WRONLY)) < 0) {
	perror(buffer);
	return -1;
    }

    free (buffer);

    priv->status = EM8300_PLAYMODE_STOPPED;
	     
    return 0;

}

static int decode_blocks()
{
    unsigned int fragmentsize, leftover;
    uint_8 *start, *end;
  
    if (audio_bytes >= AUDIO_CHUNK_SIZE)
	{
	    //      printf("video buffer (%i/%i)\n", video_bytes, VIDEO_BUF_SIZE);
	    //      printf("video_ts %li audio_ts %li (%li)\n", (long)video_ts, (long)audio_ts, (long)audio_ts - video_ts);
	    if (audio_buf_current+AUDIO_CHUNK_SIZE > audio_buf + AUDIO_BUF_SIZE)
		{
		    fragmentsize = audio_buf + AUDIO_BUF_SIZE - audio_buf_current;
		    leftover = AUDIO_CHUNK_SIZE - (audio_buf + AUDIO_BUF_SIZE - audio_buf_current);
		    if (fragmentsize + leftover > TEMP_BUF_SIZE)
			{
			    fprintf(stderr, "oops, audio tempbuf too small, %i < %i\n", fragmentsize + leftover, TEMP_BUF_SIZE);
			    exit(0);
			}
		    memcpy(tempbuf, audio_buf_current, fragmentsize);
		    memcpy(tempbuf + fragmentsize, audio_buf, leftover); 
		    start = tempbuf;
	  audio_buf_current = audio_buf + leftover;
	  end = tempbuf + fragmentsize + leftover;
	}
      else
	{
	  start = audio_buf_current;
	  audio_buf_current += AUDIO_CHUNK_SIZE;
	  end = start + AUDIO_CHUNK_SIZE;
	}	
      
      audio_bytes -= (end - start);
      //  printf("audio out %i\n", (end - start));
      //      printf("audio ts %li\n", audio_ts);
      ac3_decode_data(start, end);  
      audio_ts = 0;
    }	
}

static int _dxr3_install_firmware (struct dxr3_priv_struct *priv, char *ucode)
{
        int uCodeFD;
        int uCodeSize;
	em8300_microcode_t uCode;
 
	fprintf(stderr,"In install firmware\n");

        if ((uCodeFD = open (ucode, O_RDONLY)) < 0) {
                perror (ucode);
                return -1;
        }
        uCodeSize = lseek (uCodeFD, 0, SEEK_END);
        if ((uCode.ucode = (void*) malloc (uCodeSize)) == NULL) {
                perror ("malloc");
                return -1;
        }
        lseek(uCodeFD, 0, SEEK_SET);
        if (read (uCodeFD, uCode.ucode, uCodeSize) != uCodeSize) {
                perror (ucode);
                return -1;
        }
        close (uCodeFD);
        uCode.ucode_size = uCodeSize;

        // upload ucode
        if (dxr3_install_firmware (priv->fd_control, &uCode)) {
                fprintf (stderr,"uCode upload failed!\n");
                return -1;
        }


        return 0;
}

static int insert_audio_block(uint_8 *data, int data_len, long long clock)
{
  int i;

  //  printf("audio in %i\n", data_len);
  //if (audio_played)
  //  {
  //    printf("set audio ts %li\n", clock);
  audio_ts = clock;
  //    audio_played = 0;
  // }
  
  audio_bytes += data_len;

  if (audio_buf_next + data_len > audio_buf + AUDIO_BUF_SIZE)
    i = (audio_buf + AUDIO_BUF_SIZE) - audio_buf_next;
  else
    i = data_len;
  memcpy(audio_buf_next, data, i);
  audio_buf_next += i;
  if (i != data_len)
    {
      audio_buf_next = audio_buf;
      memcpy(audio_buf_next, data+i, data_len - i);
      audio_buf_next += data_len - i;
    }
  return 0;
}


/**
 *
 **/
static int _dxr3_read (plugin_codec_t *plugin, buf_t *buf, buf_entry_t *buf_entry)
{
    long tmp;
    struct dxr3_priv_struct *priv = plugin->priv;

    switch (buf_entry->buf_id) {
    case BUF_AUDIO: {
	if(buf_entry->flags & BUF_FLAG_PTS_VALID) {
	    //	    printf("audio: %d\n",buf_entry->pts);
	    tmp = buf_entry->pts;
	    ioctl(priv->fd_audio, EM8300_IOCTL_AUDIO_SETPTS, &tmp);
	}
	switch (buf_entry->type) {
	default:
	    while ((AUDIO_BUF_SIZE - audio_bytes) < buf_entry->data_len)
		decode_blocks(); // if we dont have room, decode some more blocks
		insert_audio_block(buf_entry->data, buf_entry->data_len, (buf_entry->clock * 300 + buf_entry->clock_ext)/27); 
	}
    }
    break;	
    case BUF_OTHER:
	if (priv->fd_other >= 0) {
	    fprintf (stderr, "other %d",  buf_entry->data_len);
	    write (priv->fd_other, buf_entry->data, buf_entry->data_len);
	}

	break;
    case BUF_SUBPIC: {
	if(buf_entry->flags & BUF_FLAG_PTS_VALID) {
	    // printf("Spu: %d\n",buf_entry->pts);
	    ioctl(priv->fd_spu, EM8300_IOCTL_SPU_SETPTS,buf_entry->pts);
	}
	
	if (priv->fd_spu >= 0) {
	    write (priv->fd_spu, buf_entry->data-3, buf_entry->data_len+3);
	}	
	break;
    }
    case BUF_VIDEO: {
	if(buf_entry->flags & BUF_FLAG_PTS_VALID) {
	    //	    printf("video: %d\n",buf_entry->pts);
	    ioctl(priv->fd_video, EM8300_IOCTL_VIDEO_SETPTS,buf_entry->pts);
	}
	if (priv->fd_video >= 0) 
	    write (priv->fd_video, buf_entry->data, buf_entry->data_len);
	break;
    }
    default:
	//		fprintf (stderr, "codec_dxr3: unhandled stream type\n");
    }

    //	ExitMP3 (&mp);

    return 0;
}

void output_close(void) {
}

uint_32 output_open(uint_32 bits, uint_32 rate, uint_32 channels)
{
  int tmp;
  
  /*
   * Open the device driver
   */

  tmp = channels == 2 ? 1 : 0;
  ioctl(fd_audio,SNDCTL_DSP_STEREO,&tmp);

  tmp = bits;
  ioctl(fd_audio,SNDCTL_DSP_SAMPLESIZE,&tmp);

  tmp = rate;
  ioctl(fd_audio,SNDCTL_DSP_SPEED, &tmp);
  
  return 1;
}

/*
 * play the sample to the already opened file descriptor
 */
static int sync_cnt=800;
void output_play(sint_16* output_samples, uint_32 num_bytes)
{
//	if(fd < 0)
//		return;
    //    printf("Sound output: %d\n",num_bytes);
    if(sync_cnt++ == 1000) {
	ioctl(fd_audio, EM8300_IOCTL_AUDIO_SYNC, 0);	
	sync_cnt=0;
    }
  if (write(fd_audio,output_samples,num_bytes * 2) == EAGAIN)
    {
      fprintf(stderr, "woops, audio full!\n");
    }
}


static int _dxr3_init (plugin_codec_t *plugin, buf_t *buf, char *output_path)
{
    struct dxr3_priv_struct *priv = plugin->priv;
    ao_functions_t ao_funcs =
    {
	output_open,
	output_play,
	output_close
    };

    priv->buf = buf;
    priv->path = output_path;
    
    _dxr3_open(plugin);

    ac3_config.num_output_ch = 2;
    ac3_config.flags = 0;
     
    audio_buf_current = audio_buf_next = audio_buf;
    sound_started = 0;

    ac3_init(&ac3_config, &ao_funcs);

    return 0;
}


/**
 *
 **/

static int _dxr3_set_status (plugin_codec_t *plugin, u_int status)
{
    switch (status) {
    case STATUS_PLAY:
    case STATUS_PAUSE:
    case STATUS_STOP:
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
    case BUF_SUBPIC: {	
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
	    ioctl(priv->fd_spu, EM8300_IOCTL_SPU_SETPALETTE,clut);
	}
	break;
    }	
    }
    return 0;
}
