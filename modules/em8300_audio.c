#define __NO_VERSION__

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/malloc.h>
#include <linux/version.h>
#include <linux/string.h>
#include <linux/pci.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/i2c-algo-bit.h>

#include "em8300_reg.h"

#include "em8300_fifo.h"
#include "em8300.h"

#include <linux/soundcard.h>


int mpegaudio_command(struct em8300_s *em, int cmd) {
    em8300_waitfor(em,ucregister(MA_Command), 0xffff, 0xffff);

    printk("MA_Command: %d\n",cmd);
    write_ucregister(MA_Command,cmd);

    return em8300_waitfor(em,ucregister(MA_Status), cmd, 0xffff);
}

static
int audio_start(struct em8300_s *em) {
    em->irqmask |= IRQSTATUS_AUDIO_FIFO;
    write_ucregister(Q_IrqMask,em->irqmask);
    return mpegaudio_command(em,MACOMMAND_PLAY);
}

static
int audio_stop(struct em8300_s *em) {
    em->irqmask &= ~IRQSTATUS_AUDIO_FIFO;
    write_ucregister(Q_IrqMask,em->irqmask);
    return mpegaudio_command(em,MACOMMAND_STOP);
}

static
int set_rate(struct em8300_s *em,int rate)
{
    em->audio_rate = rate; 
    switch(rate) {
    case 48000:
	em8300_clockgen_write(em,(em->clockgen & ~CLOCKGEN_SAMPFREQ_MASK) |
			      CLOCKGEN_SAMPFREQ_48);
	break;
    case 44100:
	em8300_clockgen_write(em,(em->clockgen & ~CLOCKGEN_SAMPFREQ_MASK) |
			      CLOCKGEN_SAMPFREQ_44);
	break;
    case 32000:
	em8300_clockgen_write(em,(em->clockgen & ~CLOCKGEN_SAMPFREQ_MASK) |
			      CLOCKGEN_SAMPFREQ_32);
	break;
    default:
	em8300_clockgen_write(em,(em->clockgen & ~CLOCKGEN_SAMPFREQ_MASK) |
			      CLOCKGEN_SAMPFREQ_48);
	rate = 48000;
    }
    return rate;
}

int em8300_audio_ioctl(struct em8300_s *em,unsigned int cmd, unsigned long arg)
{
    int err, len = 0;
    int val;

    if (_SIOC_DIR(cmd) != _SIOC_NONE && _SIOC_DIR(cmd) != 0) {
	/*
	 * Have to validate the address given by the process.
                 */
	len = _SIOC_SIZE(cmd);
	if (len < 1 || len > 65536 || arg == 0)
	    return -EFAULT;
	if (_SIOC_DIR(cmd) & _SIOC_WRITE)
	    if ((err = verify_area(VERIFY_READ, (void *)arg, len)) < 0)
		return err;
	if (_SIOC_DIR(cmd) & _SIOC_READ)
	    if ((err = verify_area(VERIFY_WRITE, (void *)arg, len)) < 0)
		return err;
    }

    switch(cmd) { 
    case SNDCTL_DSP_RESET:
	return 0;
    case SOUND_PCM_WRITE_RATE	:
	if (get_user(val, (int *)arg))
	    return -EFAULT;
	val = set_rate(em,val);
	break;
    case SNDCTL_DSP_GETBLKSIZE:
	val = 0x1000;
	break;
    case SNDCTL_DSP_STEREO:
	if (get_user(val, (int *)arg))
	    return -EFAULT;
	if (val > 1 || val < 0)
	    return -EINVAL;
	val = 1;
	break;
    case SNDCTL_DSP_SETFMT:
	if (get_user(val, (int *)arg))
	    return -EFAULT;

	switch(val) {
	case AFMT_S16_BE:
	    em->swapbytes = 0;
	    break;
	default:
	    val = AFMT_S16_LE;
	    em->swapbytes = 1;
	}
	break;
    case EM8300_IOCTL_AUDIO_SETPTS:
	if (get_user(em->audio_pts, (int *)arg))
	    return -EFAULT;
	em->audio_pts >>= 1;
	em->audio_ptsvalid=1;
	val=0;
	break;
    case EM8300_IOCTL_AUDIO_SYNC:
	if (get_user(em->audio_pts, (int *)arg)) 
	    return -EFAULT;
	em->audio_pts >>= 1;
	em->audio_ptsvalid=1;
	em->audio_sync=AUDIO_SYNC_REQUESTED;
	val=0;
	break;
    default:
	return -EINVAL;
    }
    return put_user(val, (int *)arg);
}

