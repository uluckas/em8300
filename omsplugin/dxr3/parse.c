/* 
 *    parse.c
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

#include <stdlib.h>
#include <stdio.h>
#include "ac3.h"
#include "ac3_internal.h"


#include "crc.h"
#include "parse.h"


struct frmsize_s
{
	uint_16 bit_rate;
	uint_16 frm_size[3];
};

static const struct frmsize_s frmsizecod_tbl[64] = 
{
	{ 32  ,{64   ,69   ,96   } },
	{ 32  ,{64   ,70   ,96   } },
	{ 40  ,{80   ,87   ,120  } },
	{ 40  ,{80   ,88   ,120  } },
	{ 48  ,{96   ,104  ,144  } },
	{ 48  ,{96   ,105  ,144  } },
	{ 56  ,{112  ,121  ,168  } },
	{ 56  ,{112  ,122  ,168  } },
	{ 64  ,{128  ,139  ,192  } },
	{ 64  ,{128  ,140  ,192  } },
	{ 80  ,{160  ,174  ,240  } },
	{ 80  ,{160  ,175  ,240  } },
	{ 96  ,{192  ,208  ,288  } },
	{ 96  ,{192  ,209  ,288  } },
	{ 112 ,{224  ,243  ,336  } },
	{ 112 ,{224  ,244  ,336  } },
	{ 128 ,{256  ,278  ,384  } },
	{ 128 ,{256  ,279  ,384  } },
	{ 160 ,{320  ,348  ,480  } },
	{ 160 ,{320  ,349  ,480  } },
	{ 192 ,{384  ,417  ,576  } },
	{ 192 ,{384  ,418  ,576  } },
	{ 224 ,{448  ,487  ,672  } },
	{ 224 ,{448  ,488  ,672  } },
	{ 256 ,{512  ,557  ,768  } },
	{ 256 ,{512  ,558  ,768  } },
	{ 320 ,{640  ,696  ,960  } },
	{ 320 ,{640  ,697  ,960  } },
	{ 384 ,{768  ,835  ,1152 } },
	{ 384 ,{768  ,836  ,1152 } },
	{ 448 ,{896  ,975  ,1344 } },
	{ 448 ,{896  ,976  ,1344 } },
	{ 512 ,{1024 ,1114 ,1536 } },
	{ 512 ,{1024 ,1115 ,1536 } },
	{ 576 ,{1152 ,1253 ,1728 } },
	{ 576 ,{1152 ,1254 ,1728 } },
	{ 640 ,{1280 ,1393 ,1920 } },
	{ 640 ,{1280 ,1394 ,1920 } }
};

/* Parse a syncinfo structure, minus the sync word */
void
parse_syncinfo(syncinfo_t *syncinfo,uint_8 *data)
{
	//
	// We need to read in the entire syncinfo struct (0x0b77 + 24 bits)
	// in order to determine how big the frame is
	//

	// Get the sampling rate 
	syncinfo->fscod  = (data[2] >> 6) & 0x3;

	if(syncinfo->fscod == 3)
	{
		//invalid sampling rate code
		//error_flag = 1;	
		return;
	}
	else if(syncinfo->fscod == 2)
		syncinfo->sampling_rate = 32000;
	else if(syncinfo->fscod == 1)
		syncinfo->sampling_rate = 44100;
	else
		syncinfo->sampling_rate = 48000;

	// Get the frame size code 
	syncinfo->frmsizecod = data[2] & 0x3f;

	// Calculate the frame size and bitrate
	syncinfo->frame_size = 
		frmsizecod_tbl[syncinfo->frmsizecod].frm_size[syncinfo->fscod];
	syncinfo->bit_rate = frmsizecod_tbl[syncinfo->frmsizecod].bit_rate;

}
