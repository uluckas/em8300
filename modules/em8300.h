
typedef struct {
    void *ucode;
    int ucode_size;
} em8300_microcode_t;

typedef struct {
    int reg;
    int val;
    int microcode_register;
} em8300_register_t;

typedef struct {
    int brightness;
    int contrast;
    int saturation;
} em8300_bcs_t;

#define EM8300_IOCTL_INIT       _IOW('C',0,em8300_microcode_t)
#define EM8300_IOCTL_READREG    _IOWR('C',1,em8300_register_t)
#define EM8300_IOCTL_WRITEREG   _IOW('C',2,em8300_register_t)
#define EM8300_IOCTL_GETSTATUS  _IOR('C',3,char[1024])
#define EM8300_IOCTL_SETBCS	_IOW('C',4,em8300_bcs_t)
#define EM8300_IOCTL_GETBCS	_IOR('C',4,em8300_bcs_t)
#define EM8300_IOCTL_SET_ASPECTRATIO _IOW('C',5,int)
#define EM8300_IOCTL_GET_ASPECTRATIO _IOR('C',5,int)

#define EM8300_IOCTL_VIDEO_SETPTS 1
#define EM8300_IOCTL_SPU_SETPTS 1
#define EM8300_IOCTL_SPU_SETPALETTE 2
#define EM8300_IOCTL_AUDIO_SETPTS _SIOWR('P', 31, int)
#define EM8300_IOCTL_AUDIO_SYNC _SIO('P', 30)

#define EM8300_ASPECTRATIO_3_2 0
#define EM8300_ASPECTRATIO_16_9 1

#define EM8300_VIDEOMODE_PAL 0
#define EM8300_VIDEOMODE_PAL60 1
#define EM8300_VIDEOMODE_NTSC 2
#define EM8300_VIDEOMODE_LAST 2
#ifndef EM8300_VIDEOMODE_DEFAULT
#define EM8300_VIDEOMODE_DEFAULT EM8300_VIDEOMODE_NTSC
#endif

#define EM8300_AUDIOMODE_ANALOG 0
#define EM8300_AUDIOMODE_DIGITALAC3 1
#define EM8300_AUDIOMODE_DIGITALPCM 2
#ifndef EM8300_AUDIOMODE_DEFAULT
#define EM8300_AUDIOMODE_DEFAULT EM8300_AUDIOMODE_ANALOG
#endif

#define EM8300_PLAYMODE_STOPPED         0
#define EM8300_PLAYMODE_PAUSED          1
#define EM8300_PLAYMODE_SLOWFORWARDS    2
#define EM8300_PLAYMODE_SLOWBACKWARDS   3
#define EM8300_PLAYMODE_SINGLESTEP      4
#define EM8300_PLAYMODE_PLAY            5
#define EM8300_PLAYMODE_REVERSEPLAY     6
#define EM8300_PLAYMODE_SCAN            7

#define EM8300_SUBDEVICE_CONTROL 0
#define EM8300_SUBDEVICE_VIDEO 1
#define EM8300_SUBDEVICE_AUDIO 2
#define EM8300_SUBDEVICE_SUBPICTURE 3

#ifndef PCI_VENDOR_ID_SIGMADESIGNS
#define PCI_VENDOR_ID_SIGMADESIGNS 0x1105
#define PCI_DEVICE_ID_SIGMADESIGNS_EM8300 0x8300
#endif


#define PTSLAG_LIMIT 45000
#define AUDIO_LAG_LIMIT 10000


#define CLOCKGEN_SAMPFREQ_MASK 0xc0
#define CLOCKGEN_SAMPFREQ_66 0xc0
#define CLOCKGEN_SAMPFREQ_48 0x40
#define CLOCKGEN_SAMPFREQ_44 0x80
#define CLOCKGEN_SAMPFREQ_32 0x00

#define CLOCKGEN_OUTMASK 0x30
#define CLOCKGEN_DIGITALOUT 0x10
#define CLOCKGEN_ANALOGOUT 0x20

#define MVCOMMAND_STOP 0x0
#define MVCOMMAND_11 0x11
#define MVCOMMAND_10 0x10
#define MVCOMMAND_PAUSE 1
#define MVCOMMAND_START 3

#define MACOMMAND_STOP 0x0
#define MACOMMAND_PAUSE 0x1
#define MACOMMAND_PLAY 0x2

#define IRQSTATUS_VIDEO_VBL 0x10
#define IRQSTATUS_VIDEO_FIFO 0x2
#define IRQSTATUS_AUDIO_FIFO 0x8

#define AUDIO_SYNC_INACTIVE 0
#define AUDIO_SYNC_REQUESTED 1
#define AUDIO_SYNC_INPROGRESS 2 

#define ENCODER_ADV7175 1 
#define ENCODER_ADV7170 2

#ifdef __KERNEL__

#include "em8300_reg.h"


#define EM8300_MAX 4

#define EM8300_MAJOR 121
#define EM8300_LOGNAME "em8300"

#define DEBUG(x...) x

#define DICOM_MODE_PAL 1
#define DICOM_MODE_NTSC 2
#define DICOM_MODE_PAL60 3

struct dicom_s {
    int luma;
    int chroma;
    int frametop;
    int framebottom;
    int frameleft;
    int frameright;
    int visibletop;
    int visiblebottom;
    int visibleleft;
    int visibleright;
    int tvout;
};

struct em8300_s
{
    char name[40];

