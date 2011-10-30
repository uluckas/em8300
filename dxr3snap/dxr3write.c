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

uint32_t sub_readregister(uint32_t registernum)
{ 
  em8300_register_t reg;
  reg.microcode_register = 0;
  reg.reg		= registernum;
  reg.val		= 0;
  
  ioctl(fd_control, EM8300_IOCTL_READREG, &reg);
  return (reg.val);
}

long read_ucregister(int registernum)
{
  
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

char dxr3_copy_yuv_data( uint32_t pos, uint8_t *dst, uint32_t length )
{
  uint32_t l1; /* ebp-8 */
  uint32_t l2; /* ebp-4 */
  //int l3; /* ebp-12 */
  int result;
  
  for(l1 = 0x1000; (l1) ;--l1) {
    result = sub_readregister( 0x1c1a ) ;
    //               printf("result=0x%x\n",result);
    if (result==0) 
      break;
  }
  if (l1==0) 
    {
      printf("Borked!\n");
      //return 0;
    }
  /*
    sub_writeregister( 0x1c50, 8 );
    sub_writeregister( 0x1c51, (pos & 0xffff) );
    sub_writeregister( 0x1c52, (pos >> 16) & 0xffff );
    sub_writeregister( 0x1c53, length );
    sub_writeregister( 0x1c54, length );
    sub_writeregister( 0x1c55, 0 );
    sub_writeregister( 0x1c56, 1 );
    sub_writeregister( 0x1c57, 1 );
    sub_writeregister( 0x1c58, pos & 0xffff );
    sub_writeregister( 0x1c59, 0 );
    sub_writeregister( 0x1c5a, 1 );
  */
  //sub_writeregister( 0x1c10, 8 );
  sub_writeregister( 0x1c11, pos & 0xffff );
  sub_writeregister( 0x1c12, pos >>16 );
  sub_writeregister( 0x1c13, length );
  sub_writeregister( 0x1c14, length );
  sub_writeregister( 0x1c15, 0 );
  sub_writeregister( 0x1c16, 1 );
  sub_writeregister( 0x1c17, 1 );
  sub_writeregister( 0x1c18, pos & 0xffff );
  sub_writeregister( 0x1c19, pos >> 16 );
  sub_writeregister( 0x1c1a, 1 );
  
  l2 = 0;
  for( l2=0; l2 < ((length/4)) ; ++l2) {
    uint32_t result1, result2, result3, a[12], offset;
    uint32_t b,c,d,e,f,g,h,i;
    /* FIXME: Change the 0x1 to whichever value you wish to write */
    sub_writeregister(0x11800,0x1);
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
  //	*dst++ = sub_readregister( 0x11000 );
  /*	
    switch( length % 4 ) {
    case 3:
    *dst++ = sub_readregister( 0x11000 );
    break;
    case 2:
    *dst++ = sub_readregister( 0x10800 );
    break;
    case 1:
    *dst++ = sub_readregister( 0x10000 );
    break;
    }
  */	
  for( l1=0x1000; (l1) ; --l1) {
    if (sub_readregister( 0x1c1a )==0)
      return 1;
  }
  
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
  int width,height;
  
  FILE *y_out, *u_out, *v_out, *rgb_out;
  dbuffer_info_t *di, _di;
  di = &_di; /* works better if pointing to something */
  
  buf=malloc(72000*3);
  memset(buf, 0, 72000*3);
  fd_control=open("/dev/em8300-0", O_WRONLY);
  y_out=fopen(BASE ".Y", "wb");
  u_out=fopen(BASE ".U", "wb");
  v_out=fopen(BASE ".V", "wb");
  x=0;
  
  em8300_dicom_get_dbufinfo(di);
  
  /* Grab Y */
  printf("Y: width %d ", di->xsize);
  if(di->xsize>0x160)
    di->xsize=0x220;
  else
    di->xsize=0x160;
  
  printf(" -> %d; height %d\n", di->xsize, di->ysize);
  width = di->xsize; height = di->ysize;
  
  //fprintf(y_out,"P5\n%d %d\n255\n",di->xsize,di->ysize);
  printf(" di->buffer1 = %x\n", di->buffer1);
  //di->buffer1 = 0x62000;
  printf(" start di->buffer1 = %x\n", di->buffer1);
  for(y=0;y<di->ysize*Frames;y++){
    dxr3_copy_yuv_data( di->buffer1, buf, di->xsize);
    di->buffer1=di->buffer1+(di->xsize);
    fwrite(buf,di->xsize*8,1,y_out);
  }
  printf(" end di->buffer1 = %x\n", di->buffer1);
  fclose(y_out);
  /* Grab U & V */
  //em8300_dicom_get_dbufinfo(di);
  printf("1: %d 2: %d\n",di->buffer1,di->buffer2);
  printf("UV: width %d ", di->xsize);
  //if(di->xsize>0x160)  
  //  di->xsize=0x220;	// 544 pixels wide
  //else
  //di->xsize=0x160;
  printf(" -> %d; height %d\n", di->xsize, di->ysize);
  
  /* U */ 
  //fprintf(u_out,"P5\n%d %d\n255\n",di->xsize,di->ysize);
  for(y=0;y<di->ysize;y++){
    /* get two lines at once */
    //dxr3_copy_yuv_data( di->buffer2, buf, di->xsize);
    di->buffer2 += di->xsize;
#if 1
    /* assume sampled at width/2 x height, horizontal repeat */ 	
    for (x=0; x<di->xsize/4; x++) {
      fwrite(buf+x,1, 1, u_out); 
      fwrite(buf+x,1, 1, u_out);
      fwrite(buf+x,1, 1, u_out);
      fwrite(buf+x,1, 1, u_out);
      
    }
    for (x=0; x<di->xsize/4; x++) {
      fwrite(buf+x,1, 1, u_out);
      fwrite(buf+x,1, 1, u_out);
      fwrite(buf+x,1, 1, u_out);
      fwrite(buf+x,1, 1, u_out);
    }
    
#else
    /* assume sampled at width x height/2, vertical repeat */ 	
    fwrite(buf, di->xsize, 1, u_out);
    fwrite(buf, di->xsize, 1, u_out);
#endif
  }
  /* V */ 	
  di->buffer2+=352*160; // Next frame of U :-(
  
  //fprintf(v_out,"P5\n%d %d\n255\n",di->xsize,di->ysize);
  
  for(y=0;y<di->ysize/2;y++){
    //dxr3_copy_yuv_data( di->buffer2, buf, di->xsize);
    di->buffer2 += di->xsize;
#if 1
    for (x=0; x<di->xsize/4; x++) {
      fwrite(buf+x,1, 1, v_out); 
      fwrite(buf+x,1, 1, v_out);
      fwrite(buf+x,1, 1, v_out);
      fwrite(buf+x,1, 1, v_out);
    }
    for (x=0; x<di->xsize/4; x++) {
      fwrite(buf+x,1, 1, v_out);
      fwrite(buf+x,1, 1, v_out);
      fwrite(buf+x,1, 1, v_out);
      fwrite(buf+x,1, 1, v_out);
    }
#else
    fwrite(buf, di->xsize, 1, v_out);
    fwrite(buf, di->xsize, 1, v_out);
#endif
  }
  fclose(u_out);
  fclose(v_out);
  close(fd_control);
  free(buf);
  /* convert to rgb */
  y_out = fopen(BASE ".Y", "rb");
  u_out = fopen(BASE ".U", "rb");
  v_out = fopen(BASE ".V", "rb");
  rgb_out = fopen(BASE ".rgb", "wb");
  fprintf(rgb_out,"P6\n");
  fprintf(rgb_out,"%d %d\n",width*8,(height*Frames)/2);
  fprintf(rgb_out,"255\n");
  for (y=0; y<(height*Frames); y++) for (x=0; x<width*8; x++) {
    iy = fgetc(y_out);   
    //iu = fgetc(u_out);   
    iu = 0;   
    //iv = fgetc(v_out);   
    iv=0; // iv is wrong, remove it for now!
    //r = (1.1644 * iy)                + (1.5960 * iv);
    //g = (1.1644 * iy) - (0.3918 * iu) - (0.8130 * iv);
    //b = (1.1644 * iy) + (2.0172 * iu);
    r=iy; g=iy; b=iy;
    /*r = iy + 1.371*iv;
     *g = y - 0.699*iu-0.337*iv;
     *b = y + 1.733*iv;*/
    //printf("%d %d %d\n",r,g,b);
    fputc(r &0xff, rgb_out);
    fputc(g &0xff, rgb_out);
    fputc(b &0xff, rgb_out);
  }
  fclose(y_out); fclose(u_out); fclose(v_out); fclose(rgb_out);
  return 0;
}
