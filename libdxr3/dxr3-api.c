#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include "dxr3-api.h"
#if defined(__OpenBSD__)
#include <soundcard.h>
#elif defined(__FreeBSD__)
#include <machine/soundcard.h>
#else
#include <sys/soundcard.h>
#endif
#include <sys/ioctl.h>

dxr3_state_t state = { -1,-1,-1,-1,0,0 };

static int _dxr3_install_microcode (char *ucode);

int dxr3_open(char *devname, char *ucodefile)
{ 
	char tmpstr[100];

	if (state.open)
		return -1;

	printf("dxr3_open(devname=%s)\n",devname);
    
	// open device
	if ((state.fd_control = open (devname, O_WRONLY)) < 0) {
		perror(devname);
		return -1;
	}
    
	// Upload microcode
	if(_dxr3_install_microcode(ucodefile)) {
		fprintf(stderr,"Microcode upload failed.\n");
		return -1;
	}
	
	// open video device
	snprintf (tmpstr, sizeof(tmpstr), "%s_mv", devname);
	if ((state.fd_video = open (tmpstr, O_WRONLY)) < 0) {
		perror(tmpstr);
		return -1;
	}

	// open spu device
	snprintf (tmpstr, sizeof(tmpstr), "%s_sp", devname);
	if ((state.fd_spu = open (tmpstr, O_WRONLY)) < 0) {
		perror(tmpstr);
		return -1;
	}
	
	// open audio device
	snprintf (tmpstr, sizeof(tmpstr), "%s_ma", devname);
	if ((state.fd_audio = open (tmpstr, O_WRONLY)) < 0) {
		perror(tmpstr);
		return -1;
	}
	
	state.open = 1;
	
	dxr3_audio_get_mode();
	dxr3_video_get_bcs(NULL);
	state.orig_bcs = state.bcs;
	state.min_bcs.brightness = 0;
	state.min_bcs.contrast = 0;
	state.min_bcs.saturation = 0;
	state.max_bcs.brightness = 1000;
	state.max_bcs.contrast = 1000;
	state.max_bcs.saturation = 1000;

	return 0;
}

int dxr3_close()
{
	if(state.open) {
		close(state.fd_control);
		close(state.fd_video);
		close(state.fd_audio);
		close(state.fd_spu);
		
		state.open=0;
	}
	return 0;
}

int dxr3_get_status()
{
	if (!state.open)
		return DXR3_STATUS_CLOSED;
	else
		return DXR3_STATUS_OPENED;
}

int dxr3_video_write(const char *buf, int n)
{
	return write(state.fd_video, buf, n);
}

int dxr3_subpic_write(const char *buf, int n)
{
	return write(state.fd_spu, buf, n);
}

int dxr3_audio_write(const char *buf, int n)
{
	// Fixme: In digital mode the data should be packeted in sub frames
	//	      like it's done in Sebastien Djinn Grosland's ac3spdif
	return write(state.fd_audio, buf, n);
}

int dxr3_video_set_pts(long pts)
{
	return ioctl(state.fd_video, EM8300_IOCTL_VIDEO_SETPTS, &pts);
}

int dxr3_subpic_set_pts(long pts)
{
	return ioctl(state.fd_spu, EM8300_IOCTL_SPU_SETPTS, &pts);
}

int dxr3_audio_set_pts(long pts)
{
	return ioctl(state.fd_audio, EM8300_IOCTL_AUDIO_SETPTS, &pts);
}


int dxr3_audio_set_mode(mode)
{
	state.audiomode = mode;
	return ioctl(state.fd_control, EM8300_IOCTL_SET_AUDIOMODE, &state.audiomode);
}

int dxr3_audio_get_mode()
{
        return ioctl(state.fd_control, EM8300_IOCTL_GET_AUDIOMODE, &state.audiomode);
}

int dxr3_audio_set_stereo(int val)
{
	if(state.audiomode == DXR3_AUDIOMODE_DIGITALAC3)
		return -1;

	return ioctl(state.fd_audio, SNDCTL_DSP_STEREO, &val);
}

int dxr3_audio_set_rate(int rate)
{
	if(state.audiomode == DXR3_AUDIOMODE_DIGITALAC3)
		return -1;

	return ioctl(state.fd_audio, SNDCTL_DSP_SPEED, &rate);
}

int dxr3_audio_set_samplesize(int val)
{
	if(state.audiomode == DXR3_AUDIOMODE_DIGITALAC3)
		return -1;

	return ioctl(state.fd_audio, SNDCTL_DSP_SAMPLESIZE, &val);
}

