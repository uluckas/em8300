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


/* C decompilation of sub_prepare_SPDIF by 
*  Anton Altaparmakov <antona@bigfoot.com>
*
* Notes:
*  I assume int to be 32bits in the following code! (do not compile on
*  16 bit architectures!!!). Also short int is assumed 16bits and char 8bits
*  for that matter, but that's standard stuff.
*
*  local1 = "in" = current inblock position pointer.
*
*  local3 = "i" = for loop counter.
*
*  local2 and local4 are untouched since I don't understand the functional
*  significance of the whole algorithm and hence don't know what they do.
*/

// Need unsigned otherwise get into trouble with signed shift rights!
void sub_prepare_SPDIF(struct em8300_s *em, unsigned char *outblock, unsigned char *inblock, unsigned int inlength)
{
    // 	ebp-4 = local1 = in	ebp-8 = local2
    //	ebp-0ch = local3 = i	ebp-10h = local4

    unsigned char *in; // 32 bit points to array of 8 bit chars
    unsigned short int local2; // 16 bit, unsigned
    unsigned int i; // 32 bit, signed
    unsigned char local4; // 8 bit, unsigned

    in = inblock;

    for (i = 0; i < (inlength >> 2); i++)
	{
	    if (em->dword_DB4 == 0xc0)
		em->dword_DB4 = 0;
	    {
		register int ebx;
		if (em->dword_DB4 < 0)
		    ebx = em->byte_D90[(em->dword_DB4 + 7) >> 3] & 0xff \
		          << (!((!em->dword_DB4 + 1) & 7) + 1) & 0xff;
		else 
		    ebx = (em->byte_D90[em->dword_DB4 >> 3] & 0xff) << (em->dword_DB4 & 7);
		local4 = (unsigned char)((ebx & 0x80) >> 1);
	    }
	    if (em->audio_mode == 1)
		local2 = in[0] << 8 | in[1];
	    else
		local2 = in[1] << 8 | in[0];
	    in += 2;

	    if (em->dword_DB4 != 0)
		outblock[i*8+3] = 2;
	    else
		outblock[i*8+3] = 0;

	    outblock[i*8+2] = local2 << 4;
	    outblock[i*8+1] = local2 >> 4;
	    outblock[i*8] = local2 >> 12 | local4;

	    if (em->audio_mode == 1)
		local2 = in[0] << 8 | in[1];
	    else
		local2 = in[1] << 8 | in[0];
	    in += 2;

	    outblock[i*8+7] = 1;
	    outblock[i*8+6] = local2 << 4;
	    outblock[i*8+5] = local2 >> 4;
	    outblock[i*8+4] = local2 >> 12 | local4;

	    ++em->dword_DB4;
	}
    return;
}

static void preprocess_analog(struct em8300_s *em, unsigned char *outbuf,
			     const unsigned char *inbuf_user, int inlength)
{
    int i;
    
    if(em->swapbytes) {
	if(em->stereo) {
	    for(i=0; i < inlength; i+=2) {
		get_user(outbuf[i+1], inbuf_user++);
		get_user(outbuf[i], inbuf_user++);
	    }
	} else {
	    for(i=0; i < inlength; i+=2) {
		get_user(outbuf[2*i+1], inbuf_user++);
		get_user(outbuf[2*i], inbuf_user++);
		outbuf[2*i+3] = outbuf[2*i+1];
		outbuf[2*i+2] = outbuf[2*i];
	    }
	}
    } else {
	for(i=0;i<inlength/2;i++) {
	    outbuf[2*i] = inbuf_user[i];
	    outbuf[2*i+1] = inbuf_user[i];
	}
	    
    }
}

static void preprocess_digital(struct em8300_s *em, unsigned char *outbuf,
			     const unsigned char *inbuf_user, int inlength)
{
    int i;
    unsigned char tmpbuf[0x600];

    if(em->swapbytes) {
	for(i=0; i < inlength; i+=2) {
	    get_user(tmpbuf[i+1], inbuf_user++);
	    get_user(tmpbuf[i], inbuf_user++);
	}
    } else
	copy_from_user(tmpbuf, inbuf_user, inlength);

    sub_prepare_SPDIF(em,outbuf, tmpbuf, inlength);
}

static void setup_mafifo(struct em8300_s *em) {
    if(em->audio_mode == EM8300_AUDIOMODE_ANALOG) {
	em->mafifo->preprocess_ratio = em->stereo ? 1 : 2;
	em->mafifo->preprocess_cb = &preprocess_analog;
	em->mafifo->preprocess_maxbufsize = -1;
    }
    else {
	em->mafifo->preprocess_ratio = 2;
	em->mafifo->preprocess_maxbufsize = 0x600;
	em->mafifo->preprocess_cb = &preprocess_digital;
    }
}

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
int set_stereo(struct em8300_s *em, int val) {
    em->stereo = val;
    setup_mafifo(em);

    return val;
}

static
int set_rate(struct em8300_s *em,int rate)
{
    em->clockgen &= ~CLOCKGEN_SAMPFREQ_MASK;

    switch(rate) {
    case 66000:
	em->clockgen |= CLOCKGEN_SAMPFREQ_66;
	break;
    case 44100:
	em->clockgen |= CLOCKGEN_SAMPFREQ_44;
	break;
    case 32000:
	em->clockgen |= CLOCKGEN_SAMPFREQ_32;
	break;
    case 48000:
    default:
	em->clockgen |= CLOCKGEN_SAMPFREQ_48;
	rate = 48000;
    }
    
    em->audio_rate = rate; 

    em8300_clockgen_write(em,em->clockgen);
    
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
	set_stereo(em,val);
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
    em->audio_first=1;
    return audio_start(em);
}

