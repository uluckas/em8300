/*****************************************************************************/

/*
 *      allblackbut.c -- generic functions to make all the screen black with 
 *      small patterns for autocalibration. Big modification of xlock-2.3 by 
 *      Emmanuel Michon <emmanuel_michon@sdesigns.com>.
 *

 Copyright (c) 1988-91 by Patrick J. Naughton.

 Permission to use, copy, modify, and distribute this software and its
 documentation for any purpose and without fee is hereby granted,
 provided that the above copyright notice appear in all copies and that
 both that copyright notice and this permission notice appear in
 supporting documentation.

 This file is provided AS IS with no warranties of any kind.  The author
 shall have no liability with respect to the infringement of copyrights,
 trade secrets or any patents by this file or any part thereof.  In no
 event will the author be liable for any lost revenue or profits or
 other special, indirect and consequential damages. 

 */

/*****************************************************************************/

// os stuff
#include <stdio.h>
#include <unistd.h>

// X stuff (XOpenDisplay)
#include <X11/Xlib.h>

#include "autocal.h"


#include "allblackbut.h"

// sorry, X redefines BYTE and BOOL, conflicting with fmp types.
// header from X4.0.1
Bool XF86VidModeSetViewPort(Display *display,int screen,int x,int y);

#define AllPointerEventMask \
	(ButtonPressMask | ButtonReleaseMask | \
	EnterWindowMask | LeaveWindowMask | \
	PointerMotionMask | PointerMotionHintMask | \
	Button1MotionMask | Button2MotionMask | \
	Button3MotionMask | Button4MotionMask | \
	Button5MotionMask | ButtonMotionMask | \
	KeymapStateMask)

Window win;
Screen *Scr;

GC gc; /* graphics context for animation */

u_long pixel[NUMCOLORS];

Window root;
Colormap cmap;

XSetWindowAttributes xswa;
XGCValues xgcv;

Cursor mycursor;		/* blank cursor */
Pixmap lockc;
Pixmap lockm;		/* pixmaps for cursor and mask */
char no_bits[] = {0};	/* dummy array for the blank cursor */
XColor      somecolor;

void AllBlackButClose()
{
	XSync(dpy, False);

	XDestroyWindow(dpy,win);

	XUngrabPointer(dpy, CurrentTime);
	XFlush(dpy);
}

void AllBlackButInit()
{
	Scr = DefaultScreenOfDisplay(dpy);

	cmap = DefaultColormapOfScreen(Scr);
	
	root = RootWindowOfScreen(Scr);
	
#define GETRGBPIX(R,G,B,TARGET) 				\
		somecolor.red = R << 8;				\
		somecolor.green = G << 8;			\
		somecolor.blue = B << 8;			\
		somecolor.flags = DoRed | DoGreen | DoBlue;	\
		if (!XAllocColor(dpy, cmap, &somecolor)) _exit(1);	\
		TARGET = somecolor.pixel;

	GETRGBPIX(0,0,0,pixel[MY_BLACK]);
	GETRGBPIX(255,255,255,pixel[MY_WHITE]);
	GETRGBPIX(0,0,128,pixel[MY_DEEPBLUE]);
	GETRGBPIX(0,0,255,pixel[MY_FULLBLUE]);
	GETRGBPIX(128,128,128,pixel[MY_GREY]);

	xswa.override_redirect = True;
	xswa.background_pixel = pixel[MY_BLACK];
	
	xswa.event_mask = KeyPressMask | ButtonPressMask | VisibilityChangeMask;
	
#define WIDTH WidthOfScreen(Scr)
#define HEIGHT HeightOfScreen(Scr)
#define CWMASK CWOverrideRedirect | CWBackPixel | CWEventMask
	
	win = XCreateWindow(dpy, root, 0, 0, WIDTH, HEIGHT, 0,
			    CopyFromParent, InputOutput, CopyFromParent,
			    CWMASK, &xswa);
	
	xswa.border_pixel = pixel[MY_BLACK];
	xswa.background_pixel = pixel[MY_WHITE];
	xswa.event_mask = ButtonPressMask;
	
#define CIMASK CWBorderPixel | CWBackPixel | CWEventMask
	
	XMapWindow(dpy, win);
	XRaiseWindow(dpy, win);
	
	xgcv.foreground = pixel[MY_WHITE];
	xgcv.background = pixel[MY_BLACK];
	
	gc = XCreateGC(dpy, win,
			   GCForeground | GCBackground, &xgcv);
	
	// blank the cursor
	lockc = XCreateBitmapFromData(dpy, root, no_bits, 1, 1);
	lockm = XCreateBitmapFromData(dpy, root, no_bits, 1, 1);
	mycursor = XCreatePixmapCursor(dpy, lockc, lockm,
				       &somecolor, &somecolor, 0, 0);
	XFreePixmap(dpy, lockc);
	XFreePixmap(dpy, lockm);
	
	XGrabPointer(dpy, win, True, AllPointerEventMask,
		     GrabModeAsync, GrabModeAsync, None, mycursor,
		     CurrentTime);
	XGrabPointer(dpy, win, True, AllPointerEventMask,
		     GrabModeAsync, GrabModeAsync, None, mycursor,
		     CurrentTime);
}
	
void AllBlackButPattern(int my_color,int x,int y,int width,int height)
{
	// rectify the view port offset to (0,0).
	// This command in unsupported and garbles the screen on 
	// Mach64 Rage XL or XC rev 101
	//if (!XF86VidModeSetViewPort(dpy,scr,0,0)) exit(1);
	
	XClearWindow(dpy, win);
	XSetForeground(dpy,gc,pixel[my_color]);
	XFillRectangle(dpy,win,gc,x,y,width,height);
	XFlush(dpy);
	XSync(dpy, False);
}

void AllBlackButPatternRGB(int rgb_color,int x,int y,int width,int height)
{
    u_long color;
    // rectify the view port offset to (0,0).
	// This command in unsupported and garbles the screen on 
	// Mach64 Rage XL or XC rev 101
	//if (!XF86VidModeSetViewPort(dpy,scr,0,0)) exit(1);
	GETRGBPIX(((rgb_color >> 16) & 0xff),((rgb_color >> 8) & 0xff),(rgb_color & 0xff),color);
	

	XClearWindow(dpy, win);
	XSetForeground(dpy,gc,color);
	XFillRectangle(dpy,win,gc,x,y,width,height);
	XFlush(dpy);
	XSync(dpy, False);
}

void AllDrawRectangle(int fg, int bg, int x,int y,int width,int height)
{
    u_long bgcolor,fgcolor;
    // rectify the view port offset to (0,0).
	// This command in unsupported and garbles the screen on 
	// Mach64 Rage XL or XC rev 101
	//if (!XF86VidModeSetViewPort(dpy,scr,0,0)) exit(1);
	GETRGBPIX(((fg >> 16) & 0xff),((fg >> 8) & 0xff),(fg & 0xff),fgcolor);
	GETRGBPIX(((bg >> 16) & 0xff),((bg >> 8) & 0xff),(bg & 0xff),bgcolor);
	

	XClearWindow(dpy, win);
	XSetForeground(dpy,gc,bgcolor);
	XFillRectangle(dpy,win,gc,0,0,1280,1024);
	XSetForeground(dpy,gc,fgcolor);
	XFillRectangle(dpy,win,gc,x,y,width,height);
	XFlush(dpy);
	XSync(dpy, False);
}