int em8300_audio_flush(struct em8300_s *em) {
    int audiobuf,audiobufsize;

    mpegaudio_command(em,MACOMMAND_STOP);

    /* Zero audio buffer */
    audiobuf = read_ucregister(MA_BuffStart_Lo) |
		       (read_ucregister(MA_BuffStart_Hi) << 16);
    audiobufsize = read_ucregister(MA_BuffSize) |
		       (read_ucregister(MA_BuffSize_Hi) << 16);

    return em8300_setregblock(em, audiobuf, 0, audiobufsize);
}

int em8300_audio_open(struct em8300_s *em)
{
    if(!em->ucodeloaded)
	return -ENODEV;
    em->swapbytes = 1;
    return audio_start(em);
}

int em8300_audio_release(struct em8300_s *em)
{
    return audio_stop(em);    
}

int em8300_audio_setup(struct em8300_s *em) {
    int ret;

    em->audio_mode = EM8300_AUDIOMODE_DEFAULT;
    
    em->clockgen = 0xb | CLOCKGEN_SAMPFREQ_48;

    em->clockgen &= ~CLOCKGEN_OUTMASK;
    if(em->audio_mode == EM8300_AUDIOMODE_ANALOG)
	em->clockgen |= CLOCKGEN_ANALOGOUT;
    else
	em->clockgen |= CLOCKGEN_DIGITALOUT;
    
    em8300_clockgen_write(em,em->clockgen);
    
    ret = em8300_audio_flush(em);
    
    if(ret) {
	printk("em8300_audio.o: Couldn't zero audio buffer\n");
	return ret;
    }
    
    write_ucregister(MA_Threshold,6);
    
    mpegaudio_command(em,MACOMMAND_PLAY);
    write_register(0x1f47, 0x18);
    mpegaudio_command(em,MACOMMAND_PAUSE);

    if(em->audio_mode == EM8300_AUDIOMODE_ANALOG)
	write_register(0x1fb0,0x62);
    else	
	write_register(0x1fb0,0x3a0);
    
    return 0;
}

int em8300_audio_calcbuffered(struct em8300_s *em) {
    int readptr,writeptr,bufsize,n;

    readptr = read_ucregister(MA_Rdptr) | (read_ucregister(MA_Rdptr_Hi) << 16);
    writeptr = read_ucregister(MA_Wrptr) | (read_ucregister(MA_Wrptr_Hi) << 16);
    bufsize = read_ucregister(MA_BuffSize) | (read_ucregister(MA_BuffSize_Hi) << 16);

    n = (bufsize+writeptr-readptr) % bufsize;
    
    return em8300_fifo_calcbuffered(em->mafifo) + n;
}

int em8300_audio_write(struct em8300_s *em, const char * buf,
		       size_t count, loff_t *ppos)
{
    if((em->audio_sync == AUDIO_SYNC_REQUESTED) && (em->audio_ptsvalid)) {
	int ret;
	
	//	em8300_fifo_sync(em->mafifo);
	em8300_audio_flush(em);

	ret = em8300_fifo_writeblocking(em->mafifo, count, buf,1,0);

	write_ucregister(MV_SCRlo, em->audio_pts & 0xffff);
	write_ucregister(MV_SCRhi, em->audio_pts >> 16);

	mpegaudio_command(em,MACOMMAND_PLAY);

	printk("Setting SCR: %d\n",em->audio_pts);

	em->audio_sync = AUDIO_SYNC_INACTIVE;
	em->audio_ptsvalid = 0;
	return ret;
    }

    if(em->audio_ptsvalid) {
	long picpts;
	picpts =
	    read_ucregister(PicPTSLo) +
	    (read_ucregister(PicPTSHi) << 16);

	if(em->audio_rate) {
	    em->audio_lag = picpts - (em->audio_pts -
		45000/4 * em8300_audio_calcbuffered(em)
		/ (em->audio_rate));
	    if(em->audio_sync == AUDIO_SYNC_INACTIVE &&
	       (em->audio_lag > AUDIO_LAG_LIMIT))	{
		em->audio_sync = AUDIO_SYNC_REQUESTED;
		DEBUG(printk("em8300_audio.o: Audio out of sync (%d). Resyncing.\n",				em->audio_lag	));
	    }	
	}

	if(em->audio_sync == AUDIO_SYNC_REQUESTED) {
	    if(em->audio_pts > picpts) {
		em8300_audio_flush(em);
		mpegaudio_command(em,MACOMMAND_PAUSE);
		em->audio_syncpts=em->audio_pts;
		em->audio_sync=AUDIO_SYNC_INPROGRESS;
	    }
	}
	em->audio_ptsvalid=0;
    } 

    
    return em8300_fifo_writeblocking(em->mafifo, count, buf,1,0);
}