/* probably move this to the driver eventually */
static void swab_clut(char* clut)
{
	int i;
	char tmp;
	i = 0;
	while (i < 16*4) {
		tmp = clut[i];
		clut[i] = clut[i+3];
		clut[i+3] = tmp;
		tmp = clut[i+1];
		clut[i+1] = clut[i+2];
		clut[i+2] = tmp;
		i += 4;
	}
}

int dxr3_subpic_set_palette(char *palette)
{
        char pal[16*4];
        memcpy(pal, palette, 16*4);
        swab_clut(pal);
	return ioctl(state.fd_spu, EM8300_IOCTL_SPU_SETPALETTE, pal);
}

int dxr3_subpic_set_mode(int mode)
{
	state.spumode = mode;
	return ioctl(state.fd_control, EM8300_IOCTL_SET_SPUMODE, &state.spumode);
}


int dxr3_subpic_get_mode(void)
{
	return -1;
#if 0
        return ioctl(state.fd_control, EM8300_IOCTL_GET_SPUMODE, &state.spumode);
#endif
}

int dxr3_video_set_overlaymode(int mode)
{
	return ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SETMODE, &mode);
}

int dxr3_video_set_tvmode(int mode)
{
	return ioctl(state.fd_control, EM8300_IOCTL_SET_VIDEOMODE, &mode);
}

int dxr3_video_set_aspectratio(int value)
{
	return ioctl(state.fd_control, EM8300_IOCTL_SET_ASPECTRATIO, &value);
}

int dxr3_video_set_bcs(em8300_bcs_t *bcs)
{
	if (bcs)
		return ioctl(state.fd_control, EM8300_IOCTL_SETBCS, bcs);
	else
		return ioctl(state.fd_control, EM8300_IOCTL_SETBCS, &state.bcs);
}

int dxr3_video_get_bcs(em8300_bcs_t *bcs)
{
	if (bcs)
		return ioctl(state.fd_control, EM8300_IOCTL_GETBCS, bcs);
	else
		return ioctl(state.fd_control, EM8300_IOCTL_GETBCS, &state.bcs);
}

int dxr3_video_set_overlay_attributes(int xoffset, int yoffset, int xcorr, int stability, int jitter)
{
	em8300_attribute_t attr;
	attr.attribute = EM9010_ATTRIBUTE_XOFFSET;
	attr.value = xoffset;
	if(ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SET_ATTRIBUTE, &attr) == -1)
		return -1;
	attr.attribute = EM9010_ATTRIBUTE_YOFFSET;
	attr.value = yoffset;
	if(ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SET_ATTRIBUTE, &attr) == -1)
		return -1;
	attr.attribute = EM9010_ATTRIBUTE_XCORR;
	attr.value = xcorr;
	if(ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SET_ATTRIBUTE, &attr) == -1)
		return -1;
	attr.attribute = EM9010_ATTRIBUTE_STABILITY;
	attr.value = stability;
	if(ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SET_ATTRIBUTE, &attr) == -1)
		return -1;
	attr.attribute = EM9010_ATTRIBUTE_JITTER;
	attr.value = jitter;
	return ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SET_ATTRIBUTE, &attr);
}

int dxr3_video_set_overlay_screen(int xsize, int ysize)
{
	em8300_overlay_screen_t scr;
	scr.xsize = xsize;
	scr.ysize = ysize;
	return ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SETSCREEN, &scr);
}

int dxr3_video_set_overlay_window(int xpos, int ypos, int width, int height)
{
	em8300_overlay_window_t win;
	win.xpos = xpos;
	win.ypos = ypos;
	win.width = width;
	win.height = height;
	return ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SETWINDOW, &win);
}

int dxr3_video_set_overlay_keycolor(int lower, int upper)
{
	int ret;
	em8300_attribute_t attr;
	attr.attribute = EM9010_ATTRIBUTE_KEYCOLOR_LOWER;
	attr.value = lower;
	ret = ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SET_ATTRIBUTE, &attr);
	if(ret == -1) return -1;
	attr.attribute = EM9010_ATTRIBUTE_KEYCOLOR_UPPER;
	attr.value = upper;
	return ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SET_ATTRIBUTE, &attr);
}

int dxr3_video_set_overlay_signalmode(int mode)
{
	return ioctl(state.fd_control, EM8300_IOCTL_OVERLAY_SIGNALMODE, &mode);
}

int dxr3_set_playmode(int playmode)
{
	return ioctl(state.fd_control, EM8300_IOCTL_SET_PLAYMODE,&playmode);
}

static int _dxr3_install_microcode (char *ucode)
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
	return ioctl(state.fd_control, EM8300_IOCTL_INIT, &uCode);
}