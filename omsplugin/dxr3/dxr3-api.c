#include <unistd.h>
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

#include "dxr3-api.h"

dxr3_state_t state = { -1,-1,-1,-1,0,0 };

static int _dxr3_install_microcode (char *ucode);

int dxr3_open(char *devname, char *ucodefile)
{ 
    char tmpstr[100];

    if(state.open)
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
    
    state.open=1;

    // current solution for audio output selection - until we move to the new oms architecture - Ze'ev Maor 05-09-2000
#ifdef ANALOG
    state.audiomode = DXR3_AUDIOMODE_ANALOG;
#elif defined DIGITAL
    state.audiomode = DXR3_AUDIOMODE_DIGITALAC3;
#else
#error "*** Set the DXR3_AUDIO_OUTPUT variable in Makefile.am to either ANALOG or DIGITAL ***"
#endif

	     
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
}

int dxr3_get_status()
{
    if(!state.open)
	return DXR3_STATUS_CLOSED;
    else 
	return DXR3_STATUS_OPENED;
}

int dxr3_video_write(char *buf, int n)
{
    return write(state.fd_video, buf, n);
}

int dxr3_subpicture_write(char *buf, int n)
{
    return write(state.fd_spu, buf, n);
}

int dxr3_audio_write(char *buf, int n)
{
    // Fixme: In digital mode the data should be packeted in sub frames
    //	      like it's done in Sebastien Djinn Grosland's ac3spdif
    return write(state.fd_audio, buf, n);
}

int dxr3_video_set_pts(long pts)
{
    return ioctl(state.fd_video, EM8300_IOCTL_VIDEO_SETPTS,pts);
}

int dxr3_subpic_set_pts(long pts)
{
    return ioctl(state.fd_spu, EM8300_IOCTL_SPU_SETPTS,pts);
}

int dxr3_audio_set_pts(long pts)
{
    return ioctl(state.fd_audio, EM8300_IOCTL_AUDIO_SETPTS,&pts);
}

int dxr3_audio_set_mode(mode)
{
    state.audiomode=mode;
    return 0;
}

int dxr3_audio_set_stereo(int val)
{
    if(state.audiomode != DXR3_AUDIOMODE_ANALOG)
	return -1;
    
    return ioctl(state.fd_audio,SNDCTL_DSP_STEREO,&val);
}

int dxr3_audio_set_rate(int rate)
{
    if(state.audiomode == DXR3_AUDIOMODE_DIGITALAC3)
	return -1;
    
    return ioctl(state.fd_audio,SNDCTL_DSP_SPEED,&rate);
}

int dxr3_audio_set_samplesize(int val)
{
    if(state.audiomode == DXR3_AUDIOMODE_DIGITALAC3)
	return -1;
    
    return ioctl(state.fd_audio,SNDCTL_DSP_SAMPLESIZE,&val);
}

int dxr3_subpic_setpalette(char *palette)
{
    return ioctl(state.fd_spu, EM8300_IOCTL_SPU_SETPALETTE,palette);
}

int dx3_video_enabletvout(int val)
{
    return 0;
}

int dx3_video_enableoverlay(int val)
{
    return 0;
}

int dx3_video_set_tvmode(int mode)
{
    return ioctl(state.fd_control, EM8300_IOCTL_SET_VIDEOMODE,&mode);
}

int dx3_video_set_aspectratio(int value)
{
    return ioctl(state.fd_control, EM8300_IOCTL_SET_ASPECTRATIO,&value);
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

