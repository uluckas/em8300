#ifndef __DXR3_API
#define __DXR3_API

#include <sys/ioctl.h>
#include <em8300.h>

#define DXR3_STATUS_CLOSED 0
#define DXR3_STATUS_OPENED 1
#define DXR3_STATUS_MICROCODE_LOADED 2

#define DXR3_AUDIOMODE_ANALOG 0
#define DXR3_AUDIOMODE_DIGITALAC3 1
#define DXR3_AUDIOMODE_DIGITALPCM 2  

#define DXR3_TVMODE_PAL EM8300_VIDEOMODE_PAL
#define DXR3_TVMODE_PAL60 EM8300_VIDEOMODE_PAL60
#define DXR3_TVMODE_NTSC EM8300_VIDEOMODE_NTSC

#define DXR3_OVERLAY_MODE_OFF EM8300_OVERLAY_MODE_OFF
#define DXR3_OVERLAY_MODE_RECTANGLE EM8300_OVERLAY_MODE_RECTANGLE
#define DXR3_OVERLAY_MODE_OVERLAY EM8300_OVERLAY_MODE_OVERLAY

#define DXR3_SPU_MODE_OFF EM8300_SPUMODE_OFF
#define DXR3_SPU_MODE_ON EM8300_SPUMODE_ON

#define DXR3_ASPECTRATIO_4_3 EM8300_ASPECTRATIO_4_3
#define DXR3_ASPECTRATIO_16_9 EM8300_ASPECTRATIO_16_9
#define DXR3_ASPECTRATIO_LAST EM8300_ASPECTRATIO_LAST

#define DXR3_PLAYMODE_STOPPED         0
#define DXR3_PLAYMODE_PAUSED          1
#define DXR3_PLAYMODE_SLOWFORWARDS    2
#define DXR3_PLAYMODE_SLOWBACKWARDS   3
#define DXR3_PLAYMODE_SINGLESTEP      4
#define DXR3_PLAYMODE_PLAY            5
#define DXR3_PLAYMODE_REVERSEPLAY     6
#define DXR3_PLAYMODE_SCAN            7

typedef struct
{
	int fd_control;
	int fd_video;
	int fd_audio;
	int fd_spu;
	
	int open;
	int audiomode;
        int spumode;

	em8300_bcs_t bcs;
	em8300_bcs_t orig_bcs;
	em8300_bcs_t min_bcs;
	em8300_bcs_t max_bcs;
} dxr3_state_t;

// Driver Init and close functions
extern int dxr3_open(char *devname, char *ucodefile);
extern int dxr3_close();
extern int dxr3_install_microcode(em8300_microcode_t *uCode);

// Driver status functions
extern int dxr3_get_status();

// Data IO functions
extern int dxr3_video_write(const char *buf, int n);
extern int dxr3_audio_write(const char *buf, int n);
extern int dxr3_subpic_write(const char *buf, int n);

// Timestamp related functions
extern int dxr3_video_set_pts(long);
extern int dxr3_audio_set_pts(long);
extern int dxr3_subpic_set_pts(long);

// Audio related functions
extern int dxr3_audio_set_mode(int);
extern int dxr3_audio_get_mode(void);
extern int dxr3_audio_set_stereo(int);
extern int dxr3_audio_set_rate(int);
extern int dxr3_audio_set_samplesize(int);

// Video related functions
extern int dxr3_video_set_overlaymode(int);
extern int dxr3_video_set_tvmode(int);
extern int dxr3_video_set_aspectratio(int);
extern int dxr3_video_set_bcs(em8300_bcs_t *bcs);
extern int dxr3_video_get_bcs(em8300_bcs_t *bcs);
extern int dxr3_video_set_overlay_attributes(int, int, int, int, int);
extern int dxr3_video_set_overlay_screen(int, int);
extern int dxr3_video_set_overlay_window(int, int, int, int);
extern int dxr3_video_set_overlay_keycolor(int, int);
extern int dxr3_video_set_overlay_signalmode(int);

// Subpic related functions
extern int dxr3_subpic_set_palette(char *palette);
extern int dxr3_subpic_set_mode(int);
extern int dxr3_subpic_get_mode(void);

// Play control functions
extern int dxr3_set_playmode(int);

#endif
