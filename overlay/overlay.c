#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <malloc.h>
#include <sys/ioctl.h>

#include <linux/em8300.h>

#include "overlay.h"

#define MATLAB_OUTPUT

static int update_parameters(overlay_t *o)
{
    overlay_set_attribute(o, EM9010_ATTRIBUTE_XOFFSET, o->xoffset);
    overlay_set_attribute(o, EM9010_ATTRIBUTE_YOFFSET, o->yoffset);
    overlay_set_attribute(o, EM9010_ATTRIBUTE_XCORR, o->xcorr);
    overlay_set_attribute(o, EM9010_ATTRIBUTE_STABILITY, o->stability);
    overlay_set_attribute(o, EM9010_ATTRIBUTE_JITTER, o->jitter);
}

int overlay_set_attribute(overlay_t *o, int attribute, int value)
{
    em8300_attribute_t attr;
    
    attr.attribute = attribute;
    attr.value = value;
    if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_SET_ATTRIBUTE, &attr)==-1)
        {
	     perror("Failed set attribute");
	     return -1;
        }

    return 0;
}

overlay_t *overlay_init(FILE *dev)
{
    overlay_t *o;

    o = (overlay_t *) malloc(sizeof(overlay_t));

    if(!o)
	return NULL;

    memset(o,sizeof(overlay_t),0);

    o->fp = dev;
    o->dev = fileno(dev);
    o->xres = 1280; o->yres=1024; o->xcorr=1000;
    o->color_interval=10;

    return o;
}

int overlay_release(overlay_t *o)
{
    if(o)
	free(o);

    return 0;
}
#define TYPE_INT 1
#define TYPE_XINT 2
#define TYPE_COEFF 3
#define TYPE_FLOAT 4

struct lut_entry {
    char *name;
    int type;
    void *ptr;
};

static struct lut_entry *new_lookuptable(overlay_t *o)
{
    struct lut_entry m[] = {
	{"xoffset", TYPE_INT, &o->xoffset},
	{"yoffset", TYPE_INT, &o->yoffset},
	{"xcorr", TYPE_INT, &o->xcorr},
	{"jitter", TYPE_INT, &o->jitter},
	{"stability", TYPE_INT, &o->stability},
	{"keycolor", TYPE_XINT, &o->keycolor},
	{"colcal_upper", TYPE_COEFF, &o->colcal_upper[0]},
	{"colcal_lower", TYPE_COEFF, &o->colcal_lower[0]},
	{"color_interval", TYPE_FLOAT, &o->color_interval},
	{0,0,0}
    },*p;

    p = malloc(sizeof(m));
    memcpy(p,m,sizeof(m));
    return p;
}

int lookup_parameter(overlay_t *o, struct lut_entry *lut, char *name, void **ptr, int *type) {
    int i;

    for(i=0; lut[i].name; i++) {
	if(!strcmp(name,lut[i].name)) {
	    *ptr = lut[i].ptr;
	    *type = lut[i].type;
	    return 1;
	}
    }
    return 0;
}

int overlay_read_state(overlay_t *o, char *p)
{
    char *a,*tok;
    char path[128],fname[128],tmp[128],line[256];
    FILE *fp;
    struct lut_entry *lut;
    void *ptr;
    int type;
    int j;
	
    if(!p) {
	strcpy(fname,getenv("HOME"));
	strcat(fname,"/.overlay");	    
    } else
	strcpy(fname,p);
    
    sprintf(tmp,"/res_%dx%dx%d",o->xres,o->yres,o->depth);
    strcat(fname,tmp);

    if(!(fp=fopen(fname,"r")))
	return -1;

    lut = new_lookuptable(o);
    
    while(!feof(fp)) {
	if(!fgets(line,256,fp))
	    break;
	tok=strtok(line," ");
	if(lookup_parameter(o,lut,tok,&ptr,&type)) {
	    tok=strtok(NULL," ");
	    switch(type) {
	    case TYPE_INT:
		sscanf(tok,"%d",(int *)ptr);
		break;
	    case TYPE_XINT:
		sscanf(tok,"%x",(int *)ptr);
		break;
	    case TYPE_FLOAT:
		sscanf(tok,"%f",(float *)ptr);
		break;
	    case TYPE_COEFF:
		for(j=0;j<3;j++) {
		    sscanf(tok,"%f",&((struct coeff *)ptr)[j].k);
		    tok=strtok(NULL," ");
		    sscanf(tok,"%f",&((struct coeff *)ptr)[j].m);
		    tok=strtok(NULL," ");
		}
		break;	    
	    }
	    
	}	
    }

    update_parameters(o);
    
    free(lut);
    fclose(fp);
    return 0;
}

void overlay_update_params(overlay_t *o) {
    update_parameters(o);
}

