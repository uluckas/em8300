/*
 *
 *  audio_out_internal.h 
 *    
 *	Copyright (C) Aaron Holtzman - June  2000
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
 *  This file is based on output_linux.c by Aaron Holtzman.
 *  All .wav modifications were done by Jorgen Lundman <lundman@lundman.net>
 *  Any .wav bugs and errors should be reported to him.
 *
 *
 */

static uint_32 ao_open(uint_32 bits, uint_32 rate, uint_32 channels);
static void ao_play(sint_16* output_samples, uint_32 num_bytes);
static void ao_close(void);
static const ao_info_t* ao_get_info(void);


#define LIBAO_EXTERN(x) ao_functions_t audio_out_##x =\
{\
	ao_open,\
	ao_play,\
	ao_close,\
	ao_get_info\
}

