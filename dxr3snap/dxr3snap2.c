#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include<linux/em8300.h>

int fd_control;
FILE *fd_out;

void sub_writeregister(int registernum, int val){

     	em8300_register_t reg;
     	
     	reg.microcode_register = 0;
  	reg.reg		= registernum;
     	reg.val		= val;
     	ioctl(fd_control, EM8300_IOCTL_WRITEREG, &reg);
}

long sub_readregister(int registernum){

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

char dxr3_copy_yuv_data( int pos, int *dst, int length )
{
	int l1; /* ebp-8 */
	int l2; /* ebp-4 */
	//int l3; /* ebp-12 */

	for(l1 = 0x1000; (l1) ;--l1) {
		if (sub_readregister( 0x1c1a )==0) 
			break;
	}
	if (l1==0) 
	{
		printf("Borked!\n");
		return 0;
	}

	sub_writeregister( 0x1c50, 8 );
	sub_writeregister( 0x1c51, pos & 0xffff );
	sub_writeregister( 0x1c52, pos >>16 );
	sub_writeregister( 0x1c53, length );
	sub_writeregister( 0x1c54, length );
	sub_writeregister( 0x1c55, 0 );
	sub_writeregister( 0x1c56, 1 );
	sub_writeregister( 0x1c57, 1 );
	sub_writeregister( 0x1c58, pos & 0xffff );
	sub_writeregister( 0x1c59, 0 );
	sub_writeregister( 0x1c5a, 1 );
	
	l2 = 0;
	for( l2=0; l2 < (length>>2) ; ++l2) {
		//sub_writeregister(0x11800,0xAA);
		*dst++ = sub_readregister( 0x11800 );
	}
	
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
		di->buffer1 = (sub_readregister(displaybuffer+3) |	(sub_readregister(displaybuffer+4)<<16)) << 4 ;
		di->buffer2 = (sub_readregister(displaybuffer+5) |	(sub_readregister(displaybuffer+6)<<16)) << 4 ;
	} else {
		di->buffer1 = sub_readregister(displaybuffer+3) << 6;
		di->buffer2 = sub_readregister(displaybuffer+4) << 6;
	}
		di->unk_present = 0;
}


#define BASE "snap"

int main() {
       	int x,y;
	unsigned char *bbuf;

	int *buf;
	int iy, iu, iv, r, g, b;
	//int top, bottom;
	int width,height;
	
	FILE *y_out, *u_out, *v_out, *rgb_out;
	dbuffer_info_t *di, _di;
	di = &_di; /* works better if pointing to something */

	buf=malloc(720*3);	
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

	for(y=0;y<di->ysize;y++){
		dxr3_copy_yuv_data( di->buffer1, buf, di->xsize);
		di->buffer1=di->buffer1+di->xsize;
		fwrite(buf,di->xsize,1,y_out);
	}
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
	//fprintf(v_out,"P5\n%d %d\n255\n",di->xsize,di->ysize);

	for(y=0;y<di->ysize;y++){
		/* get two lines at once */
		dxr3_copy_yuv_data( di->buffer2, buf, di->xsize);
		di->buffer2 += di->xsize;
#if 1
		/* assume sampled at width/2 x height, horizontal repeat */ 	
		bbuf=buf;
		for (x=0; x<di->xsize; x+=4) {
			fwrite(bbuf+x,1, 1, u_out); 
			fwrite(bbuf+x,1, 1, u_out); 
			fwrite(bbuf+x+2,1,1,u_out);
			fwrite(bbuf+x+2,1, 1, u_out); 

			fwrite(bbuf+x+1,1, 1, v_out); 
			fwrite(bbuf+x+1,1, 1, v_out); 
			fwrite(bbuf+x+3,1,1,v_out);
			fwrite(bbuf+x+3,1, 1, v_out); 
			//fwrite(bbuf+x,1, 1, u_out);
			//fwrite(bbuf+x,1, 1, u_out);
			//fwrite(bbuf+x,1, 1, u_out);

		}
		for (x=0; x<di->xsize; x+=4) {
		    fwrite(bbuf+x,1, 1, u_out);
			fwrite(bbuf+x,1, 1, u_out); 
			fwrite(bbuf+x+2,1,1,u_out);
			fwrite(bbuf+x+2,1, 1, u_out); 

			fwrite(bbuf+x+1,1, 1, v_out); 
			fwrite(bbuf+x+1,1, 1, v_out); 
			fwrite(bbuf+x+3,1,1,v_out);
			fwrite(bbuf+x+3,1, 1, v_out); 

		    //fwrite(bbuf+x,1, 1, u_out);
			//fwrite(bbuf+x,1, 1, u_out);
			//fwrite(bbuf+x,1, 1, u_out);
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
		dxr3_copy_yuv_data( di->buffer2, buf, di->xsize);
		di->buffer2 += di->xsize;
#if 1
		for (x=0; x<di->xsize/4; x++) {
			/*fwrite(buf+x,1, 1, v_out); 
			fwrite(buf+x,1, 1, v_out);
			fwrite(buf+x,1, 1, v_out);
			fwrite(buf+x,1, 1, v_out);
		}
		for (x=0; x<di->xsize/4; x++) {
			fwrite(buf+x,1, 1, v_out);
			fwrite(buf+x,1, 1, v_out);
			fwrite(buf+x,1, 1, v_out);
			fwrite(buf+x,1, 1, v_out);*/
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
	fprintf(rgb_out,"%d %d\n",width,height);
	fprintf(rgb_out,"255\n");
	for (y=0; y<height; y++) for (x=0; x<width; x++) {
		iy = fgetc(y_out);   
		iu = fgetc(u_out);   
		iv = fgetc(v_out);   
		//iv=0; // iv is wrong, remove it for now!
		r = (1.1644 * iy)                + (1.5960 * iv);
		g = (1.1644 * iy) - (0.3918 * iu) - (0.8130 * iv);
		b = (1.1644 * iy) + (2.0172 * iu);

		/*r = iy + 1.371*iv;
		g = y - 0.699*iu-0.337*iv;
		b = y + 1.733*iv;*/
		printf("%d %d %d\n",r,g,b);

		if(r<0)r=0;
		if(g<0)g=0;
		if(b<0)b=0;
		if(r>255)r=255;
		if(g>255)g=255;
		if(b>255)b=255;

		fputc(r &0xff, rgb_out);
		fputc(g &0xff, rgb_out);
		fputc(b &0xff, rgb_out);
	}
	fclose(y_out); fclose(u_out); fclose(v_out); fclose(rgb_out);
	return 0;
}