int overlay_write_state(overlay_t *o, char *p)	
{
    char *a;
    char path[128],fname[128],tmp[128];
    FILE *fp;
    char line[256],*tok;
    struct lut_entry *lut;
    int i,j;
	
    if(!p) {
	strcpy(fname,getenv("HOME"));
	strcat(fname,"/.overlay");	    
    } else
	strcpy(fname,p);

    if(access(fname, W_OK|X_OK|R_OK)) {
	if(mkdir(fname,0766))
	    return -1;
    }	
    
    sprintf(tmp,"/res_%dx%dx%d",o->xres,o->yres,o->depth);
    strcat(fname,tmp);
    
    if(!(fp=fopen(fname,"w")))
	return -1;
    
    lut = new_lookuptable(o);

    for(i=0; lut[i].name; i++) {	
	fprintf(fp,"%s ",lut[i].name);
	switch(lut[i].type) {
	case TYPE_INT:
	    fprintf(fp,"%d\n",*(int *)lut[i].ptr);
	    break;
	case TYPE_XINT:
	    fprintf(fp,"%06x\n",*(int *)lut[i].ptr);
	    break;
	case TYPE_FLOAT:
	    fprintf(fp,"%f\n",*(float *)lut[i].ptr);
	    break;
	case TYPE_COEFF:
	    for(j=0;j<3;j++) 
		fprintf(fp,"%f %f ",((struct coeff *)lut[i].ptr)[j].k,
			((struct coeff *)lut[i].ptr)[j].m);
	    fprintf(fp,"\n");
	    break;	    
	}
    }

    fclose(fp);
    return 0;
}

int overlay_set_screen(overlay_t *o, int xres, int yres, int depth)
{
   em8300_overlay_screen_t scr;

   o->xres = xres;
   o->yres = yres;
   o->depth = depth;

   scr.xsize = xres;
   scr.ysize = yres;

   if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_SETSCREEN, &scr)==-1)
        {
            perror("Failed set screen...exiting");
            return -1;
	}
   return 0;
}

int overlay_set_mode(overlay_t *o, int mode)
{
    if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_SETMODE, &mode)==-1) {
	perror("Failed enabling overlay..exiting");
	return -1;
    }
    return 0;
}

int overlay_set_window(overlay_t *o, int xpos,int ypos,int width,int height)
{
    em8300_overlay_window_t win;
    win.xpos = xpos;
    win.ypos = ypos;
    win.width = width;
    win.height = height;

    if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_SETWINDOW, &win)==-1)
        {
            perror("Failed resizing window");
            return -1;
        }
    return 0;
}

static int col_interp(float x, struct coeff c)
{
    float y;
    y = x*c.k + c.m;
    if(y > 255)
	y = 255;
    if(y < 0)
	y = 0;
    return rint(y);
}

int overlay_set_keycolor(overlay_t *o, int color) {
    int r = (color & 0xff0000) >> 16;
    int g = (color & 0x00ff00) >> 8;
    int b = (color & 0x0000ff);
    float ru,gu,bu;
    float rl,gl,bl;
    int upper,lower;

    ru = r+o->color_interval;
    gu = g+o->color_interval;
    bu = b+o->color_interval;

    rl = r-o->color_interval;
    gl = g-o->color_interval;
    bl = b-o->color_interval;
    
    upper = (col_interp(ru, o->colcal_upper[0]) << 16) |
	    (col_interp(gu, o->colcal_upper[1]) << 8) |
	    (col_interp(bu, o->colcal_upper[2]));

    lower = (col_interp(rl, o->colcal_lower[0]) << 16) |
	    (col_interp(gl, o->colcal_lower[1]) << 8) |
	    (col_interp(bl, o->colcal_lower[2]));

    printf("0x%06x 0x%06x\n",upper,lower);
    overlay_set_attribute(o,EM9010_ATTRIBUTE_KEYCOLOR_UPPER,upper);
    overlay_set_attribute(o,EM9010_ATTRIBUTE_KEYCOLOR_LOWER,lower);
}

static void least_sq_fit(int *x, int *y, int n, float *k, float *m)
{
    float sx=0,sy=0,sxx=0,sxy=0;
    float delta,b;
    int i;

    for(i=0; i < n; i++) {
	sx=sx+x[i];
	sy=sy+y[i];
	sxx=sxx+x[i]*x[i];
	sxy=sxy+x[i]*y[i];
    }	

    delta=sxx*n-sx*sx;

    *m=(sxx*sy-sx*sxy)/delta;
    *k=(sxy*n-sx*sy)/delta;
}

