#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/em8300.h>
#include <inttypes.h>
int fd_control;
FILE *fd_out;
		  
void sub_writeregister(uint32_t registernum, uint32_t val){
		    
  em8300_register_t reg;
					      	
  reg.microcode_register = 0;
  reg.reg		= registernum;
  reg.val		= val;
  ioctl(fd_control, EM8300_IOCTL_WRITEREG, &reg);
}

uint32_t sub_readregister(uint32_t registernum){
  
  em8300_register_t reg;
  
  reg.microcode_register = 0;
  reg.reg		= registernum;
  reg.val		= 0;
  
  ioctl(fd_control, EM8300_IOCTL_READREG, &reg);
  return (reg.val);
}

long read_ucregister(int registernum){
  
  em8300_register_t reg;
  
  reg.microcode_register = 1;
  reg.reg		= registernum;
  reg.val		= 0;
  
  ioctl(fd_control, EM8300_IOCTL_READREG, &reg);
  return (reg.val);
}


/*
 * It appears that the follow routine copies length bytes to memory pointed to
 * by dst. 
 * Most likely, pos contains some kind of buffer offset.
 */
void dxr3_print_count(uint32_t base) {
  uint32_t i, result;
  for(i=1;i<0xf;i++) {
    result = sub_readregister( base+i ) ;
    printf("0x%x : 0x%x\n",base+i,result);
  }
  printf("\n");
}

