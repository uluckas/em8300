/***************************************************************************/
/*  This code has been fully inspired from various place                   */
/*  I will give their name later. May they be bless .... It works   	   */
/*                                                                         */
/*	For the moment test it.                                            */   
/*									   */
/* 27-08-00 -- Ze'ev Maor -- fixed recovery from flase syncword detection  */
/* 								           */
/* 24-08-00 -- Ze'ev Maor -- Modified for integrtion with DXR3-OMS-plugin  */  
/***************************************************************************/
									   
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ac3.h"
#include "ac3_internal.h"
#include "parse.h"
#include "crc.h"

#define BLOCK_SIZE 6144

static char buf[BLOCK_SIZE];
static uint_32 sbuffer_size = 0;
static syncinfo_t syncinfo;
static char *sbuffer = &buf[10];

uint_32
buffer_syncframe(syncinfo_t *syncinfo, uint_8 **start, uint_8 *end)
{
  uint_8 *cur = *start;
  uint_16 syncword = syncinfo->syncword;
  uint_32 ret = 0;
 
  //
  // Find an ac3 sync frame.
  //
  while(syncword != 0x0b77)
    {
      if(cur >= end)
	goto done;
      syncword = (syncword << 8) + *cur++;
    }
 
  //need the next 3 bytes to decide how big the frame is
  while(sbuffer_size < 3)
    {
      if(cur >= end)
	goto done;
 
      sbuffer[sbuffer_size++] = *cur++;
    }
                                                                                    
  parse_syncinfo(syncinfo,sbuffer);
 
  while(sbuffer_size < syncinfo->frame_size * 2 - 2)
    {
      if(cur >= end)
	goto done;
 
      sbuffer[sbuffer_size++] = *cur++;
    }
 
  crc_init();
  crc_process_frame(sbuffer,syncinfo->frame_size * 2 - 2);
	
  if(!crc_validate())
    {
      //error_flag = 1;
      fprintf(stderr,"** CRC failed - skipping frame **\n");
      syncword = 0xffff;
      sbuffer_size = 0;
      bzero(buf,BLOCK_SIZE);

      goto done;
    }                                                                           
  //
  //if we got to this point, we found a valid ac3 frame to decode
  //
 
  //reset the syncword for next time
  syncword = 0xffff;
  sbuffer_size = 0;
  ret = 1;
 
 done:
  syncinfo->syncword = syncword;
  *start = cur;
  return ret;
}                                                                                  

void
output_spdif(uint_8 *data_start, uint_8 *data_end, int fd)
{
  unsigned short *sbuf = (unsigned short *)buf;
  
  while(buffer_syncframe(&syncinfo, &data_start, data_end))
    {
      sbuf[0] = 0xf872;  //spdif syncword
      sbuf[1] = 0x4e1f;  // .............
      sbuf[2] = 0x0001;  // AC3 data
      sbuf[3] = syncinfo.frame_size * 16;
      sbuf[4] = 0x0b77;  // AC3 syncwork
      
      // extract_ac3 seems to write swabbed data
      swab(&buf[10], &buf[10], syncinfo.frame_size * 2 - 2);
      write(fd,buf, BLOCK_SIZE);
      bzero(buf,BLOCK_SIZE);
    }
}