int overlay_autocalibrate(overlay_t *o, pattern_drawer_cb pd, void *arg)
{
    em8300_overlay_calibrate_t cal;
    em8300_overlay_window_t win;
    int x[256],r[256],g[256],b[256],n;
    float k,m;
    
#ifdef MATLAB_OUTPUT
    FILE *f;
#endif
    int i;

    o->draw_pattern=pd;
    o->dp_arg = arg;
    
    overlay_set_mode(o, EM8300_OVERLAY_MODE_OVERLAY);
    overlay_set_screen(o, o->xres, o->yres, o->depth);

    /* Calibrate Y-offset */
    
    o->draw_pattern(0x0000ff, 0, 0, 0, 355, 1, o->dp_arg);

    cal.cal_mode = EM8300_OVERLAY_CALMODE_YOFFSET; 
    if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_CALIBRATE, &cal))
        {
	    perror("Failed getting Yoffset values...exiting");
	    return -1;
        }
    o->yoffset = cal.result;
    printf("Yoffset: %d\n",cal.result);

    /* Calibrate X-offset */

    o->draw_pattern(0x0000ff, 0, 0, 0, 2, 288, o->dp_arg);

    cal.cal_mode = EM8300_OVERLAY_CALMODE_XOFFSET;  
    if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_CALIBRATE, &cal)) 
	{
 	    perror("Failed getting Xoffset values...exiting"); 
 	    return -1;
	} 
    o->xoffset = cal.result;
    printf("Xoffset: %d\n",cal.result); 

    /* Calibrate X scale correction */
    
    o->draw_pattern(0x0000ff, 0, 355, 0, 2, 288, o->dp_arg);

    cal.cal_mode = EM8300_OVERLAY_CALMODE_XCORRECTION;  
    if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_CALIBRATE, &cal)) 
	{
 	    perror("Failed getting Xoffset values...exiting"); 
 	    return -1;
	} 
    printf("Xcorrection: %d\n",cal.result);
    o->xcorr = cal.result;

    win.xpos = 0;
    win.ypos = 0;
    win.width = o->xres;
    win.height = o->yres;
    if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_SETWINDOW, &win)==-1) {	
	perror("Failed resizing window");
	exit(1);
    }

    /* Calibrate key color upper limit */
    
#ifdef MATLAB_OUTPUT
    f=fopen("upper.mat","w");
#endif
    for(i=128,n=0; i <= 0xff; i+=4) {
	o->draw_pattern(i | (i << 8) | (i << 16), 0,
			(o->xres-200)/2,0,200,o->yres,o->dp_arg);
    	
	cal.arg = i;
	cal.arg2 = 1;
	cal.cal_mode = EM8300_OVERLAY_CALMODE_COLOR;
    
	if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_CALIBRATE, &cal)) 
	    {
#ifdef MATLAB_OUTPUT
		fclose(f);
#endif
		return -1 ;
	    }
#ifdef MATLAB_OUTPUT	
	fprintf(f,"%d %d %d %d\n",i,
		(cal.result>>16)&0xff,
		(cal.result>>8)&0xff,
		(cal.result)&0xff);
#endif

	x[n] = i;
	r[n] = (cal.result>>16)&0xff;
	g[n] = (cal.result>>8)&0xff;
	b[n] = (cal.result)&0xff;
	n++;
    }
#ifdef MATLAB_OUTPUT
    fclose(f);
#endif

    least_sq_fit(x,r,n,&o->colcal_upper[0].k,&o->colcal_upper[0].m);
    least_sq_fit(x,g,n,&o->colcal_upper[1].k,&o->colcal_upper[1].m);
    least_sq_fit(x,b,n,&o->colcal_upper[2].k,&o->colcal_upper[2].m);

    /* Calibrate key color lower limit */

#ifdef MATLAB_OUTPUT
    f=fopen("lower.mat","w");
#endif
    for(i=128,n=0; i <= 0xff; i+=4) {
	o->draw_pattern(i | (i << 8) | (i << 16), 0xffffff,
			(o->xres-200)/2,0,200,o->yres, o->dp_arg);
    	
	cal.arg = i;
	cal.arg2 = 2;
	cal.cal_mode = EM8300_OVERLAY_CALMODE_COLOR;
    
	if (ioctl(o->dev, EM8300_IOCTL_OVERLAY_CALIBRATE, &cal)) 
	    {
#ifdef MATLAB_OUTPUT
		fclose(f);
#endif
		return -1 ;
	    }
#ifdef MATLAB_OUTPUT	
	fprintf(f,"%d %d %d %d\n",i,
		(cal.result>>16)&0xff,
		(cal.result>>8)&0xff,
		(cal.result)&0xff);
#endif	
	x[n] = i;
	r[n] = (cal.result>>16)&0xff;
	g[n] = (cal.result>>8)&0xff;
	b[n] = (cal.result)&0xff;
	n++;
    }

#ifdef MATLAB_OUTPUT
    fclose(f);
#endif

    least_sq_fit(x,r,n,&o->colcal_lower[0].k,&o->colcal_lower[0].m);
    least_sq_fit(x,g,n,&o->colcal_lower[1].k,&o->colcal_lower[1].m);
    least_sq_fit(x,b,n,&o->colcal_lower[2].k,&o->colcal_lower[2].m);

    overlay_set_mode(o, EM8300_OVERLAY_MODE_OFF);

    return 0;
}


int overlay_signalmode(overlay_t *o, int mode) {
	if(ioctl(o->dev, EM8300_IOCTL_OVERLAY_SIGNALMODE, &mode) ==-1) {
	    perror("Failed set signal mix");
	    return -1;
	}
	return 0;
}