char dxr3_copy_yuv_data( uint32_t pos, uint8_t *dst, uint32_t length, uint32_t value )
{
  uint32_t l1; /* ebp-8 */
  uint32_t l2; /* ebp-4 */
  //int l3; /* ebp-12 */
  int result;
  static try=0;
  try++;
  //printf("Pass 0x%x\n", value);
  //printf("Length=%x\n",length);
  
  //	for(l1 = 0x1000; (l1) ;--l1) {
  result = sub_readregister( 0x1c1a ) ;
  sub_writeregister( 0x1c1a, 1 ); // This stays 1 until all bytes have been written.
  // This also locks the counter if set to 1 or 3.
  if (result!=0)  printf("start result=0x%x\n",result);
  //		if (result==0) 
  //			break;
  //	}
  //	if (l1==0) 
  //	{
  //		//printf("Borked!\n");
  //		//return 0;
  //	}
  /* Make sure length is a multiple of 12 so the write loop writes the correct amount
     of data. */
  //length = length / 12;
  //length = length * 12;
  printf("Length=%d\n",length);
  pos = pos / 12;
  pos = pos * 12;
  printf("Pos=%d\n",pos);
  /*
  // For Read. 
  sub_writeregister( 0x1c50, 8 );
  sub_writeregister( 0x1c51, (pos & 0xffff) );
  sub_writeregister( 0x1c52, ((pos >> 16) & 0xffff) );
  sub_writeregister( 0x1c53, length - 1);
  sub_writeregister( 0x1c54, length - 1);
  sub_writeregister( 0x1c55, 0 );
  sub_writeregister( 0x1c56, 1 );
  sub_writeregister( 0x1c57, 1 );
  sub_writeregister( 0x1c58, (pos & 0xffff) );
  sub_writeregister( 0x1c59, ((pos >> 16) & 0xffff) );
  sub_writeregister( 0x1c5a, 1 );
  */
  // For write. All regs are 16 bits and any value higher is & 0xffff before writing.
  sub_writeregister( 0x1c1a, 1 ); // This stays 1 until all bytes have been written.
  sub_writeregister( 0x1c10, 8 );
  sub_writeregister( 0x1c11, pos ); // Position lsb
  sub_writeregister( 0x1c12, pos >> 16 );       // Position msb
  sub_writeregister( 0x1c13, length - 1);         // Length
  sub_writeregister( 0x1c14, length - 1);         // Use to reset length after Length -> 0
  sub_writeregister( 0x1c15, 0 );              // Unknown
  sub_writeregister( 0x1c16, 1 );              // Unknown
  sub_writeregister( 0x1c17, 1 );              // Unknown
  sub_writeregister( 0x1c18, pos & 0xffff ); // Use to reset Position lsb after Length -> 0
  sub_writeregister( 0x1c19, pos >> 16 );    // Use to reset Position msb after length -> 0 All written values are & 0x3f;
  sub_writeregister( 0x1c1a, 1 ); // This stays 1 until all bytes have been written then goes to 0.
    // If set to 3, it will never reset to 0, so the program can keep the lock for longer.
  // Simply setting this to 0, releases the lock. 
  for( l2=0; l2 < ((length)) ; l2++) {
    //dxr3_print_count(0x1c10);
    if (l2==try) {
      sub_writeregister(0x10000,value ); /* Write 1 bytes at once */
    } else {
      sub_writeregister(0x10000,0 ); /* Write 1 bytes at once */
    }
  }
#if 0	
  l2 = 0;
  for( l2=0; l2 < ((length/12)) ; ++l2) {
    uint32_t result1, result2, result3, a[12], offset;
    uint32_t b,c,d,e,f,g,h,i;
    /* FIXME: Change the 0x1 to whichever value you wish to write */
    //value=0x00;
    dxr3_print_count(0x1c10);
    sub_writeregister(0x11800,value ); /* Write 4 bytes at once */
    dxr3_print_count(0x1c10);
    sub_writeregister(0x11800,value );
    dxr3_print_count(0x1c10);
    sub_writeregister(0x11800,value );
    dxr3_print_count(0x1c10);
    //*dst++ = sub_readregister( 0x11800 );
    //result1 = sub_readregister( 0x11800  );
    //result2 = sub_readregister( 0x11800  );
    //result3 = sub_readregister( 0x11800  );
    //printf("0x%x ",result);
    
    a[0] = result1 & 0xff;
    a[1] = ((result1 >> 8) & 0xff);
    a[2] = ((result1 >> 16) & 0xff);
    a[3] = ((result1 >> 24) & 0xff);
    a[4] = result2 & 0xff;
    a[5] = ((result2 >> 8) & 0xff);
    a[6] = ((result2 >> 16) & 0xff);
    a[7] = ((result2 >> 24) & 0xff);
    a[8] = result3 & 0xff;
    a[9] = ((result3 >> 8) & 0xff);
    a[10] = ((result3 >> 16) & 0xff);
    a[11] = ((result3 >> 24) & 0xff);
    
    /* 
       a = result & 0x3f;
       b = ((result >> 6) & 0x3f);
       c = ((result >> 12) & 0x3f);
       d = ((result >> 18) & 0x3f);
       e = ((result >> 24) & 0x3f);
    */ 
    //*dst++ = d ;
    
    offset=0;
    
    *dst++ = 255;
    for(offset=0; offset < 12; offset++) {
      b = ((a[offset] & 0x1)==0) ? 0 : 0x80 ;
      c = ((a[offset] & 0x2)==0) ? 0 : 0x80 ;
      d = ((a[offset] & 0x4)==0) ? 0 : 0x80 ;
      e = ((a[offset] & 0x8)==0) ? 0 : 0x80 ;
      f = ((a[offset] & 0x10)==0) ? 0 : 0x80 ;
      g = ((a[offset] & 0x20)==0) ? 0 : 0x80 ;
      h = ((a[offset] & 0x40)==0) ? 0 : 0x80 ;
      i = ((a[offset] & 0x80)==0) ? 0 : 0x80 ;
      *dst++ = 200;
      *dst++ = i;
      *dst++ = h;
      *dst++ = g;
      *dst++ = f;
      *dst++ = e;
      *dst++ = d;
      *dst++ = c;
      *dst++ = b;
    }
    
    /* 
     *dst++ = ((a[0+offset] >> 2) & 0x3f) * 4;
     *dst++ = ((a[3+offset] >> 2) & 0x3f) * 4;
     *dst++ = ((a[6+offset] >> 2) & 0x3f) * 4;
     *dst++ = ((a[9+offset] >> 2) & 0x3f) * 4;
     */ 
    /*
     *dst++ = ((a[0+offset] >> 0) & 0x3) * 64;
     *dst++ = ((a[3+offset] >> 0) & 0x3) * 64;
     *dst++ = ((a[6+offset] >> 0) & 0x3) * 64;
     *dst++ = ((a[9+offset] >> 0) & 0x3) * 64;
     */
    /* 
     *dst++ = 255 ;
     for(offset=0;offset<12;offset++) {
     *dst++ = (a[0+offset] & 0x3f) * 4 ;
     }
    */
    //*dst++ = e << 2;
    //*dst++ = 255;
    //dst+=3;
  }
#endif
  //	*dst++ = sub_readregister( 0x11000 );
  /*	
    switch( length % 4 ) {
    case 3:
    *dst++ = sub_readregister( 0x11000 ); // Read 3 bytes 
    break;
    case 2:
    *dst++ = sub_readregister( 0x10800 ); // Read 2 bytes 
    break;
    case 1:
    *dst++ = sub_readregister( 0x10000 ); // Read 1 byte 
    break;
    }
  */	
  //	for( l1=0x1000; (l1) ; --l1) {
  //		if (sub_readregister( 0x1c1a )==0)
  //			return 1;
  //	}
  result = sub_readregister( 0x1c1a ) ;
  if (result != 0 ) printf("end result=0x%x\n",result);
  
  return 0;
}