    int chip_revision;
    
    int inuse[4];
    int ucodeloaded;
    
    struct pci_dev *dev;
    ulong adr;
    volatile unsigned *mem;
    ulong memsize;

    /* Fifos */
    struct fifo_s *mvfifo;
    struct fifo_s *mafifo;
    struct fifo_s *spfifo;

    /* DICOM */
    int dicom_vertoffset;
    int dicom_horizoffset;
    int dicom_brightness;
    int dicom_contrast;
    int dicom_saturation;

    /* I2C */
    int i2c_pin_reg;
    int i2c_oe_reg;
    
    /* I2C bus 1*/
    struct i2c_algo_bit_data i2c_data_1;
    struct i2c_adapter i2c_ops_1;

    /* I2C bus 2*/
    struct i2c_algo_bit_data i2c_data_2;
    struct i2c_adapter i2c_ops_2;

    /* I2C clients */
    int encoder_type;
    struct i2c_client *encoder;
    struct i2c_client *eeprom;

    /* Microcode registers */
    unsigned ucode_regs[MAX_UCODE_REGISTER];

    /* Interrupt */
    unsigned irqmask;

    /* Clockgenerator */
    int clockgen;
    
    /* Timing measurement */
    struct timeval tv,last_status_time;
    long irqtimediff;
    int irqcount;
    int frames;
    int scr;

    /* Audio */
    int audio_mode;
    int swapbytes;
    int stereo;
    int audio_ptsvalid;
    int audio_pts;
    int audio_rate;
    int audio_lag;
    int audio_sync,audio_syncpts;
    int audio_first;
    int dword_DB4;
    unsigned char byte_D90[24];

    /* Video */
    int video_mode;
    int videodelay;
    int max_videodelay;
    int aspect_ratio;
    int video_pts,video_ptsvalid,video_offset,video_count;
    int video_ptsfifo_ptr;
#if LINUX_VERSION_CODE < 0x020314    
    struct wait_queue *video_ptsfifo_wait;
#else
    wait_queue_head_t video_ptsfifo_wait;
#endif
    int video_ptsfifo_waiting;
    int video_first;

    /* Sub Picture */
    int sp_pts,sp_ptsvalid,sp_count;
    int sp_ptsfifo_ptr;
#if LINUX_VERSION_CODE < 0x020314    
    struct wait_queue *sp_ptsfifo_wait;
#else
    wait_queue_head_t sp_ptsfifo_wait;
#endif
    int sp_ptsfifo_waiting;
    
    int linecounter;
};

#define TIMEDIFF(a,b) a.tv_usec - b.tv_usec + \
	    1000000 * (a.tv_sec - b.tv_sec)


/*
  Prototypes
*/

/* em8300_i2c.c */
int em8300_i2c_init(struct em8300_s *em);
void em8300_i2c_exit(struct em8300_s *em);
void em8300_clockgen_write(struct em8300_s *em, int abyte);

/* em8300_audio.c */
int em8300_audio_ioctl(struct em8300_s *em,unsigned int cmd, unsigned long arg);
int em8300_audio_open(struct em8300_s *em);
int em8300_audio_release(struct em8300_s *em);
int em8300_audio_setup(struct em8300_s *em);
int em8300_audio_write(struct em8300_s *em, const char * buf,
		       size_t count, loff_t *ppos);
int mpegaudio_command(struct em8300_s *em, int cmd);

/* em8300_ucode.c */
int em8300_ucode_upload(struct em8300_s *em, void *ucode_user, int ucode_size);

/* em8300_misc.c */
int em8300_setregblock(struct em8300_s *em, int offset, int val, int len);
int em8300_writeregblock(struct em8300_s *em, int offset, unsigned *buf, int len);
int em8300_waitfor(struct em8300_s *em, int reg, int val, int mask);

/* em8300_dicom.c */
void em8300_dicom_setBCS(struct em8300_s *em, int brightness, int contrast, int saturation);
int em8300_dicom_update(struct em8300_s *em);

/* em8300_video.c */
int em8300_video_start(struct em8300_s *em);
int em8300_video_setup(struct em8300_s *em);
int em8300_video_stop(struct em8300_s *em);
int em8300_video_release(struct em8300_s *em);
void em8300_video_setspeed(struct em8300_s *em, int speed);
int em8300_video_write(struct em8300_s *em, const char * buf,
		       size_t count, loff_t *ppos);
int em8300_video_ioctl(struct em8300_s *em, unsigned int cmd, unsigned long arg);
void em8300_video_check_ptsfifo(struct em8300_s *em);

/* em8300_spu.c */
int em8300_spu_write(struct em8300_s *em, const char * buf,
		       size_t count, loff_t *ppos);
int em8300_spu_open(struct em8300_s *em);
int em8300_spu_ioctl(struct em8300_s *em, unsigned int cmd, unsigned long arg);
int em8300_spu_init(struct em8300_s *em);
void em8300_spu_check_ptsfifo(struct em8300_s *em);


/* em8300_ioctl.c */
int em8300_control_ioctl(struct em8300_s *em, int cmd, unsigned long arg);
int em8300_ioctl_setvideomode(struct em8300_s *em, int mode);
int em8300_ioctl_setaspectratio(struct em8300_s *em, int ratio);
void em8300_ioctl_getstatus(struct em8300_s *em, char *usermsg);
int em8300_ioctl_init(struct em8300_s *em, em8300_microcode_t *useruc);

#endif