int em8300_audio_release(struct em8300_s *em)
{
    return audio_stop(em);    
}

static int set_audiomode(struct em8300_s *em, int mode) {
    unsigned char mutepattern_src[0x300];
    unsigned char mutepattern[0x600];
    
    em->audio_mode = mode;
	
    em->clockgen &= ~CLOCKGEN_OUTMASK;
    if(em->audio_mode == EM8300_AUDIOMODE_ANALOG)
	em->clockgen |= CLOCKGEN_ANALOGOUT;
    else
	em->clockgen |= CLOCKGEN_DIGITALOUT;
    
    em8300_clockgen_write(em,em->clockgen);

    memset(mutepattern_src, 0, sizeof(mutepattern_src));
    memset(em->byte_D90, 0, sizeof(em->byte_D90));

    em->byte_D90[1]=0x98;

   switch(em->audio_rate) {
    case 32000:
        em->byte_D90[3] = 0xc0;
        break;
    case 44100:
        em->byte_D90[3] = 0;
        break;
    case 48000:
        em->byte_D90[3] = 0x40;
        break;
    }
     
    switch(em->audio_mode) {
    case EM8300_AUDIOMODE_ANALOG:
	write_register(0x1fb0,0x62);
	em8300_setregblock(em, 2*ucregister(Mute_Pattern), 0, 0x600);
	printk("em8300_audio.o: Analog audio enabled\n");
	break;
    case EM8300_AUDIOMODE_DIGITALPCM:
	write_register(0x1fb0,0x3a0);

	em->byte_D90[0]=0x0;
	sub_prepare_SPDIF(em,mutepattern,mutepattern_src,0x300);
	
	em8300_writeregblock(em, 2*ucregister(Mute_Pattern), (unsigned *)mutepattern, 0x600);
	printk("em8300_audio.o: Digital PCM audio enabled\n");
	break;
    case EM8300_AUDIOMODE_DIGITALAC3:
	write_register(0x1fb0,0x3a0);

	em->byte_D90[0]=0x40;
	sub_prepare_SPDIF(em,mutepattern,mutepattern_src,0x300);

	em8300_writeregblock(em, 2*ucregister(Mute_Pattern), (unsigned *)mutepattern, 0x600);
	printk("em8300_audio.o: Digital AC3 audio enabled\n");
	break;
    }
    return 0;
}

int em8300_audio_setup(struct em8300_s *em) {
    int ret;

    em->clockgen = 0xb;

    set_rate(em,48000);
    
    set_audiomode(em,EM8300_AUDIOMODE_DEFAULT);
    
    ret = em8300_audio_flush(em);

    setup_mafifo(em);
    
    if(ret) {
	printk("em8300_audio.o: Couldn't zero audio buffer\n");
	return ret;
    }
    
    write_ucregister(MA_Threshold,6);
    
    mpegaudio_command(em,MACOMMAND_PLAY);
    write_register(0x1f47, 0x18);
    mpegaudio_command(em,MACOMMAND_PAUSE);

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

    if( ((em->audio_sync == AUDIO_SYNC_REQUESTED) || em->audio_first) && (em->audio_ptsvalid)) {
	int ret;

	if(!em->audio_first) {
	    em8300_fifo_sync(em->mafifo);
	    em8300_audio_flush(em);
	}

	ret = em8300_fifo_writeblocking(em->mafifo, count, buf,0);

	write_ucregister(MV_SCRlo, em->audio_pts & 0xffff);
	write_ucregister(MV_SCRhi, em->audio_pts >> 16);

	mpegaudio_command(em,MACOMMAND_PLAY);

	printk("Setting SCR: %d\n",em->audio_pts);

	em->audio_sync = AUDIO_SYNC_INACTIVE;
	em->audio_ptsvalid = 0;
	em->audio_first=0;
	return ret;
    }

    if(em->audio_ptsvalid) {
	long scr;
	
	scr = (read_ucregister(MV_SCRhi) << 16) | read_ucregister(MV_SCRlo);
	
	if(em->audio_rate) {
	    em->audio_lag = scr - (em->audio_pts -
		45000/4 * em8300_audio_calcbuffered(em)
		/ (em->audio_rate));
	    if(em->audio_sync == AUDIO_SYNC_INACTIVE &&
	       (em->audio_lag > AUDIO_LAG_LIMIT))
		{
		    em->audio_sync = AUDIO_SYNC_REQUESTED;
		    DEBUG(printk("em8300_audio.o: Audio out of sync (%d). Resyncing.\n",				em->audio_lag	));
		}		
	}
	/*
	if(em->audio_sync == AUDIO_SYNC_REQUESTED) {
	    if(em->audio_pts > scr) {
		em8300_audio_flush(em);
		mpegaudio_command(em,MACOMMAND_PAUSE);
		em->audio_syncpts=em->audio_pts;
		em->audio_sync=AUDIO_SYNC_INPROGRESS;
	    }
	}
	*/
	em->audio_ptsvalid=0;
    } 

    
    return em8300_fifo_writeblocking(em->mafifo, count, buf,0);
}