#define DICOM_DisplayBuffer 62
#define Vsync_DBuf 63
#define MicroCodeVersion 105
#define Width_Buf3 26
#define Frames 1


typedef struct dbuffer_info_s {
  int xsize;
  int ysize;
  int xsize2;
  int flag1,flag2;
  int buffer1;
  int buffer2;
  int unk_present;
  int unknown1;
  int unknown2;
  int unknown3;
}dbuffer_info_t;

void em8300_dicom_get_dbufinfo(dbuffer_info_t *di)
{
  int displaybuffer;
  displaybuffer = read_ucregister(DICOM_DisplayBuffer)+0x1000;
  di->xsize = sub_readregister(displaybuffer);
  di->ysize = sub_readregister(displaybuffer+1);
  di->xsize2 = sub_readregister(displaybuffer+2) & 0xfff;
  di->flag1 = sub_readregister(displaybuffer+2) & 0x8000;
  
  printf("UCode: %d\n",read_ucregister(MicroCodeVersion));
  if(read_ucregister(MicroCodeVersion) <= 0xf) {
    di->buffer1 = (sub_readregister(displaybuffer+3)
		   |	(sub_readregister(displaybuffer+4)<<16)) << 4 ;
    di->buffer2 = (sub_readregister(displaybuffer+5)
		   |	(sub_readregister(displaybuffer+6)<<16)) << 4 ;
  } else {
    di->buffer1 = sub_readregister(displaybuffer+3) << 6;
    di->buffer2 = sub_readregister(displaybuffer+4) << 6;
  }
  di->unk_present = 0;
}


#define BASE "snap"

int main() {
  int x,y;
  
  uint8_t *buf;
  int iy, iu, iv, r, g, b;
  //int top, bottom;
  int width,height, looping;
  
  FILE *y_out, *u_out, *v_out, *rgb_out;
  dbuffer_info_t *di, _di;
  di = &_di; /* works better if pointing to something */
  
  buf=malloc(72000*3);
  memset(buf,72000*3, 0);
  //for(looping=0;looping < 32;looping ++) {
  fd_control=open("/dev/em8300-0", O_WRONLY);
  x=0;
  
  em8300_dicom_get_dbufinfo(di);
  
  /* Write Y */
  printf("Y: width %d ", di->xsize);
  //if(di->xsize>0x160)
  //  di->xsize=0x220;
  //else
  //  di->xsize=0x160;
  
  printf(" -> %d; height %d\n", di->xsize, di->ysize);
  width = di->xsize; height = di->ysize;
  
  //fprintf(y_out,"P5\n%d %d\n255\n",di->xsize,di->ysize);
  printf(" di->buffer1 = %x\n", di->buffer1);
  //di->buffer1 = 0x8fffc;
  printf(" start di->buffer1 = %x\n", di->buffer1);
  for(y=0;y<di->ysize*Frames;y++){
    //for(y=0;y<2;y++){
    //dxr3_copy_yuv_data( di->buffer1, buf, di->xsize, (0x1 << (y/15)) | 0x4);
    //dxr3_copy_yuv_data( di->buffer1, buf, di->xsize, (0x1 << (y/15)) & 0xffffff | 0x800000);
  dxr3_copy_yuv_data( di->buffer1, buf, di->xsize, 0xff );
  di->buffer1=di->buffer1+(di->xsize);
}
//printf(" end di->buffer1 = %x\n", di->buffer1);
//}
close(fd_control);
free(buf);
return 0;
}
