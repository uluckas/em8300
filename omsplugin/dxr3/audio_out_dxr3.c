/*
 *
 *  audio_out_linux.c
 *    
 *	Copyright (C) Aaron Holtzman - May 1999
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
#  include <config.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#if defined(__OpenBSD__)
#include <soundcard.h>
#elif defined(__FreeBSD__)
#include <machine/soundcard.h>
#else
#include <sys/soundcard.h>
#endif
#include <sys/ioctl.h>

#include <audio_out.h>
#include "audio_out_internal.h"

#include "dxr3-api.h"

static ao_info_t ao_info =
{
	"DXR3 driver output ",
	"dxr3",
	"Henrik Johansson <henrikjo@post.utfors.se>",
	""
};

/*
 * open the audio device for writing to
 */
static uint_32
ao_open(uint_32 bits, uint_32 rate, uint_32 channels)
{
    if(dxr3_get_status() == DXR3_STATUS_CLOSED) {
	fprintf(stderr,"audio_out_dxr3.c: DXR3 driver is not initiated.\n");
	return -1;
    }	

    dxr3_audio_set_stereo(channels == 2 ? 1 : 0);
    dxr3_audio_set_samplesize(bits);
    dxr3_audio_set_rate(rate);

  return 1;
}

/*
 * play the sample to the already opened file descriptor
 */
static void 
ao_play(sint_16* output_samples, uint_32 num_bytes)
{
    dxr3_audio_write(output_samples,1024 * 6);
}


static void
ao_close(void)
{
}


static const ao_info_t*
ao_get_info(void)
{
    return &ao_info;
}

//export our ao implementation
LIBAO_EXTERN(dxr3);
